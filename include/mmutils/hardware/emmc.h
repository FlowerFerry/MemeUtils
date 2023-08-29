
#ifndef MMUTILS_HARDWARE_EMMC_H_INCLUDED
#define MMUTILS_HARDWARE_EMMC_H_INCLUDED

#include <meme/string.h>
#include <mego/predef/os/linux.h>
#include <mego/predef/os/windows.h>

#ifdef __cplusplus
extern "C" {
#endif // __cppplusplus

struct mmu_emmc_info
{
    uint32_t st_size;
    mmsstk_t cid;
    mmsstk_t csd;
    mmsstk_t oemid;
    mmsstk_t name;
    mmsstk_t serial;
    mmsstk_t manfid;
    mmsstk_t date;

    //! "MMC", "SD", "SDIO"
    mmsstk_t type; 
};

static void mmu_emmc_info_init   (struct mmu_emmc_info* _info);
static void mmu_emmc_info_uninit (struct mmu_emmc_info* _info);

static int mmu_get_emmc_info_list(struct mmu_emmc_info* _info, size_t* _size);


static void mmu_emmc_info_init(struct mmu_emmc_info* _info)
{
    mmsstk_init(&_info->cid);
    mmsstk_init(&_info->csd);
    mmsstk_init(&_info->oemid);
    mmsstk_init(&_info->name);
    mmsstk_init(&_info->serial);
    mmsstk_init(&_info->manfid);
    mmsstk_init(&_info->date);
}

static void mmu_emmc_info_uninit(struct mmu_emmc_info* _info)
{
    mmsstk_uninit(&_info->cid);
    mmsstk_uninit(&_info->csd);
    mmsstk_uninit(&_info->oemid);
    mmsstk_uninit(&_info->name);
    mmsstk_uninit(&_info->serial);
    mmsstk_uninit(&_info->manfid);
    mmsstk_uninit(&_info->date);
}

static int mmu_get_emmc_info(const char* _device_name, size_t _slen, struct mmu_emmc_info* _info)
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
    
    mmsstk_assign(&_info->cid, buf, len);

    
    snprintf(path, sizeof(path), "/sys/block/%s/device/csd", name);
    fp = fopen(path, "r");
    if (fp) {
        len = fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
        if (len > 0) {
            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }
            mmsstk_assign(&_info->csd, buf, len);
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
            mmsstk_assign(&_info->oemid, buf, len);
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
            mmsstk_assign(&_info->name, buf, len);
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
            mmsstk_assign(&_info->serial, buf, len);
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
            mmsstk_assign(&_info->manfid, buf, len);
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
            mmsstk_assign(&_info->date, buf, len);
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
            mmsstk_assign(&_info->type, buf, len);
        }
    }

    return 0;
}

static int mmu_get_emmc_info_list(struct mmu_emmc_info* _info, size_t* _size)
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
