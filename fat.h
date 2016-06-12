#ifndef _FAT_H
#define _FAT_H


#include "fat_t.h"


//Partion
extern void* fat_create_partion();
extern void fat_wrt_bootloader(void* data, void* bootloader);
//Dir
extern int fat_get_dir_cluster(void* data, char* path, int clust);
extern int fat_add_(void* data, int cluster, char* name, int clust, int size, int dir);
//File objects
extern int fat_create_directory(void* data, char* path, char* name);
extern int fat_create_file(void* data, char* path, char* name, char* filename);


#endif /*_FAT_H*/
