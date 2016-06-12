#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "fat.h"


void add_files(void* addr, char* dir, char* cur_dir)
{
    char filename[256];
    strcpy(filename, dir);
    strcpy(filename+strlen(filename), cur_dir);
    strcpy(filename+strlen(filename), "/*");
    WIN32_FIND_DATA fd = {};
    HANDLE hFind = FindFirstFile(filename, &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do {
            if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, ".."))
                continue;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (fat_create_directory(addr, cur_dir, fd.cFileName) == EXIT_FAILURE)
                {
                    printf("Cant create directory %s%s\n", cur_dir, fd.cFileName);
                    return;
                }
                else
                    printf("Create directory %s%s\n", cur_dir, fd.cFileName);
                char* new_cur_dir = calloc(sizeof(char), strlen(cur_dir) + strlen(fd.cFileName) + 1 + 1);
                strcpy(new_cur_dir, cur_dir);
                strcpy(new_cur_dir + strlen(new_cur_dir), fd.cFileName);
                strcpy(new_cur_dir + strlen(new_cur_dir), "/");
                add_files(addr, dir, new_cur_dir);
                free(new_cur_dir);
            }
            else
            {
                char* filename = calloc(sizeof(char), strlen(dir) + 1 + strlen(cur_dir) + strlen(fd.cFileName) + 1);
                strcpy(filename, dir);
                strcpy(filename+strlen(filename), cur_dir);
                strcpy(filename+strlen(filename), fd.cFileName);
                int st = fat_create_file(addr, cur_dir, fd.cFileName, filename);
                if (st == EXIT_FAILURE)
                    printf("Cant add file %s%s\n", cur_dir, fd.cFileName);
                else
                    printf("Add file %s%s\n", cur_dir, fd.cFileName);
                free(filename);
            }
        } while (FindNextFile(hFind, &fd));
        FindClose(hFind);
    }
}


int main(int argc, char** argv)
{
    //Arguments
    char* source = NULL;
    char* output = NULL;
    char* loader = NULL;
    if (argc < 5 || argc > 9)
    {
        printf("To few arguments...\n");
        return 1;
    }
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-o") == 0)
        {
            output = strdup(argv[i+1]);
            i++;
        }
        else if (strcmp(argv[i], "-c") == 0)
        {
            source = strdup(argv[i+1]);
            i++;
        }
        else if (strcmp(argv[i], "-b") == 0)
        {
            loader = strdup(argv[i+1]);
            i++;
        }
    }
    //Create image
    void* image = fat_create_partion();
    if (loader == NULL)
    {
        printf("Error creating image...\n");
        return 1;
    }
    printf("Creating image ok...\n");
    //Add bootloader
    if (loader != NULL)
    {
        void* loader_data = calloc(1, 513);
        if (loader_data != NULL)
        {
            FILE* loader_file = fopen(loader, "rb");
            if (loader_file != NULL)
            {
                int r;
                for (int i = 0; (r = getc(loader_file)) != EOF && i < 512; i++)
                    ((uint8_t*)loader_data)[i] = r;
                fat_wrt_bootloader(image, loader_data);
                fclose(loader_file);
                printf("Add loader ok...\n");
            }
            else
                printf("Add loader error...\n");
        }
        else
                printf("Add loader error...\n");
    }
    //Add files
    add_files(image, source, "/");
    printf("Add files ok...\n");
    //Write image
    FILE* image_file = fopen(output, "wb");
    if (image_file == NULL)
    {
        printf("Error writing image...\n");
        return 1;
    }
    uint8_t* image__ = (uint8_t*)image;
    for (int i = 0; i < 2880*512; i++)
        fputc(image__[i], image_file);
    fclose(image_file);
    printf("Writing image ok...\n");
    return 0;
}
