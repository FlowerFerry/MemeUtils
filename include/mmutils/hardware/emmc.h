
#ifndef MMUTILS_HARDWARE_EMMC_H_INCLUDED
#define MMUTILS_HARDWARE_EMMC_H_INCLUDED

#include <meme/string.h>
#include <mego/predef/os/linux.h>
#include <mego/predef/os/windows.h>
#include <mego/predef/symbol/inline.h>

#ifdef __cplusplus
extern "C" {
#endif // __cppplusplus

struct mmu_emmc_info
{
    uint32_t st_size;
    mmstrstk_t cid;
    mmstrstk_t csd;
    mmstrstk_t oemid;
    mmstrstk_t name;
    mmstrstk_t serial;
    mmstrstk_t manfid;
    mmstrstk_t date;

    //! "MMC", "SD", "SDIO"
    mmstrstk_t type; 
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

MG_CAPI_INLINE int mmu_get_emmc_info(const char* _device_name, size_t _slen, struct mmu_emmc_info* _info)
{
#if MEGO_OS__LINUX__AVAILABLE
    char name[128] = { 0 };
    char path[PATH_MAX];
    char buf[512];
    size_t len = 0;
    FILE* fp = NULL;
    
    strncpy (name, _device_name, _slen);
    snprintf(path, sizeof(path), "/sys/block/%s/device/cid", name);
    fp = fopen(path, "r");
    if (fp == NULL) {
        return -1;
    }
    
    len = fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    if (len == 0) {
        return -1;
    }
    
    // remove '\n'
    if (buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    }
    
    mmstrstk_assign(&_info->cid, buf, len);

    
    snprintf(path, sizeof(path), "/sys/block/%s/device/csd", name);
    fp = fopen(path, "r");
    if (fp) {
        len = fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
        if (len > 0) {
            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }
            mmstrstk_assign(&_info->csd, buf, len);
        }
    }
    
    snprintf(path, sizeof(path), "/sys/block/%s/device/oemid", name);
    fp = fopen(path, "r");
    if (fp) {
        len = fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
        if (len > 0) {
            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }
            mmstrstk_assign(&_info->oemid, buf, len);
        }
    }

    snprintf(path, sizeof(path), "/sys/block/%s/device/name", name);
    fp = fopen(path, "r");
    if (fp) {
        len = fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
        if (len > 0) {
            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }
            mmstrstk_assign(&_info->name, buf, len);
        }
    }

    snprintf(path, sizeof(path), "/sys/block/%s/device/serial", name);
    fp = fopen(path, "r");
    if (fp) {
        len = fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
        if (len > 0) {
            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }
            mmstrstk_assign(&_info->serial, buf, len);
        }
    }

    snprintf(path, sizeof(path), "/sys/block/%s/device/manfid", name);
    fp = fopen(path, "r");
    if (fp) {
        len = fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
        if (len > 0) {
            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }
            mmstrstk_assign(&_info->manfid, buf, len);
        }
    }

    snprintf(path, sizeof(path), "/sys/block/%s/device/date", name);
    fp = fopen(path, "r");
    if (fp) {
        len = fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
        if (len > 0) {
            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }
            mmstrstk_assign(&_info->date, buf, len);
        }
    }

    snprintf(path, sizeof(path), "/sys/block/%s/device/type", name);
    fp = fopen(path, "r");
    if (fp) {
        len = fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
        if (len > 0) {
            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }
            mmstrstk_assign(&_info->type, buf, len);
        }
    }

#endif
    return 0;
}

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
