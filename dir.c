#include "dir.h"
#include "fio.h"


int fat_write_elem(void* data, int cluster, fat_dir_entry* elem)
{
    if ((data == NULL) || (cluster < 0) || (cluster > 3072))
        return EXIT_FAILURE;
    int max_pos = 16;
    if (cluster == fat_rootdir_clust)
        max_pos = 14 * 16;
    while (1)
    {
        void* c = fat_read_cluster(data, cluster);
        if (c == NULL)
            return EXIT_FAILURE;
        fat_dir_entry* cur_pos = (fat_dir_entry*)c;
        int i = 0;
        while ((cur_pos[i].short_name[0] != 0) && (i < max_pos))
            i++;
        if (i < max_pos)
        {
            cur_pos[i] = *elem;
            int r = fat_write_cluster(data, cluster, c);
            free(c);
            return r;
        }
        int t = fat_get_cluster(data, cluster);
        if ((t == 0) || (t >= 0xFF0))
            break;
        cluster = t;
    }
    if (cluster != fat_rootdir_clust)
    {
        int new_cluster = fat_frr_cluster(data);
        if (new_cluster != fat_invalid_clust)
        {
            void* t = calloc(512, 1);
            if (t != NULL)
            {
                fat_set_cluster(data, cluster, new_cluster);
                fat_set_cluster(data, new_cluster, 0xFFF);
                *((fat_dir_entry*)(t)) = *elem;
                int r = fat_write_cluster(data, new_cluster, t);
                free(t);
                return r;
            }
        }
    }
    return EXIT_FAILURE;
}
