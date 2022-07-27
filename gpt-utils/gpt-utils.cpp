/*
 * Copyright (c) 2013,2016,2020 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _LARGEFILE64_SOURCE /* enable lseek64() */

/******************************************************************************
 * INCLUDE SECTION
 ******************************************************************************/
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/fs.h>
#include <limits.h>
#include <dirent.h>
#include <linux/kernel.h>
#include <asm/byteorder.h>
#include <map>
#include <vector>
#include <string>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>


#define LOG_TAG "gpt-utils"
#include <cutils/log.h>
#include <cutils/properties.h>
#include "gpt-utils.h"
#include <zlib.h>
#include <endian.h>


/******************************************************************************
 * DEFINE SECTION
 ******************************************************************************/
#define BLK_DEV_FILE    "/dev/block/mmcblk0"
/* list the names of the backed-up partitions to be swapped */
/* extension used for the backup partitions - tzbak, abootbak, etc. */
#define BAK_PTN_NAME_EXT    "bak"
#define XBL_PRIMARY         "/dev/block/bootdevice/by-name/xbl"
#define XBL_BACKUP          "/dev/block/bootdevice/by-name/xblbak"
#define XBL_AB_PRIMARY      "/dev/block/bootdevice/by-name/xbl_a"
#define XBL_AB_SECONDARY    "/dev/block/bootdevice/by-name/xbl_b"
/* GPT defines */
#define MAX_LUNS                    26
//This will allow us to get the root lun path from the path to the partition.
//i.e: from /dev/block/sdaXXX get /dev/block/sda. The assumption here is that
//the boot critical luns lie between sda to sdz which is acceptable because
//only user added external disks,etc would lie beyond that limit which do not
//contain partitions that interest us here.
#define PATH_TRUNCATE_LOC (sizeof("/dev/block/sda") - 1)

//From /dev/block/sda get just sda
#define LUN_NAME_START_LOC (sizeof("/dev/block/") - 1)
#define BOOT_LUN_A_ID 1
#define BOOT_LUN_B_ID 2
/******************************************************************************
 * MACROS
 ******************************************************************************/


#define GET_4_BYTES(ptr)    ((uint32_t) *((uint8_t *)(ptr)) | \
        ((uint32_t) *((uint8_t *)(ptr) + 1) << 8) | \
        ((uint32_t) *((uint8_t *)(ptr) + 2) << 16) | \
        ((uint32_t) *((uint8_t *)(ptr) + 3) << 24))

#define GET_8_BYTES(ptr)    ((uint64_t) *((uint8_t *)(ptr)) | \
        ((uint64_t) *((uint8_t *)(ptr) + 1) << 8) | \
        ((uint64_t) *((uint8_t *)(ptr) + 2) << 16) | \
        ((uint64_t) *((uint8_t *)(ptr) + 3) << 24) | \
        ((uint64_t) *((uint8_t *)(ptr) + 4) << 32) | \
        ((uint64_t) *((uint8_t *)(ptr) + 5) << 40) | \
        ((uint64_t) *((uint8_t *)(ptr) + 6) << 48) | \
        ((uint64_t) *((uint8_t *)(ptr) + 7) << 56))

#define PUT_4_BYTES(ptr, y)   *((uint8_t *)(ptr)) = (y) & 0xff; \
        *((uint8_t *)(ptr) + 1) = ((y) >> 8) & 0xff; \
        *((uint8_t *)(ptr) + 2) = ((y) >> 16) & 0xff; \
        *((uint8_t *)(ptr) + 3) = ((y) >> 24) & 0xff;

/******************************************************************************
 * TYPES
 ******************************************************************************/
using namespace std;
enum gpt_state {
    GPT_OK = 0,
    GPT_BAD_SIGNATURE,
    GPT_BAD_CRC
};
//List of LUN's containing boot critical images.
//Required in the case of UFS devices
struct update_data {
     char lun_list[MAX_LUNS][PATH_MAX];
     uint32_t num_valid_entries;
};

int32_t set_boot_lun(char *sg_dev,uint8_t boot_lun_id);
/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
/**
 *  ==========================================================================
 *
 *  \brief  Read/Write len bytes from/to block dev
 *
 *  \param [in] fd      block dev file descriptor (returned from open)
 *  \param [in] rw      RW flag: 0 - read, != 0 - write
 *  \param [in] offset  block dev offset [bytes] - RW start position
 *  \param [in] buf     Pointer to the buffer containing the data
 *  \param [in] len     RW size in bytes. Buf must be at least that big
 *
 *  \return  0 on success
 *
 *  ==========================================================================
 */
static int blk_rw(int fd, int rw, int64_t offset, uint8_t *buf, unsigned len)
{
    int r;

    if (lseek64(fd, offset, SEEK_SET) < 0) {
        fprintf(stderr, "block dev lseek64 %" PRIi64 " failed: %s\n", offset,
                strerror(errno));
        return -1;
    }

    if (rw)
        r = write(fd, buf, len);
    else
        r = read(fd, buf, len);

    if (r < 0) {
        fprintf(stderr, "block dev %s failed: %s\n", rw ? "write" : "read",
                strerror(errno));
    } else {
        if (rw) {
            r = fsync(fd);
            if (r < 0)
                fprintf(stderr, "fsync failed: %s\n", strerror(errno));
        } else {
            r = 0;
        }
    }

    return r;
}



/**
 *  ==========================================================================
 *
 *  \brief  Search within GPT for partition entry with the given name
 *  or it's backup twin (name-bak).
 *
 *  \param [in] ptn_name        Partition name to seek
 *  \param [in] pentries_start  Partition entries array start pointer
 *  \param [in] pentries_end    Partition entries array end pointer
 *  \param [in] pentry_size     Single partition entry size [bytes]
 *
 *  \return  First partition entry pointer that matches the name or NULL
 *
 *  ==========================================================================
 */
static uint8_t *gpt_pentry_seek(const char *ptn_name,
                                const uint8_t *pentries_start,
                                const uint8_t *pentries_end,
                                uint32_t pentry_size)
{
    char     *pentry_name;
    unsigned  len = strlen(ptn_name);
    unsigned  i;
    char      name8[MAX_GPT_NAME_SIZE] = {0}; // initialize with null

    for (pentry_name = (char *) (pentries_start + PARTITION_NAME_OFFSET);
         pentry_name < (char *) pentries_end;
         pentry_name += pentry_size) {

        /* Partition names in GPT are UTF-16 - ignoring UTF-16 2nd byte */
        for (i = 0; i < sizeof(name8) / 2; i++)
            name8[i] = pentry_name[i * 2];
        name8[i] = '\0';

        if (!strncmp(ptn_name, name8, len)) {
            if (name8[len] == 0 || !strcmp(&name8[len], BAK_PTN_NAME_EXT))
                return (uint8_t *) (pentry_name - PARTITION_NAME_OFFSET);
        }
    }

    return NULL;
}



/**
 *  ==========================================================================
 *
 *  \brief  Swaps boot chain in GPT partition entries array
 *
 *  \param [in] pentries_start  Partition entries array start
 *  \param [in] pentries_end    Partition entries array end
 *  \param [in] pentry_size     Single partition entry size
 *
 *  \return  0 on success, 1 if no backup partitions found
 *
 *  ==========================================================================
 */
static int gpt_boot_chain_swap(const uint8_t *pentries_start,
                                const uint8_t *pentries_end,
                                uint32_t pentry_size)
{
    const char ptn_swap_list[][MAX_GPT_NAME_SIZE] = { PTN_SWAP_LIST };

    int backup_not_found = 1;
    unsigned i;

    for (i = 0; i < ARRAY_SIZE(ptn_swap_list); i++) {
        uint8_t *ptn_entry;
        uint8_t *ptn_bak_entry;
        uint8_t ptn_swap[PTN_ENTRY_SIZE];
        //Skip the xbl partition on UFS devices. That is handled
        //seperately.
        if (gpt_utils_is_ufs_device() && !strncmp(ptn_swap_list[i],
                                PTN_XBL,
                                strlen(PTN_XBL)))
            continue;

        ptn_entry = gpt_pentry_seek(ptn_swap_list[i], pentries_start,
                        pentries_end, pentry_size);
        if (ptn_entry == NULL)
            continue;

        ptn_bak_entry = gpt_pentry_seek(ptn_swap_list[i],
                        ptn_entry + pentry_size, pentries_end, pentry_size);
        if (ptn_bak_entry == NULL) {
            fprintf(stderr, "'%s' partition not backup - skip safe update\n",
                    ptn_swap_list[i]);
            continue;
        }

        /* swap primary <-> backup partition entries */
        memcpy(ptn_swap, ptn_entry, PTN_ENTRY_SIZE);
        memcpy(ptn_entry, ptn_bak_entry, PTN_ENTRY_SIZE);
        memcpy(ptn_bak_entry, ptn_swap, PTN_ENTRY_SIZE);
        backup_not_found = 0;
    }

    return backup_not_found;
}



/**
 *  ==========================================================================
 *
 *  \brief  Sets secondary GPT boot chain
 *
 *  \param [in] fd    block dev file descriptor
 *  \param [in] boot  Boot chain to switch to
 *
 *  \return  0 on success
 *
 *  ==========================================================================
 */
static int gpt2_set_boot_chain(int fd, enum boot_chain boot)
{
    int64_t  gpt2_header_offset;
    uint64_t pentries_start_offset;
    uint32_t gpt_header_size;
    uint32_t pentry_size;
    uint32_t pentries_array_size;

    uint8_t *gpt_header = NULL;
    uint8_t  *pentries = NULL;
    uint32_t crc;
    uint32_t crc_zero;
    uint32_t blk_size = 0;
    int r;


    crc_zero = crc32(0L, Z_NULL, 0);
    if (ioctl(fd, BLKSSZGET, &blk_size) != 0) {
            fprintf(stderr, "Failed to get GPT device block size: %s\n",
                            strerror(errno));
            r = -1;
            goto EXIT;
    }
    gpt_header = (uint8_t*)malloc(blk_size);
    if (!gpt_header) {
            fprintf(stderr, "Failed to allocate memory to hold GPT block\n");
            r = -1;
            goto EXIT;
    }
    gpt2_header_offset = lseek64(fd, 0, SEEK_END) - blk_size;
    if (gpt2_header_offset < 0) {
        fprintf(stderr, "Getting secondary GPT header offset failed: %s\n",
                strerror(errno));
        r = -1;
        goto EXIT;
    }

    /* Read primary GPT header from block dev */
    r = blk_rw(fd, 0, blk_size, gpt_header, blk_size);

    if (r) {
            fprintf(stderr, "Failed to read primary GPT header from blk dev\n");
            goto EXIT;
    }
    pentries_start_offset =
        GET_8_BYTES(gpt_header + PENTRIES_OFFSET) * blk_size;
    pentry_size = GET_4_BYTES(gpt_header + PENTRY_SIZE_OFFSET);
    pentries_array_size =
        GET_4_BYTES(gpt_header + PARTITION_COUNT_OFFSET) * pentry_size;

    pentries = (uint8_t *) calloc(1, pentries_array_size);
    if (pentries == NULL) {
        fprintf(stderr,
                    "Failed to alloc memory for GPT partition entries array\n");
        r = -1;
        goto EXIT;
    }
    /* Read primary GPT partititon entries array from block dev */
    r = blk_rw(fd, 0, pentries_start_offset, pentries, pentries_array_size);
    if (r)
        goto EXIT;

    crc = crc32(crc_zero, pentries, pentries_array_size);
    if (GET_4_BYTES(gpt_header + PARTITION_CRC_OFFSET) != crc) {
        fprintf(stderr, "Primary GPT partition entries array CRC invalid\n");
        r = -1;
        goto EXIT;
    }

    /* Read secondary GPT header from block dev */
    r = blk_rw(fd, 0, gpt2_header_offset, gpt_header, blk_size);
    if (r)
        goto EXIT;

    gpt_header_size = GET_4_BYTES(gpt_header + HEADER_SIZE_OFFSET);
    pentries_start_offset =
        GET_8_BYTES(gpt_header + PENTRIES_OFFSET) * blk_size;

    if (boot == BACKUP_BOOT) {
        r = gpt_boot_chain_swap(pentries, pentries + pentries_array_size,
                                pentry_size);
        if (r)
            goto EXIT;
    }

    crc = crc32(crc_zero, pentries, pentries_array_size);
    PUT_4_BYTES(gpt_header + PARTITION_CRC_OFFSET, crc);

    /* header CRC is calculated with this field cleared */
    PUT_4_BYTES(gpt_header + HEADER_CRC_OFFSET, 0);
    crc = crc32(crc_zero, gpt_header, gpt_header_size);
    PUT_4_BYTES(gpt_header + HEADER_CRC_OFFSET, crc);

    /* Write the modified GPT header back to block dev */
    r = blk_rw(fd, 1, gpt2_header_offset, gpt_header, blk_size);
    if (!r)
        /* Write the modified GPT partititon entries array back to block dev */
        r = blk_rw(fd, 1, pentries_start_offset, pentries,
                    pentries_array_size);

EXIT:
    if(gpt_header)
            free(gpt_header);
    if (pentries)
            free(pentries);
    return r;
}

/**
 *  ==========================================================================
 *
 *  \brief  Checks GPT state (header signature and CRC)
 *
 *  \param [in] fd      block dev file descriptor
 *  \param [in] gpt     GPT header to be checked
 *  \param [out] state  GPT header state
 *
 *  \return  0 on success
 *
 *  ==========================================================================
 */
static int gpt_get_state(int fd, enum gpt_instance gpt, enum gpt_state *state)
{
    int64_t gpt_header_offset;
    uint32_t gpt_header_size;
    uint8_t  *gpt_header = NULL;
    uint32_t crc;
    uint32_t crc_zero;
    uint32_t blk_size = 0;

    *state = GPT_OK;

    crc_zero = crc32(0L, Z_NULL, 0);
    if (ioctl(fd, BLKSSZGET, &blk_size) != 0) {
            fprintf(stderr, "Failed to get GPT device block size: %s\n",
                            strerror(errno));
            goto error;
    }
    gpt_header = (uint8_t*)malloc(blk_size);
    if (!gpt_header) {
            fprintf(stderr, "gpt_get_state:Failed to alloc memory for header\n");
            goto error;
    }
    if (gpt == PRIMARY_GPT)
        gpt_header_offset = blk_size;
    else {
        gpt_header_offset = lseek64(fd, 0, SEEK_END) - blk_size;
        if (gpt_header_offset < 0) {
            fprintf(stderr, "gpt_get_state:Seek to end of GPT part fail\n");
            goto error;
        }
    }

    if (blk_rw(fd, 0, gpt_header_offset, gpt_header, blk_size)) {
        fprintf(stderr, "gpt_get_state: blk_rw failed\n");
        goto error;
    }
    if (memcmp(gpt_header, GPT_SIGNATURE, sizeof(GPT_SIGNATURE)))
        *state = GPT_BAD_SIGNATURE;
    gpt_header_size = GET_4_BYTES(gpt_header + HEADER_SIZE_OFFSET);

    crc = GET_4_BYTES(gpt_header + HEADER_CRC_OFFSET);
    /* header CRC is calculated with this field cleared */
    PUT_4_BYTES(gpt_header + HEADER_CRC_OFFSET, 0);
    if (crc32(crc_zero, gpt_header, gpt_header_size) != crc)
        *state = GPT_BAD_CRC;
    free(gpt_header);
    return 0;
error:
    if (gpt_header)
            free(gpt_header);
    return -1;
}



/**
 *  ==========================================================================
 *
 *  \brief  Sets GPT header state (used to corrupt and fix GPT signature)
 *
 *  \param [in] fd     block dev file descriptor
 *  \param [in] gpt    GPT header to be checked
 *  \param [in] state  GPT header state to set (GPT_OK or GPT_BAD_SIGNATURE)
 *
 *  \return  0 on success
 *
 *  ==========================================================================
 */
static int gpt_set_state(int fd, enum gpt_instance gpt, enum gpt_state state)
{
    int64_t gpt_header_offset;
    uint32_t gpt_header_size;
    uint8_t  *gpt_header = NULL;
    uint32_t crc;
    uint32_t crc_zero;
    uint32_t blk_size = 0;

    crc_zero = crc32(0L, Z_NULL, 0);
    if (ioctl(fd, BLKSSZGET, &blk_size) != 0) {
            fprintf(stderr, "Failed to get GPT device block size: %s\n",
                            strerror(errno));
            goto error;
    }
    gpt_header = (uint8_t*)malloc(blk_size);
    if (!gpt_header) {
            fprintf(stderr, "Failed to alloc memory for gpt header\n");
            goto error;
    }
    if (gpt == PRIMARY_GPT)
        gpt_header_offset = blk_size;
    else {
        gpt_header_offset = lseek64(fd, 0, SEEK_END) - blk_size;
        if (gpt_header_offset < 0) {
            fprintf(stderr, "Failed to seek to end of GPT device\n");
            goto error;
        }
    }
    if (blk_rw(fd, 0, gpt_header_offset, gpt_header, blk_size)) {
        fprintf(stderr, "Failed to r/w gpt header\n");
        goto error;
    }
    if (state == GPT_OK)
        memcpy(gpt_header, GPT_SIGNATURE, sizeof(GPT_SIGNATURE));
    else if (state == GPT_BAD_SIGNATURE)
        *gpt_header = 0;
    else {
        fprintf(stderr, "gpt_set_state: Invalid state\n");
        goto error;
    }

    gpt_header_size = GET_4_BYTES(gpt_header + HEADER_SIZE_OFFSET);

    /* header CRC is calculated with this field cleared */
    PUT_4_BYTES(gpt_header + HEADER_CRC_OFFSET, 0);
    crc = crc32(crc_zero, gpt_header, gpt_header_size);
    PUT_4_BYTES(gpt_header + HEADER_CRC_OFFSET, crc);

    if (blk_rw(fd, 1, gpt_header_offset, gpt_header, blk_size)) {
        fprintf(stderr, "gpt_set_state: blk write failed\n");
        goto error;
    }
    return 0;
error:
    if(gpt_header)
           free(gpt_header);
    return -1;
}

int get_scsi_node_from_bootdevice(const char *bootdev_path,
                char *sg_node_path,
                size_t buf_size)
{
        char sg_dir_path[PATH_MAX] = {0};
        char real_path[PATH_MAX] = {0};
        DIR *scsi_dir = NULL;
        struct dirent *de;
        int node_found = 0;
        if (!bootdev_path || !sg_node_path) {
                fprintf(stderr, "%s : invalid argument\n",
                                 __func__);
                goto error;
        }
        if (readlink(bootdev_path, real_path, sizeof(real_path) - 1) < 0) {
                        fprintf(stderr, "failed to resolve link for %s(%s)\n",
                                        bootdev_path,
                                        strerror(errno));
                        goto error;
        }
        if(strlen(real_path) < PATH_TRUNCATE_LOC + 1){
            fprintf(stderr, "Unrecognized path :%s:\n",
                           real_path);
            goto error;
        }
        //For the safe side in case there are additional partitions on
        //the XBL lun we truncate the name.
        real_path[PATH_TRUNCATE_LOC] = '\0';
        if(strlen(real_path) < LUN_NAME_START_LOC + 1){
            fprintf(stderr, "Unrecognized truncated path :%s:\n",
                           real_path);
            goto error;
        }
        //This will give us /dev/block/sdb/device/scsi_generic
        //which contains a file sgY whose name gives us the path
        //to /dev/sgY which we return
        snprintf(sg_dir_path, sizeof(sg_dir_path) - 1,
                        "/sys/block/%s/device/scsi_generic",
                        &real_path[LUN_NAME_START_LOC]);
        scsi_dir = opendir(sg_dir_path);
        if (!scsi_dir) {
                fprintf(stderr, "%s : Failed to open %s(%s)\n",
                                __func__,
                                sg_dir_path,
                                strerror(errno));
                goto error;
        }
        while((de = readdir(scsi_dir))) {
                if (de->d_name[0] == '.')
                        continue;
                else if (!strncmp(de->d_name, "sg", 2)) {
                          snprintf(sg_node_path,
                                        buf_size -1,
                                        "/dev/%s",
                                        de->d_name);
                          fprintf(stderr, "%s:scsi generic node is :%s:\n",
                                          __func__,
                                          sg_node_path);
                          node_found = 1;
                          break;
                }
        }
        if(!node_found) {
                fprintf(stderr,"%s: Unable to locate scsi generic node\n",
                               __func__);
                goto error;
        }
        closedir(scsi_dir);
        return 0;
error:
        if (scsi_dir)
                closedir(scsi_dir);
        return -1;
}



//Swtich betwieen using either the primary or the backup
//boot LUN for boot. This is required since UFS boot partitions
//cannot have a backup GPT which is what we use for failsafe
//updates of the other 'critical' partitions. This function will
//not be invoked for emmc targets and on UFS targets is only required
//to be invoked for XBL.
//
//The algorithm to do this is as follows:
//- Find the real block device(eg: /dev/block/sdb) that corresponds
//  to the /dev/block/bootdevice/by-name/xbl(bak) symlink
//
//- Once we have the block device 'node' name(sdb in the above example)
//  use this node to to locate the scsi generic device that represents
//  it by checking the file /sys/block/sdb/device/scsi_generic/sgY
//
//- Once we locate sgY we call the query ioctl on /dev/sgy to switch
//the boot lun to either LUNA or LUNB
int gpt_utils_set_xbl_boot_partition(enum boot_chain chain)
{
        struct stat st;
        ///sys/block/sdX/device/scsi_generic/
        char sg_dev_node[PATH_MAX] = {0};
        uint8_t boot_lun_id = 0;
        const char *boot_dev = NULL;

        if (chain == BACKUP_BOOT) {
                boot_lun_id = BOOT_LUN_B_ID;
                if (!stat(XBL_BACKUP, &st))
                        boot_dev = XBL_BACKUP;
                else if (!stat(XBL_AB_SECONDARY, &st))
                        boot_dev = XBL_AB_SECONDARY;
                else {
                        fprintf(stderr, "%s: Failed to locate secondary xbl\n",
                                        __func__);
                        goto error;
                }
        } else if (chain == NORMAL_BOOT) {
                boot_lun_id = BOOT_LUN_A_ID;
                if (!stat(XBL_PRIMARY, &st))
                        boot_dev = XBL_PRIMARY;
                else if (!stat(XBL_AB_PRIMARY, &st))
                        boot_dev = XBL_AB_PRIMARY;
                else {
                        fprintf(stderr, "%s: Failed to locate primary xbl\n",
                                        __func__);
                        goto error;
                }
        } else {
                fprintf(stderr, "%s: Invalid boot chain id\n", __func__);
                goto error;
        }
        //We need either both xbl and xblbak or both xbl_a and xbl_b to exist at
        //the same time. If not the current configuration is invalid.
        if((stat(XBL_PRIMARY, &st) ||
                                stat(XBL_BACKUP, &st)) &&
                        (stat(XBL_AB_PRIMARY, &st) ||
                         stat(XBL_AB_SECONDARY, &st))) {
                fprintf(stderr, "%s:primary/secondary XBL prt not found(%s)\n",
                                __func__,
                                strerror(errno));
                goto error;
        }
        fprintf(stderr, "%s: setting %s lun as boot lun\n",
                        __func__,
                        boot_dev);
        if (get_scsi_node_from_bootdevice(boot_dev,
                                sg_dev_node,
                                sizeof(sg_dev_node))) {
                fprintf(stderr, "%s: Failed to get scsi node path for xblbak\n",
                                __func__);
                goto error;
        }
        /* set boot lun using /dev/sg or /dev/ufs-bsg* */
        if (set_boot_lun(sg_dev_node, boot_lun_id)) {
                fprintf(stderr, "%s: Failed to set xblbak as boot partition\n",
                                __func__);
                goto error;
        }
        return 0;
error:
        return -1;
}

int gpt_utils_is_ufs_device()
{
    char bootdevice[PROPERTY_VALUE_MAX] = {0};
    property_get("ro.boot.bootdevice", bootdevice, "N/A");
    if (strlen(bootdevice) < strlen(".ufshc") + 1)
        return 0;
    return (!strncmp(&bootdevice[strlen(bootdevice) - strlen(".ufshc")],
                            ".ufshc",
                            sizeof(".ufshc")));
}
//dev_path is the path to the block device that contains the GPT image that
//needs to be updated. This would be the device which holds one or more critical
//boot partitions and their backups. In the case of EMMC this function would
//be invoked only once on /dev/block/mmcblk1 since it holds the GPT image
//containing all the partitions For UFS devices it could potentially be
//invoked multiple times, once for each LUN containing critical image(s) and
//their backups
int prepare_partitions(enum boot_update_stage stage, const char *dev_path)
{
    int r = 0;
    int fd = -1;
    int is_ufs = gpt_utils_is_ufs_device();
    enum gpt_state gpt_prim, gpt_second;
    enum boot_update_stage internal_stage;
    struct stat xbl_partition_stat;

    if (!dev_path) {
        fprintf(stderr, "%s: Invalid dev_path\n",
                        __func__);
        r = -1;
        goto EXIT;
    }
    fd = open(dev_path, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "%s: Opening '%s' failed: %s\n",
                        __func__,
                       BLK_DEV_FILE,
                       strerror(errno));
        r = -1;
        goto EXIT;
    }
    r = gpt_get_state(fd, PRIMARY_GPT, &gpt_prim) ||
        gpt_get_state(fd, SECONDARY_GPT, &gpt_second);
    if (r) {
        fprintf(stderr, "%s: Getting GPT headers state failed\n",
                        __func__);
        goto EXIT;
    }

    /* These 2 combinations are unexpected and unacceptable */
    if (gpt_prim == GPT_BAD_CRC || gpt_second == GPT_BAD_CRC) {
        fprintf(stderr, "%s: GPT headers CRC corruption detected, aborting\n",
                        __func__);
        r = -1;
        goto EXIT;
    }
    if (gpt_prim == GPT_BAD_SIGNATURE && gpt_second == GPT_BAD_SIGNATURE) {
        fprintf(stderr, "%s: Both GPT headers corrupted, aborting\n",
                        __func__);
        r = -1;
        goto EXIT;
    }

    /* Check internal update stage according GPT headers' state */
    if (gpt_prim == GPT_OK && gpt_second == GPT_OK)
        internal_stage = UPDATE_MAIN;
    else if (gpt_prim == GPT_BAD_SIGNATURE)
        internal_stage = UPDATE_BACKUP;
    else if (gpt_second == GPT_BAD_SIGNATURE)
        internal_stage = UPDATE_FINALIZE;
    else {
        fprintf(stderr, "%s: Abnormal GPTs state: primary (%d), secondary (%d), "
                "aborting\n", __func__, gpt_prim, gpt_second);
        r = -1;
        goto EXIT;
    }

    /* Stage already set - ready for update, exitting */
    if ((int) stage == (int) internal_stage - 1)
        goto EXIT;
    /* Unexpected stage given */
    if (stage != internal_stage) {
        r = -1;
        goto EXIT;
    }

    switch (stage) {
    case UPDATE_MAIN:
            if (is_ufs) {
                if(stat(XBL_PRIMARY, &xbl_partition_stat)||
                                stat(XBL_BACKUP, &xbl_partition_stat)){
                        //Non fatal error. Just means this target does not
                        //use XBL but relies on sbl whose update is handled
                        //by the normal methods.
                        fprintf(stderr, "%s: xbl part not found(%s).Assuming sbl in use\n",
                                        __func__,
                                        strerror(errno));
                } else {
                        //Switch the boot lun so that backup boot LUN is used
                        r = gpt_utils_set_xbl_boot_partition(BACKUP_BOOT);
                        if(r){
                                fprintf(stderr, "%s: Failed to set xbl backup partition as boot\n",
                                                __func__);
                                goto EXIT;
                        }
                }
        }
        //Fix up the backup GPT table so that it actually points to
        //the backup copy of the boot critical images
        fprintf(stderr, "%s: Preparing for primary partition update\n",
                        __func__);
        r = gpt2_set_boot_chain(fd, BACKUP_BOOT);
        if (r) {
            if (r < 0)
                fprintf(stderr,
                                "%s: Setting secondary GPT to backup boot failed\n",
                                __func__);
            /* No backup partitions - do not corrupt GPT, do not flag error */
            else
                r = 0;
            goto EXIT;
        }
        //corrupt the primary GPT so that the backup(which now points to
        //the backup boot partitions is used)
        r = gpt_set_state(fd, PRIMARY_GPT, GPT_BAD_SIGNATURE);
        if (r) {
            fprintf(stderr, "%s: Corrupting primary GPT header failed\n",
                            __func__);
            goto EXIT;
        }
        break;
    case UPDATE_BACKUP:
        if (is_ufs) {
                if(stat(XBL_PRIMARY, &xbl_partition_stat)||
                                stat(XBL_BACKUP, &xbl_partition_stat)){
                        //Non fatal error. Just means this target does not
                        //use XBL but relies on sbl whose update is handled
                        //by the normal methods.
                        fprintf(stderr, "%s: xbl part not found(%s).Assuming sbl in use\n",
                                        __func__,
                                        strerror(errno));
                } else {
                        //Switch the boot lun so that backup boot LUN is used
                        r = gpt_utils_set_xbl_boot_partition(NORMAL_BOOT);
                        if(r) {
                                fprintf(stderr, "%s: Failed to set xbl backup partition as boot\n",
                                                __func__);
                                goto EXIT;
                        }
                }
        }
        //Fix the primary GPT header so that is used
        fprintf(stderr, "%s: Preparing for backup partition update\n",
                        __func__);
        r = gpt_set_state(fd, PRIMARY_GPT, GPT_OK);
        if (r) {
            fprintf(stderr, "%s: Fixing primary GPT header failed\n",
                             __func__);
            goto EXIT;
        }
        //Corrupt the scondary GPT header
        r = gpt_set_state(fd, SECONDARY_GPT, GPT_BAD_SIGNATURE);
        if (r) {
            fprintf(stderr, "%s: Corrupting secondary GPT header failed\n",
                            __func__);
            goto EXIT;
        }
        break;
    case UPDATE_FINALIZE:
        //Undo the changes we had made in the UPDATE_MAIN stage so that the
        //primary/backup GPT headers once again point to the same set of
        //partitions
        fprintf(stderr, "%s: Finalizing partitions\n",
                        __func__);
        r = gpt2_set_boot_chain(fd, NORMAL_BOOT);
        if (r < 0) {
            fprintf(stderr, "%s: Setting secondary GPT to normal boot failed\n",
                            __func__);
            goto EXIT;
        }

        r = gpt_set_state(fd, SECONDARY_GPT, GPT_OK);
        if (r) {
            fprintf(stderr, "%s: Fixing secondary GPT header failed\n",
                            __func__);
            goto EXIT;
        }
        break;
    default:;
    }

EXIT:
    if (fd >= 0) {
       fsync(fd);
       close(fd);
    }
    return r;
}

int add_lun_to_update_list(char *lun_path, struct update_data *dat)
{
        uint32_t i = 0;
        struct stat st;
        if (!lun_path || !dat){
                fprintf(stderr, "%s: Invalid data",
                                __func__);
                return -1;
        }
        if (stat(lun_path, &st)) {
                fprintf(stderr, "%s: Unable to access %s. Skipping adding to list",
                                __func__,
                                lun_path);
                return -1;
        }
        if (dat->num_valid_entries == 0) {
                fprintf(stderr, "%s: Copying %s into lun_list[%d]\n",
                                __func__,
                                lun_path,
                                i);
                strlcpy(dat->lun_list[0], lun_path,
                                PATH_MAX * sizeof(char));
                dat->num_valid_entries = 1;
        } else {
                for (i = 0; (i < dat->num_valid_entries) &&
                                (dat->num_valid_entries < MAX_LUNS - 1); i++) {
                        //Check if the current LUN is not already part
                        //of the lun list
                        if (!strncmp(lun_path,dat->lun_list[i],
                                                strlen(dat->lun_list[i]))) {
                                //LUN already in list..Return
                                return 0;
                        }
                }
                fprintf(stderr, "%s: Copying %s into lun_list[%d]\n",
                                __func__,
                                lun_path,
                                dat->num_valid_entries);
                //Add LUN path lun list
                strlcpy(dat->lun_list[dat->num_valid_entries], lun_path,
                                PATH_MAX * sizeof(char));
                dat->num_valid_entries++;
        }
        return 0;
}

int prepare_boot_update(enum boot_update_stage stage)
{
        int is_ufs = gpt_utils_is_ufs_device();
        struct stat ufs_dir_stat;
        struct update_data data;
        int rcode = 0;
        uint32_t i = 0;
        int is_error = 0;
        const char ptn_swap_list[][MAX_GPT_NAME_SIZE] = { PTN_SWAP_LIST };
        //Holds /dev/block/bootdevice/by-name/*bak entry
        char buf[PATH_MAX] = {0};
        //Holds the resolved path of the symlink stored in buf
        char real_path[PATH_MAX] = {0};

        if (!is_ufs) {
                //emmc device. Just pass in path to mmcblk0
                return prepare_partitions(stage, BLK_DEV_FILE);
        } else {
                //Now we need to find the list of LUNs over
                //which the boot critical images are spread
                //and set them up for failsafe updates.To do
                //this we find out where the symlinks for the
                //each of the paths under
                ///dev/block/bootdevice/by-name/PTN_SWAP_LIST
                //actually point to.
                fprintf(stderr, "%s: Running on a UFS device\n",
                                __func__);
                memset(&data, '\0', sizeof(struct update_data));
                for (i=0; i < ARRAY_SIZE(ptn_swap_list); i++) {
                        //XBL on UFS does not follow the convention
                        //of being loaded based on well known GUID'S.
                        //We take care of switching the UFS boot LUN
                        //explicitly later on.
                        if (!strncmp(ptn_swap_list[i],
                                                PTN_XBL,
                                                strlen(PTN_XBL)))
                                continue;
                        snprintf(buf, sizeof(buf),
                                        "%s/%sbak",
                                        BOOT_DEV_DIR,
                                        ptn_swap_list[i]);
                        if (stat(buf, &ufs_dir_stat)) {
                                continue;
                        }
                        if (readlink(buf, real_path, sizeof(real_path) - 1) < 0)
                        {
                                fprintf(stderr, "%s: readlink error. Skipping %s",
                                                __func__,
                                                strerror(errno));
                        } else {
                              if(strlen(real_path) < PATH_TRUNCATE_LOC + 1){
                                    fprintf(stderr, "Unknown path.Skipping :%s:\n",
                                                real_path);
                                } else {
                                    real_path[PATH_TRUNCATE_LOC] = '\0';
                                    add_lun_to_update_list(real_path, &data);
                                }
                        }
                        memset(buf, '\0', sizeof(buf));
                        memset(real_path, '\0', sizeof(real_path));
                }
                for (i=0; i < data.num_valid_entries; i++) {
                        fprintf(stderr, "%s: Preparing %s for update stage %d\n",
                                        __func__,
                                        data.lun_list[i],
                                        stage);
                        rcode = prepare_partitions(stage, data.lun_list[i]);
                        if (rcode != 0)
                        {
                                fprintf(stderr, "%s: Failed to prepare %s.Continuing..\n",
                                                __func__,
                                                data.lun_list[i]);
                                is_error = 1;
                        }
                }
        }
        if (is_error)
                return -1;
        return 0;
}

//Given a parttion name(eg: rpm) get the path to the block device that
//represents the GPT disk the partition resides on. In the case of emmc it
//would be the default emmc dev(/dev/block/mmcblk0). In the case of UFS we look
//through the /dev/block/bootdevice/by-name/ tree for partname, and resolve
//the path to the LUN from there.
static int get_dev_path_from_partition_name(const char *partname,
                char *buf,
                size_t buflen)
{
        struct stat st;
        char path[PATH_MAX] = {0};
        if (!partname || !buf || buflen < ((PATH_TRUNCATE_LOC) + 1)) {
                ALOGE("%s: Invalid argument", __func__);
                goto error;
        }
        if (gpt_utils_is_ufs_device()) {
                //Need to find the lun that holds partition partname
                snprintf(path, sizeof(path),
                                "%s/%s",
                                BOOT_DEV_DIR,
                                partname);
                if (stat(path, &st)) {
                        goto error;
                }
                if (readlink(path, buf, buflen) < 0)
                {
                        goto error;
                } else {
                        buf[PATH_TRUNCATE_LOC] = '\0';
                }
        } else {
                snprintf(buf, buflen, BLK_DEV_FILE);
        }
        return 0;

error:
        return -1;
}

int gpt_utils_get_partition_map(vector<string>& ptn_list,
                map<string, vector<string>>& partition_map) {
        char devpath[PATH_MAX] = {'\0'};
        map<string, vector<string>>::iterator it;
        if (ptn_list.size() < 1) {
                fprintf(stderr, "%s: Invalid ptn list\n", __func__);
                goto error;
        }
        //Go through the passed in list
        for (uint32_t i = 0; i < ptn_list.size(); i++)
        {
                //Key in the map is the path to the device that holds the
                //partition
                if (get_dev_path_from_partition_name(ptn_list[i].c_str(),
                                devpath,
                                sizeof(devpath))) {
                        //Not necessarily an error. The partition may just
                        //not be present.
                        continue;
                }
                string path = devpath;
                it = partition_map.find(path);
                if (it != partition_map.end()) {
                        it->second.push_back(ptn_list[i]);
                } else {
                        vector<string> str_vec;
                        str_vec.push_back( ptn_list[i]);
                        partition_map.insert(pair<string, vector<string>>
                                        (path, str_vec));
                }
                memset(devpath, '\0', sizeof(devpath));
        }
        return 0;
error:
        return -1;
}

//Get the block size of the disk represented by decsriptor fd
static uint32_t gpt_get_block_size(int fd)
{
        uint32_t block_size = 0;
        if (fd < 0) {
                ALOGE("%s: invalid descriptor",
                                __func__);
                goto error;
        }
        if (ioctl(fd, BLKSSZGET, &block_size) != 0) {
                ALOGE("%s: Failed to get GPT dev block size : %s",
                                __func__,
                                strerror(errno));
                goto error;
        }
        return block_size;
error:
        return 0;
}

//Write the GPT header present in the passed in buffer back to the
//disk represented by fd
static int gpt_set_header(uint8_t *gpt_header, int fd,
                enum gpt_instance instance)
{
        uint32_t block_size = 0;
        off64_t gpt_header_offset = 0;
        if (!gpt_header || fd < 0) {
                ALOGE("%s: Invalid arguments",
                                __func__);
                goto error;
        }
        block_size = gpt_get_block_size(fd);
        if (block_size == 0) {
                ALOGE("%s: Failed to get block size", __func__);
                goto error;
        }
        if (instance == PRIMARY_GPT)
                gpt_header_offset = block_size;
        else
                gpt_header_offset = lseek64(fd, 0, SEEK_END) - block_size;
        if (gpt_header_offset <= 0) {
                ALOGE("%s: Failed to get gpt header offset",__func__);
                goto error;
        }
        if (blk_rw(fd, 1, gpt_header_offset, gpt_header, block_size)) {
                ALOGE("%s: Failed to write back GPT header", __func__);
                goto error;
        }
        return 0;
error:
        return -1;
}

//Read out the GPT header for the disk that contains the partition partname
static uint8_t* gpt_get_header(const char *partname, enum gpt_instance instance)
{
        uint8_t* hdr = NULL;
        char devpath[PATH_MAX] = {0};
        int64_t hdr_offset = 0;
        uint32_t block_size = 0;
        int fd = -1;
        if (!partname) {
                ALOGE("%s: Invalid partition name", __func__);
                goto error;
        }
        if (get_dev_path_from_partition_name(partname, devpath, sizeof(devpath))
                        != 0) {
                ALOGE("%s: Failed to resolve path for %s",
                                __func__,
                                partname);
                goto error;
        }
        fd = open(devpath, O_RDWR);
        if (fd < 0) {
                ALOGE("%s: Failed to open %s : %s",
                                __func__,
                                devpath,
                                strerror(errno));
                goto error;
        }
        block_size = gpt_get_block_size(fd);
        if (block_size == 0)
        {
                ALOGE("%s: Failed to get gpt block size for %s",
                                __func__,
                                partname);
                goto error;
        }

        hdr = (uint8_t*)malloc(block_size);
        if (!hdr) {
                ALOGE("%s: Failed to allocate memory for gpt header",
                                __func__);
        }
        if (instance == PRIMARY_GPT)
                hdr_offset = block_size;
        else {
                hdr_offset = lseek64(fd, 0, SEEK_END) - block_size;
        }
        if (hdr_offset < 0) {
                ALOGE("%s: Failed to get gpt header offset",
                                __func__);
                goto error;
        }
        if (blk_rw(fd, 0, hdr_offset, hdr, block_size)) {
                ALOGE("%s: Failed to read GPT header from device",
                                __func__);
                goto error;
        }
        close(fd);
        return hdr;
error:
        if (fd >= 0)
                close(fd);
        if (hdr)
                free(hdr);
        return NULL;
}

//Returns the partition entry array based on the
//passed in buffer which contains the gpt header.
//The fd here is the descriptor for the 'disk' which
//holds the partition
static uint8_t* gpt_get_pentry_arr(uint8_t *hdr, int fd)
{
        uint64_t pentries_start = 0;
        uint32_t pentry_size = 0;
        uint32_t block_size = 0;
        uint32_t pentries_arr_size = 0;
        uint8_t *pentry_arr = NULL;
        int rc = 0;
        if (!hdr) {
                ALOGE("%s: Invalid header", __func__);
                goto error;
        }
        if (fd < 0) {
                ALOGE("%s: Invalid fd", __func__);
                goto error;
        }
        block_size = gpt_get_block_size(fd);
        if (!block_size) {
                ALOGE("%s: Failed to get gpt block size for",
                                __func__);
                goto error;
        }
        pentries_start = GET_8_BYTES(hdr + PENTRIES_OFFSET) * block_size;
        pentry_size = GET_4_BYTES(hdr + PENTRY_SIZE_OFFSET);
        pentries_arr_size =
                GET_4_BYTES(hdr + PARTITION_COUNT_OFFSET) * pentry_size;
        pentry_arr = (uint8_t*)calloc(1, pentries_arr_size);
        if (!pentry_arr) {
                ALOGE("%s: Failed to allocate memory for partition array",
                                __func__);
                goto error;
        }
        rc = blk_rw(fd, 0,
                        pentries_start,
                        pentry_arr,
                        pentries_arr_size);
        if (rc) {
                ALOGE("%s: Failed to read partition entry array",
                                __func__);
                goto error;
        }
        return pentry_arr;
error:
        if (pentry_arr)
                free(pentry_arr);
        return NULL;
}

static int gpt_set_pentry_arr(uint8_t *hdr, int fd, uint8_t* arr)
{
        uint32_t block_size = 0;
        uint64_t pentries_start = 0;
        uint32_t pentry_size = 0;
        uint32_t pentries_arr_size = 0;
        int rc = 0;
        if (!hdr || fd < 0 || !arr) {
                ALOGE("%s: Invalid argument", __func__);
                goto error;
        }
        block_size = gpt_get_block_size(fd);
        if (!block_size) {
                ALOGE("%s: Failed to get gpt block size for",
                                __func__);
                goto error;
        }
        pentries_start = GET_8_BYTES(hdr + PENTRIES_OFFSET) * block_size;
        pentry_size = GET_4_BYTES(hdr + PENTRY_SIZE_OFFSET);
        pentries_arr_size =
                GET_4_BYTES(hdr + PARTITION_COUNT_OFFSET) * pentry_size;
        rc = blk_rw(fd, 1,
                        pentries_start,
                        arr,
                        pentries_arr_size);
        if (rc) {
                ALOGE("%s: Failed to read partition entry array",
                                __func__);
                goto error;
        }
        return 0;
error:
        return -1;
}



//Allocate a handle used by calls to the "gpt_disk" api's
struct gpt_disk * gpt_disk_alloc()
{
        struct gpt_disk *disk;
        disk = (struct gpt_disk *)malloc(sizeof(struct gpt_disk));
        if (!disk) {
                ALOGE("%s: Failed to allocate memory", __func__);
                goto end;
        }
        memset(disk, 0, sizeof(struct gpt_disk));
end:
        return disk;
}

//Free previously allocated/initialized handle
void gpt_disk_free(struct gpt_disk *disk)
{
        if (!disk)
                return;
        if (disk->hdr)
                free(disk->hdr);
        if (disk->hdr_bak)
                free(disk->hdr_bak);
        if (disk->pentry_arr)
                free(disk->pentry_arr);
        if (disk->pentry_arr_bak)
                free(disk->pentry_arr_bak);
        free(disk);
        return;
}

//fills up the passed in gpt_disk struct with information about the
//disk represented by path dev. Returns 0 on success and -1 on error.
int gpt_disk_get_disk_info(const char *dev, struct gpt_disk *dsk)
{

	struct gpt_disk *disk = NULL;
	int fd = -1;
	uint32_t gpt_header_size = 0;
	uint32_t crc_zero;

	crc_zero = crc32(0L, Z_NULL, 0);
        if (!dsk || !dev) {
                ALOGE("%s: Invalid arguments", __func__);
                goto error;
        }
        disk = dsk;
        disk->hdr = gpt_get_header(dev, PRIMARY_GPT);
        if (!disk->hdr) {
                ALOGE("%s: Failed to get primary header", __func__);
                goto error;
        }
        gpt_header_size = GET_4_BYTES(disk->hdr + HEADER_SIZE_OFFSET);
        disk->hdr_crc = crc32(crc_zero, disk->hdr, gpt_header_size);
        disk->hdr_bak = gpt_get_header(dev, SECONDARY_GPT);
        if (!disk->hdr_bak) {
                ALOGE("%s: Failed to get backup header", __func__);
                goto error;
        }
        disk->hdr_bak_crc = crc32(crc_zero, disk->hdr_bak, gpt_header_size);

        //Descriptor for the block device. We will use this for further
        //modifications to the partition table
        if (get_dev_path_from_partition_name(dev,
                                disk->devpath,
                                sizeof(disk->devpath)) != 0) {
                ALOGE("%s: Failed to resolve path for %s",
                                __func__,
                                dev);
                goto error;
        }
        fd = open(disk->devpath, O_RDWR);
        if (fd < 0) {
                ALOGE("%s: Failed to open %s: %s",
                                __func__,
                                disk->devpath,
                                strerror(errno));
                goto error;
        }
        disk->pentry_arr = gpt_get_pentry_arr(disk->hdr, fd);
        if (!disk->pentry_arr) {
                ALOGE("%s: Failed to obtain partition entry array",
                                __func__);
                goto error;
        }
        disk->pentry_arr_bak = gpt_get_pentry_arr(disk->hdr_bak, fd);
        if (!disk->pentry_arr_bak) {
                ALOGE("%s: Failed to obtain backup partition entry array",
                                __func__);
                goto error;
        }
        disk->pentry_size = GET_4_BYTES(disk->hdr + PENTRY_SIZE_OFFSET);
        disk->pentry_arr_size =
                GET_4_BYTES(disk->hdr + PARTITION_COUNT_OFFSET) *
                disk->pentry_size;
        disk->pentry_arr_crc = GET_4_BYTES(disk->hdr + PARTITION_CRC_OFFSET);
        disk->pentry_arr_bak_crc = GET_4_BYTES(disk->hdr_bak +
                        PARTITION_CRC_OFFSET);
        disk->block_size = gpt_get_block_size(fd);
        close(fd);
        disk->is_initialized = GPT_DISK_INIT_MAGIC;
        return 0;
error:
        if (fd >= 0)
                close(fd);
        return -1;
}

//Get pointer to partition entry from a allocated gpt_disk structure
uint8_t* gpt_disk_get_pentry(struct gpt_disk *disk,
                const char *partname,
                enum gpt_instance instance)
{
        uint8_t *ptn_arr = NULL;
        if (!disk || !partname || disk->is_initialized != GPT_DISK_INIT_MAGIC) {
                ALOGE("%s: Invalid argument",__func__);
                goto error;
        }
        ptn_arr = (instance == PRIMARY_GPT) ?
                disk->pentry_arr : disk->pentry_arr_bak;
        return (gpt_pentry_seek(partname, ptn_arr,
                        ptn_arr + disk->pentry_arr_size ,
                        disk->pentry_size));
error:
        return NULL;
}

//Update CRC values for the various components of the gpt_disk
//structure. This function should be called after any of the fields
//have been updated before the structure contents are written back to
//disk.
int gpt_disk_update_crc(struct gpt_disk *disk)
{
        uint32_t gpt_header_size = 0;
        uint32_t crc_zero;
        crc_zero = crc32(0L, Z_NULL, 0);
        if (!disk || (disk->is_initialized != GPT_DISK_INIT_MAGIC)) {
                ALOGE("%s: invalid argument", __func__);
                goto error;
        }
        //Recalculate the CRC of the primary partiton array
        disk->pentry_arr_crc = crc32(crc_zero,
                        disk->pentry_arr,
                        disk->pentry_arr_size);
        //Recalculate the CRC of the backup partition array
        disk->pentry_arr_bak_crc = crc32(crc_zero,
                        disk->pentry_arr_bak,
                        disk->pentry_arr_size);
        //Update the partition CRC value in the primary GPT header
        PUT_4_BYTES(disk->hdr + PARTITION_CRC_OFFSET, disk->pentry_arr_crc);
        //Update the partition CRC value in the backup GPT header
        PUT_4_BYTES(disk->hdr_bak + PARTITION_CRC_OFFSET,
                        disk->pentry_arr_bak_crc);
        //Update the CRC value of the primary header
        gpt_header_size = GET_4_BYTES(disk->hdr + HEADER_SIZE_OFFSET);
        //Header CRC is calculated with its own CRC field set to 0
        PUT_4_BYTES(disk->hdr + HEADER_CRC_OFFSET, 0);
        PUT_4_BYTES(disk->hdr_bak + HEADER_CRC_OFFSET, 0);
        disk->hdr_crc = crc32(crc_zero, disk->hdr, gpt_header_size);
        disk->hdr_bak_crc = crc32(crc_zero, disk->hdr_bak, gpt_header_size);
        PUT_4_BYTES(disk->hdr + HEADER_CRC_OFFSET, disk->hdr_crc);
        PUT_4_BYTES(disk->hdr_bak + HEADER_CRC_OFFSET, disk->hdr_bak_crc);
        return 0;
error:
        return -1;
}

//Write the contents of struct gpt_disk back to the actual disk
int gpt_disk_commit(struct gpt_disk *disk)
{
        int fd = -1;
        if (!disk || (disk->is_initialized != GPT_DISK_INIT_MAGIC)){
                ALOGE("%s: Invalid args", __func__);
                goto error;
        }
        fd = open(disk->devpath, O_RDWR | O_DSYNC);
        if (fd < 0) {
                ALOGE("%s: Failed to open %s: %s",
                                __func__,
                                disk->devpath,
                                strerror(errno));
                goto error;
        }
        //Write the primary header
        if(gpt_set_header(disk->hdr, fd, PRIMARY_GPT) != 0) {
                ALOGE("%s: Failed to update primary GPT header",
                                __func__);
                goto error;
        }
        //Write back the primary partition array
        if (gpt_set_pentry_arr(disk->hdr, fd, disk->pentry_arr)) {
                ALOGE("%s: Failed to write primary GPT partition arr",
                                __func__);
                goto error;
        }
        //Write back the secondary header
        if(gpt_set_header(disk->hdr_bak, fd, SECONDARY_GPT) != 0) {
                ALOGE("%s: Failed to update secondary GPT header",
                                __func__);
                goto error;
        }
        //Write back the secondary partition array
        if (gpt_set_pentry_arr(disk->hdr_bak, fd, disk->pentry_arr_bak)) {
                ALOGE("%s: Failed to write secondary GPT partition arr",
                                __func__);
                goto error;
        }
        fsync(fd);
        close(fd);
        return 0;
error:
        if (fd >= 0)
                close(fd);
        return -1;
}
