#ifndef _FIO_H
#define _FIO_H


#include <stdlib.h>
#include "fat_t.h"


//Clusters
extern int fat_get_cluster(void* data, int cluster);
extern int fat_set_cluster(void* data, int cluster, int sv);
extern int fat_frr_cluster(void* data);
//Clusters IO
extern void* fat_read_cluster(void* data, int cluster);
extern int fat_write_cluster(void* data, int cluster, void* clust);
extern void* fat_read_clusters(void* data, int cluster);


#endif /*_FIO_H*/
