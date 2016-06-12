#ifndef _FAT_T_H
#define _FAT_T_H


#include <stdint.h>
#include <wchar.h>
//Signatures
#define boost_signature 0x29
#define boot_signature 0xAA55
#define fs_type_fat12__ "FAT12   "
//File types
#define fat_file_attrib_readonly    0x01
#define fat_file_attrib_hidden      0x02
#define fat_file_attrib_system      0x04
#define fat_file_attrib_vlabel      0x08
#define fat_file_attrib_directory   0x10
#define fat_file_attrib_archive     0x20
//Cluster types
#define fat_rootdir_clust 0
#define fat_invalid_clust -1


#pragma pack(push, 1)
typedef struct
{
	uint8_t     jump[3];
	char        oem_name[8];
	uint16_t    bytes_per_sector;
	uint8_t     sectors_per_cluster;
	uint16_t    reserved_sectors;
	uint8_t     num_fats;
	uint16_t    root_dir_entries;
	uint16_t    sector_count;
	uint8_t     media_descriptor;
	uint16_t    sectors_per_fat;
	uint16_t    sectors_per_track;
	uint16_t    head_count;
	uint32_t    hidden_sectors_count;
    uint32_t    fat32_reserved1;
    uint16_t    fat32_reserved2;
    uint8_t     boost_sign;
    uint32_t    volume_id;
    char        volume_label[11];
    char        fs_type_str[8];
    uint8_t     bootloader_code[448];
    uint16_t    boot_sign;
} fat12_bs;
typedef struct
{
	char        short_name[11];
	uint8_t     attrib;
	uint8_t     rsvd;
	uint8_t     create_time_tenth;
	uint16_t    create_time;
	uint16_t    create_date;
	uint16_t    last_read_date;
	uint16_t    clust_hight;
	uint16_t    last_write_date;
	uint16_t    last_write_time;
	uint16_t    clust_low;
	uint32_t    size;
} fat_dir_entry;
typedef struct
{
    char        order;
    wchar_t     part0[5];
    uint8_t     attrib;
    uint8_t     type;
    char        crc;
    wchar_t     part1[6];
    uint16_t    clust_low;
    wchar_t     part2[2];
} fat_lfn;
#pragma pack(pop)


#endif /*_FAT_T_H*/
