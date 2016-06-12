#ifndef _DIR_H
#define _DIR_H


#include <stdlib.h>
#include "fat_t.h"


extern int fat_write_elem(void* data, int cluster, fat_dir_entry* elem);


#endif /*_DIR_H*/
