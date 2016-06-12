#include <string.h>
#include "fio.h"


int fat_get_cluster(void* data, int cluster)
{
    if ((data == NULL) || (cluster < 0) || (cluster > 3072))
        return fat_invalid_clust;
    int addr = cluster * 3 / 2;
    int fat_tmpa = 512 + addr;
    int fat_tmpv = *(uint16_t*)(((uint32_t)data) + fat_tmpa);
    if ((cluster % 2) == 0)
        return fat_tmpv & 0xFFF;
    else
        return fat_tmpv >> 4;
}


int fat_set_cluster(void* data, int cluster, int sv)
{
    if ((data == NULL) || (cluster < 0) || (cluster > 3072))
        return fat_invalid_clust;
    int addr = cluster * 3 / 2;
    //Fat1 table
    int fat1_tmpa = 512 + addr;
    int fat1_tmpv = *(uint16_t*)(((uint32_t)data) + fat1_tmpa);
    if ((cluster % 2) == 0)
    {
        fat1_tmpv &= 0xF000;
        fat1_tmpv |= (sv & 0xFFF);
    }
    else
    {
        fat1_tmpv &= 0x000F;
        fat1_tmpv |= ((sv & 0xFFF) << 4);
    }
    *(uint16_t*)(((uint32_t)data) + fat1_tmpa) = fat1_tmpv;
    //Fat2 table
    int fat2_tmpa = 10 * 512 + addr;
    int fat2_tmpv = *(uint16_t*)(((uint32_t)data) + fat2_tmpa);
    if ((cluster % 2) == 0)
    {
        fat2_tmpv &= 0xF000;
        fat2_tmpv |= (sv & 0xFFF);
    }
    else
    {
        fat2_tmpv &= 0x000F;
        fat2_tmpv |= ((sv & 0xFFF) << 4);
    }
    *(uint16_t*)(((uint32_t)data) + fat2_tmpa) = fat2_tmpv;
    return 0;
}


int fat_frr_cluster(void* data)
{
    if (data == NULL)
        return fat_invalid_clust;
    for (int i = 0; i < (2880 - 1 - 8*2 - 224*32/512); i++)
        if (fat_get_cluster(data, i) == 0)
            return i;
    return fat_invalid_clust;
}


void* fat_read_cluster(void* data, int cluster)
{
    if ((data == NULL) || (cluster < 0) || (cluster > 3072))
        return NULL;
    void* res;
    if (cluster == fat_rootdir_clust)
        res = calloc(14*512, 1);
    else
        res = calloc(1*512, 1);
    if (res == NULL)
        return NULL;
    int sector, size;
    if (cluster == fat_rootdir_clust)
    {
        sector = 19;
        size = 512 * 14;
    }
    else
    {
        sector = 33 + cluster - 2;
        size = 512 * 1;
    }
    unsigned char* buff = (unsigned char*)res;
    for (int i = 0; i < size; i++)
        buff[i] = ((unsigned char*)data)[(sector * 512) + i];
    return res;
}


int fat_write_cluster(void* data, int cluster, void* clust)
{
    if ((data == NULL) || (cluster < 0) || (cluster > 3072))
        return EXIT_FAILURE;
    int sector, size;
    if (cluster == fat_rootdir_clust)
    {
        sector = 19;
        size = 512 * 14;
    }
    else
    {
        sector = 33 + cluster - 2;
        size = 512 * 1;
    }
    unsigned char* buff = (unsigned char*)clust;
    for (int i = 0; i < size; i++)
        ((unsigned char*)data)[(sector * 512) + i] = buff[i];
    return EXIT_SUCCESS;
}


void* fat_read_clusters(void* data, int cluster)
{
    if ((data == NULL) || (cluster < 0) || (cluster > 3072))
        return NULL;
    if (cluster == fat_rootdir_clust)
        return fat_read_cluster(data, cluster);
    void* res;
    int count = 0;
    do
    {
        res = realloc(res, (count + 1) * 512);
        if (res == NULL)
            return NULL;
        void* c = fat_read_cluster(data, cluster);
        if  (c == NULL)
        {
            free(res);
            return NULL;
        }
        char* dest = (char*)res + (count * 512);
        memcpy((void*)dest, c, 512);
        free(c);
        count++;
        cluster = fat_get_cluster(data, cluster);
    } while ((cluster != 0) && (cluster < 0xFF0));
    return res;
}
