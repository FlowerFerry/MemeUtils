
#ifndef MMUTILS_HARDWARE_EMMC_H_INCLUDED
#define MMUTILS_HARDWARE_EMMC_H_INCLUDED

#include <meme/string.h>
#include <mego/predef/os/linux.h>
#include <mego/predef/os/windows.h>
#include <mego/predef/symbol/inline.h>
#include <mego/util/math.h>

#ifdef __cplusplus
extern "C" {
#endif // __cppplusplus

//! @struct mmu_emmc_info
//! @brief 用于存储 eMMC 信息的数据结构。
struct mmu_emmc_info
{
    uint32_t st_size;
    mmstrstk_t cid;

    //! @var csd
    //! @brief 卡片特性描述符 (Card-Specific Data)。
    mmstrstk_t csd;
    mmstrstk_t oemid;
    mmstrstk_t name;
    mmstrstk_t serial;
    mmstrstk_t manfid;
    mmstrstk_t date;

    //! @var type
    //! @brief 卡片类型，例如 "MMC", "SD", "SDIO"。
    mmstrstk_t type; 

    //! @var removable
    //! @brief 是否是可移动设备，-1 表示未知，0 表示不可移动，1 表示可移动。
    int8_t removable;
};

MG_CAPI_INLINE void mmu_emmc_info_init(struct mmu_emmc_info* _info)
{
    mmstrstk_init(&_info->cid);
    mmstrstk_init(&_info->csd);
    mmstrstk_init(&_info->oemid);
    mmstrstk_init(&_info->name);
    mmstrstk_init(&_info->serial);
    mmstrstk_init(&_info->manfid);
    mmstrstk_init(&_info->date);

    _info->removable = -1;
}

MG_CAPI_INLINE void mmu_emmc_info_uninit(struct mmu_emmc_info* _info)
{
    mmstrstk_uninit(&_info->cid);
    mmstrstk_uninit(&_info->csd);
    mmstrstk_uninit(&_info->oemid);
    mmstrstk_uninit(&_info->name);
    mmstrstk_uninit(&_info->serial);
    mmstrstk_uninit(&_info->manfid);
    mmstrstk_uninit(&_info->date);
}

//! @brief 获取指定设备的 eMMC 信息。
//!
//! @param _device_name 设备名称字符串。
//! @param _slen 设备名称字符串的长度。
//! @param _info 指向要填充的 mmu_emmc_info 结构体的指针。
//! @return 返回操作结果，0 表示成功，非 0 表示失败。
MG_CAPI_INLINE int mmu_get_emmc_info(const char* _device_name, size_t _slen, struct mmu_emmc_info* _info)
{
#if MEGO_OS__LINUX__AVAILABLE
    char name[128];
    char path[PATH_MAX];
    char buf[512];
    size_t len = 0;
    FILE* fp = NULL;
    
    strncpy (name, _device_name, MGU_MATH__MIN(_slen, sizeof(name) - 1));
    name[sizeof(name) - 1] = '\0';
    
    snprintf(path, sizeof(path), "/sys/block/%s/device/cid", name);
    fp = fopen(path, "r");
    if (fp == NULL) {
        return -1;
    }
    
    len = fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    if (len == 0 || len >= sizeof(buf)) {
        return -1;
    }
    
    // remove '\n'
    if (buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    }
    
    mmstrstk_assign_by_utf8(&_info->cid, buf, len);

    
    snprintf(path, sizeof(path), "/sys/block/%s/device/csd", name);
    fp = fopen(path, "r");
    if (fp) {
        len = fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
        if (len > 0 && len < sizeof(buf)) {
            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }
            mmstrstk_assign_by_utf8(&_info->csd, buf, len);
        }
    }
    
    snprintf(path, sizeof(path), "/sys/block/%s/device/oemid", name);
    fp = fopen(path, "r");
    if (fp) {
        len = fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
        if (len > 0 && len < sizeof(buf)) {
            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }
            mmstrstk_assign_by_utf8(&_info->oemid, buf, len);
        }
    }

    snprintf(path, sizeof(path), "/sys/block/%s/device/name", name);
    fp = fopen(path, "r");
    if (fp) {
        len = fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
        if (len > 0 && len < sizeof(buf)) {
            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }
            mmstrstk_assign_by_utf8(&_info->name, buf, len);
        }
    }

    snprintf(path, sizeof(path), "/sys/block/%s/device/serial", name);
    fp = fopen(path, "r");
    if (fp) {
        len = fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
        if (len > 0 && len < sizeof(buf)) {
            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }
            mmstrstk_assign_by_utf8(&_info->serial, buf, len);
        }
    }

    snprintf(path, sizeof(path), "/sys/block/%s/device/manfid", name);
    fp = fopen(path, "r");
    if (fp) {
        len = fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
        if (len > 0 && len < sizeof(buf)) {
            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }
            mmstrstk_assign_by_utf8(&_info->manfid, buf, len);
        }
    }

    snprintf(path, sizeof(path), "/sys/block/%s/device/date", name);
    fp = fopen(path, "r");
    if (fp) {
        len = fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
        if (len > 0 && len < sizeof(buf)) {
            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }
            mmstrstk_assign_by_utf8(&_info->date, buf, len);
        }
    }

    snprintf(path, sizeof(path), "/sys/block/%s/device/type", name);
    fp = fopen(path, "r");
    if (fp) {
        len = fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
        if (len > 0 && len < sizeof(buf)) {
            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }
            mmstrstk_assign_by_utf8(&_info->type, buf, len);
        }
    }

#endif
    return -1;
}

//! @brief 获取系统中所有 eMMC 设备的信息列表。
//!
//! @param _info 指向 mmu_emmc_info 结构体数组的指针，用于存储获取到的信息。
//! @param _size 指向大小变量的指针，用于存储数组大小。
//! @return 返回操作结果，0 表示成功，非 0 表示失败。
MG_CAPI_INLINE int mmu_get_emmc_info_list(struct mmu_emmc_info* _info, size_t* _size)
{
#if MEGO_OS__LINUX__AVAILABLE
    struct mmu_emmc_info* info = _info;
    struct dirent* ent = NULL;
    DIR* dir = opendir("/sys/class/block");
    if (dir == NULL)
    {
        return -1;
    }

    while ((ent = readdir(dir)) != NULL)
    {
        if (ent->d_type != DT_LNK)
        {
            continue;
        }

        if (strncmp(ent->d_name, "mmcblk", 6) != 0)
        {
            continue;
        }

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "/sys/class/block/%s/device/cid", ent->d_name);

        struct stat st;
        if (stat(path, &st) != 0)
        {
            continue;
        }

        if (!info) {
            if (_size)
                ++(*_size);
        }
        else {
            mmu_get_emmc_info(ent->d_name, strlen(ent->d_name), info);
            ++info;
        }
    }
    
    closedir(dir);
    return 0;
#endif 
    return -1;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !MMUTILS_HARDWARE_EMMC_H_INCLUDED
