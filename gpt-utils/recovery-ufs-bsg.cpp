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


#define LOG_TAG "recovery_ufs"

#include "recovery-ufs-bsg.h"

#ifndef _BSG_FRAMEWORK_KERNEL_HEADERS
#ifndef _GENERIC_KERNEL_HEADERS
#include <scsi/ufs/ioctl.h>
#include <scsi/ufs/ufs.h>
#endif
#endif

//Size of the buffer that needs to be passed to the UFS ioctl
#define UFS_ATTR_DATA_SIZE          32

#ifdef _BSG_FRAMEWORK_KERNEL_HEADERS
static int get_ufs_bsg_dev(void)
{
    DIR *dir;
    struct dirent *ent;
    int ret = -ENODEV;

    if ((dir = opendir ("/dev")) != NULL) {
        /* read all the files and directories within directory */
        while ((ent = readdir(dir)) != NULL) {
            if (!strcmp(ent->d_name, "ufs-bsg") ||
                    !strcmp(ent->d_name, "ufs-bsg0")) {
                snprintf(ufs_bsg_dev, FNAME_SZ, "/dev/%s", ent->d_name);
                ret = 0;
                break;
            }
        }
        if (ret)
            ALOGE("could not find the ufs-bsg dev\n");
        closedir (dir);
    } else {
        /* could not open directory */
        ALOGE("could not open /dev (error no: %d)\n", errno);
        ret = -EINVAL;
    }

    return ret;
}

int ufs_bsg_dev_open(void)
{
    int ret;
    if (!fd_ufs_bsg) {
        fd_ufs_bsg = open(ufs_bsg_dev, O_RDWR);
        ret = errno;
        if (fd_ufs_bsg < 0) {
            ALOGE("Unable to open %s (error no: %d)",
                    ufs_bsg_dev, errno);
            fd_ufs_bsg = 0;
            return ret;
        }
    }
    return 0;
}

void ufs_bsg_dev_close(void)
{
    if (fd_ufs_bsg) {
        close(fd_ufs_bsg);
        fd_ufs_bsg = 0;
    }
}

static int ufs_bsg_ioctl(int fd, struct ufs_bsg_request *req,
        struct ufs_bsg_reply *rsp, __u8 *buf, __u32 buf_len,
        enum bsg_ioctl_dir dir)
{
    int ret;
    struct sg_io_v4 sg_io = {0};

    sg_io.guard = 'Q';
    sg_io.protocol = BSG_PROTOCOL_SCSI;
    sg_io.subprotocol = BSG_SUB_PROTOCOL_SCSI_TRANSPORT;
    sg_io.request_len = sizeof(*req);
    sg_io.request = (__u64)req;
    sg_io.response = (__u64)rsp;
    sg_io.max_response_len = sizeof(*rsp);
    if (dir == BSG_IOCTL_DIR_FROM_DEV) {
        sg_io.din_xfer_len = buf_len;
        sg_io.din_xferp = (__u64)(buf);
    } else {
        sg_io.dout_xfer_len = buf_len;
        sg_io.dout_xferp = (__u64)(buf);
    }

    ret = ioctl(fd, SG_IO, &sg_io);
    if (ret)
        ALOGE("%s: Error from sg_io ioctl (return value: %d, error no: %d, reply result from LLD: %d\n)",
                __func__, ret, errno, rsp->result);

    if (sg_io.info || rsp->result) {
        ALOGE("%s: Error from sg_io info (check sg info: device_status: 0x%x, transport_status: 0x%x, driver_status: 0x%x, reply result from LLD: %d\n)",
                __func__, sg_io.device_status, sg_io.transport_status,
                sg_io.driver_status, rsp->result);
        ret = -EAGAIN;
    }

    return ret;
}

static void compose_ufs_bsg_query_req(struct ufs_bsg_request *req, __u8 func,
        __u8 opcode, __u8 idn, __u8 index, __u8 sel,
        __u16 length)
{
    struct utp_upiu_header *hdr = &req->upiu_req.header;
    struct utp_upiu_query *qr = &req->upiu_req.qr;

    req->msgcode = UTP_UPIU_QUERY_REQ;
    hdr->dword_0 = DWORD(UTP_UPIU_QUERY_REQ, 0, 0, 0);
    hdr->dword_1 = DWORD(0, func, 0, 0);
    hdr->dword_2 = DWORD(0, 0, length >> 8, (__u8)length);
    qr->opcode = opcode;
    qr->idn = idn;
    qr->index = index;
    qr->selector = sel;
    qr->length = htobe16(length);
}


static int ufs_query_attr(int fd, __u32 value,
        __u8 func, __u8 opcode, __u8 idn,
        __u8 index, __u8 sel)
{
    struct ufs_bsg_request req = {0};
    struct ufs_bsg_reply rsp = {0};
    enum bsg_ioctl_dir dir = BSG_IOCTL_DIR_FROM_DEV;
    int ret = 0;

    if (opcode == QUERY_REQ_OP_WRITE_DESC || opcode == QUERY_REQ_OP_WRITE_ATTR)
        dir = BSG_IOCTL_DIR_TO_DEV;

    req.upiu_req.qr.value = htobe32(value);

    compose_ufs_bsg_query_req(&req, func, opcode, idn, index, sel, 0);

    ret = ufs_bsg_ioctl(fd, &req, &rsp, 0, 0, dir);
    if (ret)
        ALOGE("%s: Error from ufs_bsg_ioctl (return value: %d, error no: %d\n)",
                __func__, ret, errno);

    return ret;
}

int32_t set_boot_lun(char *sg_dev,uint8_t lun_id)
{
    int32_t ret;
    __u32 boot_lun_id  = lun_id;

    ret = get_ufs_bsg_dev();
    if (ret)
        return ret;
    ALOGV("Found the ufs bsg dev: %s\n", ufs_bsg_dev);

    ret = ufs_bsg_dev_open();
    if (ret)
        return ret;
    ALOGV("Opened ufs bsg dev: %s\n", ufs_bsg_dev);

    ret = ufs_query_attr(fd_ufs_bsg, boot_lun_id, QUERY_REQ_FUNC_STD_WRITE,
            QUERY_REQ_OP_WRITE_ATTR, QUERY_ATTR_IDN_BOOT_LU_EN, 0, 0);
    if (ret) {
        ALOGE("Error requesting ufs attr idn %d via query ioctl (return value: %d, error no: %d)",
                QUERY_ATTR_IDN_BOOT_LU_EN, ret, errno);
        goto out;
    }
out:
    ufs_bsg_dev_close();
    return ret;
}
#endif

#ifndef _BSG_FRAMEWORK_KERNEL_HEADERS
int32_t set_boot_lun(char *sg_dev, uint8_t boot_lun_id)
{
#ifndef _GENERIC_KERNEL_HEADERS
        int fd = -1;
        int rc;
        struct ufs_ioctl_query_data *data = NULL;
        size_t ioctl_data_size = sizeof(struct ufs_ioctl_query_data) + UFS_ATTR_DATA_SIZE;

        data = (struct ufs_ioctl_query_data*)malloc(ioctl_data_size);
        if (!data) {
                fprintf(stderr, "%s: Failed to alloc query data struct\n",
                                __func__);
                goto error;
        }
        memset(data, 0, ioctl_data_size);
        data->opcode = UPIU_QUERY_OPCODE_WRITE_ATTR;
        data->idn = QUERY_ATTR_IDN_BOOT_LU_EN;
        data->buf_size = UFS_ATTR_DATA_SIZE;
        data->buffer[0] = boot_lun_id;
        fd = open(sg_dev, O_RDWR);
        if (fd < 0) {
                fprintf(stderr, "%s: Failed to open %s(%s)\n",
                                __func__,
                                sg_dev,
                                strerror(errno));
                goto error;
        }
        rc = ioctl(fd, UFS_IOCTL_QUERY, data);
        if (rc) {
                fprintf(stderr, "%s: UFS query ioctl failed(%s)\n",
                                __func__,
                                strerror(errno));
                goto error;
        }
        close(fd);
        free(data);
        return 0;
error:
        if (fd >= 0)
                close(fd);
        if (data)
                free(data);
        return -1;
#else
        return 0;
#endif
}
#endif

