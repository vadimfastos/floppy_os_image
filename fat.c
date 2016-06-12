#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat.h"
#include "fio.h"
#include "dir.h"
#include "crc.h"


void* fat_create_partion()
{
    void* res = calloc(512, 2880);
    if (res == NULL)
        return NULL;
    void* bootloader = calloc(512, 1);
    if (bootloader == NULL)
    {
        free(res);
        return NULL;
    }
    fat_wrt_bootloader(res, bootloader);
    free(bootloader);
    fat_set_cluster(res, 0, 0xFFF);
    fat_set_cluster(res, 1, 0xFFF);
    return res;
}


void fat_wrt_bootloader(void* data, void* bootloader)
{
    if (data == NULL)
        return;
    fat12_bs* bootsect = (fat12_bs*)data;
    for (int i = 0; i < 512; i++)
        ((char*)data)[i] = ((char*)bootloader)[i];
    bootsect->bytes_per_sector = 512;
    bootsect->sectors_per_cluster = 1;
    bootsect->reserved_sectors = 1;
    bootsect->num_fats = 2;
    bootsect->root_dir_entries = 224;
    bootsect->sector_count = 2880;
    bootsect->media_descriptor = 0xF8;
    bootsect->sectors_per_fat = 9;
    bootsect->sectors_per_track = 18;
    bootsect->head_count = 2;
    bootsect->hidden_sectors_count = 0;
    bootsect->fat32_reserved1 = 0;
    bootsect->fat32_reserved2 = 0;
    bootsect->boost_sign = boost_signature;
    bootsect->volume_id = 0;
    bootsect->boot_sign = boot_signature;
    char fat12_str[] = "FAT12   ";
    for (int i = 0; i <  8; i++)
        bootsect->fs_type_str[i] = fat12_str[i];
    return;
}


int fat_get_dir_cluster(void* data, char* path, int clust)
{
    if ((strcmp(path, "/") == 0) || (strcmp(path, "\\") == 0) || (*path == 0))
        return clust;
    //Get directory with cur_level-1
    char p[256];
    char n[256];
    for (int i = 0; i < 256; i++)
        p[i] = 0;
    for (int i = 0; i < 256; i++)
        n[i] = 0;
    char* dest = p;
    int   dest_len = 0;
    while (*path)
    {
        if (*path == '/' || *path == '\\')
        {
            if (dest == n)
                break;
            int tmp = 1;
            for (int i = 1; path[i]; i++)
                if ((path[i] == '/' || path[i] == '\\') && (path[i+1] != 0))
                tmp = 0;
            if (tmp)
            {
                dest[dest_len] = *path;
                dest = n;
                dest_len = 0;
                path++;
                continue;
            }
        }
        dest[dest_len] = *path;
        dest_len++;
        path++;
    }
    int cluster = fat_get_dir_cluster(data, p, clust);
    if (cluster == fat_invalid_clust)
        return fat_invalid_clust;
    //Read cluster
    void* cluster_data = NULL;
    if (cluster == fat_rootdir_clust)
    {
        cluster_data = realloc(cluster_data, 224*32+1);
        if (cluster_data == NULL)
            return -1;
        void* addr = (void*)((char*)data + 19*512);
        memcpy(cluster_data, addr, 224*32);
    }
    else
    {
        int pos = 0;
        do
        {
            cluster_data = realloc(cluster_data, pos * 512 + 512);
            if (cluster_data == NULL)
                return -1;
            char* addr = (char*)(cluster_data);
            addr += pos * 512;
            void* t = fat_read_cluster(data, cluster);
            if (t != NULL)
            {
                memcpy((void*)addr, t, 512);
                free(t);
            }
            cluster = fat_get_cluster(data, cluster);
            pos++;
        } while ((cluster < 0xFF0) && (cluster != 0));
    }
    //Scan cluster
    fat_dir_entry* cdata = (fat_dir_entry*)(cluster_data);
    char filename[256];
    int ln_pos = 0;
    int ln = 0;
    int crc = 0;
    while (cdata->short_name[0])
    {
        if (cdata->attrib == 0xF)
        {
            if (ln == 0)
            {
                crc = ((fat_lfn*)(cdata))->crc;
                ln = 1;
            }
            if (((fat_lfn*)(cdata))->crc != crc)
            {
                ln = 0;
                ln_pos = 0;
                continue;
            }
            wchar_t tstr[14];
            wchar_t* rdp;
            for (int i = 0; i < 13; i++)
            {
                if (i == 0)
                    rdp = ((fat_lfn*)(cdata))->part0;
                else if (i == 5)
                    rdp = ((fat_lfn*)(cdata))->part1;
                else if (i == 11)
                    rdp = ((fat_lfn*)(cdata))->part2;
                if (*rdp == 0)
                    break;
                tstr[i] = *rdp;
                tstr[i+1] = 0;
                rdp++;
            }
            size_t tlen = wcslen(tstr);
            for (int i = 0; i < ln_pos; i++)
                filename[i+tlen] = filename[i];
            for (int i = 0; i < tlen; i++)
            {
                filename[i] = tstr[i];
                ln_pos++;
            }
        }
        else if (cdata->attrib & fat_file_attrib_directory)
        {
            if (ln)
            {
                //Zero filename
                filename[ln_pos] = 0;
                //Reset lfn variables
                ln_pos = 0;
                ln = 0;
                //Test for crc
                if (crc8(cdata->short_name, 11) != crc)
                    continue;
            }
            else
            {
                int name_i = 0;
                for (int i = 0; i < 8; i++)
                {
                    if (cdata->short_name[i] == ' ')
                        break;
                    filename[name_i] = cdata->short_name[i];
                    name_i++;
                }
                filename[name_i] = 0;
            }
            if (strcmp(filename, n) == 0)
                return cdata->clust_low & 0xFFF;
        }
        cdata++;
    }
    return -1;
}


int fat_create_directory(void* data, char* path, char* name)
{
    if (data == NULL)
        return EXIT_FAILURE;
    //Get cluster of parent directory
    int cluster = fat_get_dir_cluster(data, path, fat_rootdir_clust);
    if (cluster == -1)
        return EXIT_FAILURE;
    //Create element
    int dir_clust = fat_frr_cluster(data);
    if (dir_clust == fat_invalid_clust)
        return EXIT_FAILURE;
    fat_set_cluster(data, dir_clust, 0xFFF);
    if (fat_add_(data, cluster, name, dir_clust, 0, 1) == EXIT_FAILURE)
    {
        fat_set_cluster(data, dir_clust, 0);
        return EXIT_FAILURE;
    }
    //Add files "." and ".."
    fat_add_(data, dir_clust, ".", dir_clust, 0, 1);
    fat_add_(data, dir_clust, "..", cluster, 0, 1);
    return EXIT_SUCCESS;
}


int fat_create_file(void* data, char* path, char* name, char* filename)
{
    if (data == NULL)
        return EXIT_FAILURE;
    //Get cluster of parent directory
    int cluster = fat_get_dir_cluster(data, path, fat_rootdir_clust);
    if (cluster == -1)
        return EXIT_FAILURE;
    //Read file
    FILE* fin = fopen(filename, "rb");
    if (fin == NULL)
        return EXIT_FAILURE;
    //Create element
    fseek(fin, 0, SEEK_END);
    int size = (int)ftell(fin);
    fseek(fin, 0, SEEK_SET);
    int file_clust = 0;
    if (size)
    {
        int clusters[3000];
        for (int i = 0; i < 3000; i++)
            clusters[i] = 0;
        int size_in_clust = size / 512;
        if (size % 512)
            size_in_clust++;
        for (int i = 0; i < size_in_clust; i++)
        {
            int this_clust = fat_frr_cluster(data);
            if (this_clust == fat_invalid_clust)
            {
                for (int j = 0; j < (i - 1); j++)
                    fat_set_cluster(data, clusters[j], 0);
                return EXIT_FAILURE;
            }
            clusters[i] = this_clust;
            fat_set_cluster(data, clusters[i], 0xFFF);
            if (i)
                fat_set_cluster(data, clusters[i-1], this_clust);
        }
        //Write file
        void* file__ = calloc(512, size_in_clust);
        char* file_data = (char*)file__;
        for (int i = 0; i < size; i++)
            file_data[i] = getc(fin);
        for (int i = 0; i < size_in_clust; i++)
        {
            fat_write_cluster(data, clusters[i], file_data);
            file_data += 512;
        }
        file_clust = clusters[0];
    }
    if (fat_add_(data, cluster, name, file_clust, size, 0) == EXIT_FAILURE)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}


int fat_add_(void* data, int cluster, char* name, int clust, int size, int dir)
{
    //Variables
    int is_ln = 0;
    int ns = 0;
    int es = 0;
    //Short name
    char short_name[12];
    for (int i = 0; i < 11; i++)
        short_name[i] = ' ';
    short_name[11] = 0;
    //Test for long name (8.3)
    int* ts = &ns;
    for (int i = 0; name[i]; i++)
    {
        if (!dir && name[i] == '.')
            ts = &es;
        else
            (*ts)++;
    }
    if ((ns > 8) || (es > 3))
        is_ln = 1;
    //Get short name
    int is_n = 1;
    for (int i = 0, j = 0; (i < 11) && (name[j]); j++)
    {
        //Exit for directory (8 max)
        if (dir && i >= 8)
            break;
        //Get simbol
        char c = name[j];
        //Test file ext
        if (!dir && c == '.')
        {
            i = 8;
            is_n = 0;
            continue;
        }
        //Test lfn simbol
        if ((islower(c)) ||
            (c == '+') ||
            (c == ',') ||
            (c == ';') ||
            (c == '=') ||
            (c == '[') ||
            (c == ']'))
            is_ln = 1;
        //LFN writing
        if (ns > 8)
        {
            if (is_n)
            {
                if (i == 6)
                {
                    short_name[6] = '~';
                    short_name[7] = '1';
                    i = 8;
                    continue;
                }
            }
            if (is_n && i >= 8);
            else
            {
                short_name[i] = toupper(c);
                 i++;
            }

        }
        //Usual writing
        else
        {
            short_name[i] = toupper(c);
            i++;
        }
    }
    //Long name
    if (is_ln)
    {
        //Long name
        int parts_count = 0;
        parts_count = strlen(name) / 13;
        if (strlen(name) % 13)
            parts_count++;
        //Write elements
        fat_lfn elements[20];
        fat_lfn descr;
        descr.attrib = 0xF;
        descr.type = 0;
        descr.clust_low = 0;
        descr.crc = crc8(short_name, 11);
        for (int i = 0; i < 5; i++)
            descr.part0[i] = 0xFFFF;
        for (int i = 0; i < 6; i++)
            descr.part1[i] = 0xFFFF;
        for (int i = 0; i < 2; i++)
            descr.part2[i] = 0xFFFF;
        for (int i = 0; i < 20; i++)
            elements[i] = descr;
        char* pstr = name;
        int cur_element = 0;
        wchar_t* pthis = elements[cur_element].part0;
        int count = 0;
        while (*pstr)
        {
            if (count == 0)
            {
                int torder;
                torder = cur_element+1;
                if (parts_count == torder)
                    torder += 0x40;
                elements[cur_element].order = (char)torder;
            }
            *(pthis++) = *(pstr++);
            count++;
            if (count == 5)
                pthis = elements[cur_element].part1;
            else if (count == 11)
                pthis = elements[cur_element].part2;
            else if (count == 13)
            {
                cur_element++;
                pthis = elements[cur_element].part0;
                count = 0;
            }
        }
        *pthis = 0;
        //Write
        for (int i = parts_count-1; i >= 0; i--)
            fat_write_elem(data, cluster, (fat_dir_entry*)&elements[i]);
    }
    //Add element with short name
    fat_dir_entry* elem = calloc(sizeof(fat_dir_entry), 1);
    if (elem == NULL)
        return EXIT_FAILURE;
    for (int i = 0; i < 11; i++)
        elem->short_name[i] = short_name[i];
    elem->attrib = 0;
    if (dir)
        elem->attrib |= fat_file_attrib_directory;
    elem->clust_hight = 0;
    elem->clust_low = clust;
    elem->size = size;
    fat_write_elem(data, cluster, elem);
    free(elem);
    return EXIT_SUCCESS;
}
