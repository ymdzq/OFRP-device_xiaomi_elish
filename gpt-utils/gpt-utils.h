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

#ifndef __GPT_UTILS_H__
#define __GPT_UTILS_H__
#include <vector>
#include <string>
#include <map>
#ifdef __cplusplus
extern "C" {
#endif
#include <unistd.h>
#include <stdlib.h>
/******************************************************************************
 * GPT HEADER DEFINES
 ******************************************************************************/
#define GPT_SIGNATURE               "EFI PART"
#define HEADER_SIZE_OFFSET          12
#define HEADER_CRC_OFFSET           16
#define PRIMARY_HEADER_OFFSET       24
#define BACKUP_HEADER_OFFSET        32
#define FIRST_USABLE_LBA_OFFSET     40
#define LAST_USABLE_LBA_OFFSET      48
#define PENTRIES_OFFSET             72
#define PARTITION_COUNT_OFFSET      80
#define PENTRY_SIZE_OFFSET          84
#define PARTITION_CRC_OFFSET        88

#define TYPE_GUID_OFFSET            0
#define TYPE_GUID_SIZE              16
#define PTN_ENTRY_SIZE              128
#define UNIQUE_GUID_OFFSET          16
#define FIRST_LBA_OFFSET            32
#define LAST_LBA_OFFSET             40
#define ATTRIBUTE_FLAG_OFFSET       48
#define PARTITION_NAME_OFFSET       56
#define MAX_GPT_NAME_SIZE           72

/******************************************************************************
 * AB RELATED DEFINES
 ******************************************************************************/
//Bit 48 onwords in the attribute field are the ones where we are allowed to
//store our AB attributes.
#define AB_FLAG_OFFSET (ATTRIBUTE_FLAG_OFFSET + 6)
#define GPT_DISK_INIT_MAGIC 0xABCD
#define AB_PARTITION_ATTR_SLOT_ACTIVE (0x1<<2)
#define AB_PARTITION_ATTR_BOOT_SUCCESSFUL (0x1<<6)
#define AB_PARTITION_ATTR_UNBOOTABLE (0x1<<7)
#define AB_SLOT_ACTIVE_VAL              0x3F
#define AB_SLOT_INACTIVE_VAL            0x0
#define AB_SLOT_ACTIVE                  1
#define AB_SLOT_INACTIVE                0
#define AB_SLOT_A_SUFFIX                "_a"
#define AB_SLOT_B_SUFFIX                "_b"
#define PTN_XBL                         "xbl"
#define PTN_XBL_CFG                     "xbl_config"
#define PTN_MULTIIMGOEM                 "multiimgoem"
#define PTN_MULTIIMGQTI                 "multiimgqti"
#define PTN_SWAP_LIST                   PTN_XBL, PTN_XBL_CFG, "sbl1", "rpm", "tz", "aboot", "abl", "hyp", "lksecapp", "keymaster", "cmnlib", "cmnlib32", "cmnlib64", "pmic", "apdp", "devcfg", "hosd", "keystore", "msadp", "mdtp", "mdtpsecapp", "dsp", "aop", "qupfw", "vbmeta", "dtbo", "imagefv", "ImageFv", "multiimgoem", "multiimgqti", "vm-bootsys", "shrm", "cpucp"
#define AB_PTN_LIST PTN_SWAP_LIST, "boot", "system", "vendor", "odm", "modem", "bluetooth", "system_ext", "vendor_boot", "product"
#define BOOT_DEV_DIR    "/dev/block/bootdevice/by-name"

/******************************************************************************
 * HELPER MACROS
 ******************************************************************************/
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
/******************************************************************************
 * TYPES
 ******************************************************************************/
enum boot_update_stage {
	UPDATE_MAIN = 1,
	UPDATE_BACKUP,
	UPDATE_FINALIZE
};

enum gpt_instance {
	PRIMARY_GPT = 0,
	SECONDARY_GPT
};

enum boot_chain {
	NORMAL_BOOT = 0,
	BACKUP_BOOT
};

struct gpt_disk {
	//GPT primary header
	uint8_t *hdr;
	//primary header crc
	uint32_t hdr_crc;
	//GPT backup header
	uint8_t *hdr_bak;
	//backup header crc
	uint32_t hdr_bak_crc;
	//Partition entries array
	uint8_t *pentry_arr;
	//Partition entries array for backup table
	uint8_t *pentry_arr_bak;
	//Size of the pentry array
	uint32_t pentry_arr_size;
	//Size of each element in the pentry array
	uint32_t pentry_size;
	//CRC of the partition entry array
	uint32_t pentry_arr_crc;
	//CRC of the backup partition entry array
	uint32_t pentry_arr_bak_crc;
	//Path to block dev representing the disk
	char devpath[PATH_MAX];
	//Block size of disk
	uint32_t block_size;
	uint32_t is_initialized;
};

/******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************/
int prepare_boot_update(enum boot_update_stage stage);
//GPT disk methods
struct gpt_disk* gpt_disk_alloc();
//Free previously allocated gpt_disk struct
void gpt_disk_free(struct gpt_disk *disk);
//Get the details of the disk holding the partition whose name
//is passed in via dev
int gpt_disk_get_disk_info(const char *dev, struct gpt_disk *disk);

//Get pointer to partition entry from a allocated gpt_disk structure
uint8_t* gpt_disk_get_pentry(struct gpt_disk *disk,
		const char *partname,
		enum gpt_instance instance);

//Update the crc fields of the modified disk structure
int gpt_disk_update_crc(struct gpt_disk *disk);

//Write the contents of struct gpt_disk back to the actual disk
int gpt_disk_commit(struct gpt_disk *disk);

//Return if the current device is UFS based or not
int gpt_utils_is_ufs_device();

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
int gpt_utils_set_xbl_boot_partition(enum boot_chain chain);

//Given a vector of partition names as a input and a reference to a map,
//populate the map to indicate which physical disk each of the partitions
//sits on. The key in the map is the path to the block device where the
//partiton lies and the value is a vector of strings indicating which of
//the passed in partiton names sits on that device.
int gpt_utils_get_partition_map(std::vector<std::string>& partition_list,
                std::map<std::string,std::vector<std::string>>& partition_map);
#ifdef __cplusplus
}
#endif
#endif /* __GPT_UTILS_H__ */
