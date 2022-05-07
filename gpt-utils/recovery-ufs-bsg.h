#ifndef __RECOVERY_UFS_BSG_H__
#define __RECOVERY_UFS_BSG_H__

/*
 * Copyright (c) 2020 The Linux Foundation. All rights reserved.
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


#include <linux/bsg.h>
#include <scsi/scsi_bsg_ufs.h>
#include <endian.h>
#include <dirent.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#ifdef ANDROID
#include "cutils/log.h"
#endif

#ifdef OE
#include <syslog.h>
#define LOGI(...) syslog(LOG_NOTICE, "INFO:" __VA_ARGS__)
#define LOGV(...) syslog(LOG_NOTICE,"VERB:" __VA_ARGS__)
#define LOGD(...) syslog(LOG_DEBUG,"DBG:" __VA_ARGS__)
#define LOGE(...) syslog(LOG_ERR,"ERR:" __VA_ARGS__)
#define LOGW(...) syslog(LOG_WARNING,"WRN:" __VA_ARGS__)
#define strlcat(d,s,l) snprintf(d+strlen(d),l,"%s",s)
#endif



#define FNAME_SZ 64

#define SG_IO  0x2285

#define DWORD(b3, b2, b1, b0) htobe32((b3 << 24) | (b2 << 16) |\
        (b1 << 8) | b0)

/* UFS BSG device nodes */
char ufs_bsg_dev[FNAME_SZ] = "/dev/ufs-bsg";

int fd_ufs_bsg;

int32_t set_ufs_lun(uint8_t lun_id);

#ifdef _BSG_FRAMEWORK_KERNEL_HEADERS
/* UPIU Transaction Codes */
enum {
    UTP_UPIU_NOP_OUT    = 0x00,
    UTP_UPIU_COMMAND    = 0x01,
    UTP_UPIU_DATA_OUT   = 0x02,
    UTP_UPIU_TASK_REQ   = 0x04,
    UTP_UPIU_QUERY_REQ  = 0x16,
};

/* UPIU Query Function field */
enum {
    QUERY_REQ_FUNC_STD_READ    = 0x01,
    QUERY_REQ_FUNC_STD_WRITE   = 0x81,
};

enum query_req_opcode {
    QUERY_REQ_OP_READ_DESC    = 0x1,
    QUERY_REQ_OP_WRITE_DESC   = 0x2,
    QUERY_REQ_OP_READ_ATTR    = 0x3,
    QUERY_REQ_OP_WRITE_ATTR   = 0x4,
    QUERY_REQ_OP_READ_FLAG    = 0x5,
    QUERY_REQ_OP_SET_FLAG     = 0x6,
    QUERY_REQ_OP_CLEAR_FLAG   = 0x7,
    QUERY_REQ_OP_TOGGLE_FLAG  = 0x8,
};

enum query_desc_idn {
    QUERY_DESC_IDN_DEVICE    = 0x0,
    QUERY_DESC_IDN_UNIT      = 0x2,
    QUERY_DESC_IDN_GEOMETRY  = 0x7,
};

enum query_desc_size {
    QUERY_DESC_SIZE_DEVICE    = 0x40,
    QUERY_DESC_SIZE_GEOMETRY  = 0x48,
    QUERY_DESC_SIZE_UNIT      = 0x23,
};

enum bsg_ioctl_dir {
    BSG_IOCTL_DIR_TO_DEV,
    BSG_IOCTL_DIR_FROM_DEV,
};

enum query_attr_idn {
    QUERY_ATTR_IDN_BOOT_LU_EN        = 0x00,
    QUERY_ATTR_IDN_RESERVED          = 0x01,
    QUERY_ATTR_IDN_POWER_MODE        = 0x02,
    QUERY_ATTR_IDN_ACTIVE_ICC_LVL    = 0x03,
};
#endif  /*  _BSG_FRAMEWORK_KERNEL_HEADERS */

#endif /* __RECOVERY_UFS_BSG_H__ */
