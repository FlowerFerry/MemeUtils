
#ifndef MMUHW_DISK_SERIAL_NUMBER_H_INCLUDED
#define MMUHW_DISK_SERIAL_NUMBER_H_INCLUDED

#include <mego/predef/os/linux.h>

#include <stdint.h>

#if MG_OS__LINUX_AVAIL
#include <fcntl.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>
#endif // !MG_OS__LINUX_AVAIL

#ifndef __cplusplus
extern "C" {
#endif // __cplusplus

mmsstk_t mmuhw_get_disk_serial_number(const char* _disk_path, size_t _len);


inline mmsstk_t mmuhw_get_disk_serial_number(const char* _disk_path, size_t _len)
{
    mmsstk_t s;
#if MG_OS__LINUX_AVAIL
    int reply_len = 0;
    uint8_t* reply_ptr = NULL;
    uint8_t* index_ptr = NULL;
    int fd = open(_disk_path, O_RDONLY);
    if (fd < 0) {
        mmsstk_init(&s, MMSTR__OBJ_SIZE);
        return s; 
    }

    uint8_t sense_buffer[32] = { 0 };
    uint8_t inquiry_buffer[255] = { 0 };
    uint8_t inq_cmd[] = { INQUIRY, 0x01, 0x80, 0, sizeof(inquiry_buffer), 0 };
    struct sg_io_hdr io_hdr;
    int result;

    memset(&io_hdr, 0, sizeof(struct sg_io_hdr));

    io_hdr.interface_id = 'S';
    io_hdr.cmdp = inq_cmd;
    io_hdr.cmd_len = sizeof(inq_cmd);
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxferp = inquiry_buffer;
    io_hdr.dxfer_len = sizeof(inquiry_buffer);
    io_hdr.sbp = sense_buffer;
    io_hdr.mx_sb_len = sizeof(sense_buffer);
    io_hdr.timeout = 5000;

    result = ioctl(fd, SG_IO, &io_hdr);
    close(fd);

    if (result < 0) {
        mmsstk_init(&s, MMSTR__OBJ_SIZE);
        return s; 
    }

    if ((io_hdr.info & SG_INFO_OK_MASK) != SG_INFO_OK) 
    {
        mmsstk_init(&s, MMSTR__OBJ_SIZE);
        return s; 
    }

    // if (io_hdr.masked_status) {
    //     mmsstk_init(&s, MMSTR__OBJ_SIZE);
    //     return s; 
    // }

    // if (io_hdr.host_status) {
    //     mmsstk_init(&s, MMSTR__OBJ_SIZE);
    //     return s; 
    // }

    // if (io_hdr.driver_status) {
    //     mmsstk_init(&s, MMSTR__OBJ_SIZE);
    //     return s; 
    // }

    reply_len = inquiry_buffer[3];
    reply_ptr = inquiry_buffer + 4;
    index_ptr = reply_ptr;
    for (int i = 0; i < reply_len; ++i)
    {
        if (reply_ptr[i] > 0x20)
        {
            if (reply_ptr[i] == ':')
                *index_ptr++ = ';';
            else
                *index_ptr++ = reply_ptr[i];
        }
    }
    reply_len = index_ptr - reply_ptr;
    index_ptr = reply_ptr;
    if (reply_len > MAX_RAID_SERIAL_LEN)
    {
        index_ptr += reply_len - MAX_RAID_SERIAL_LEN;
        reply_len  = MAX_RAID_SERIAL_LEN;
    }

    MemeStringStack_initByU8bytes(&s, MMSTR__OBJ_SIZE, index_ptr, reply_len);
#endif // !MG_OS__LINUX_AVAIL
    return s;
}

#ifndef __cplusplus
}
#endif // __cplusplus

#endif // !MMUHW_DISK_SERIAL_NUMBER_H_INCLUDED
