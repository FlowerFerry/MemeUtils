
#ifndef MMUTILS_HARD_DISK_H_INCLUDED
#define MMUTILS_HARD_DISK_H_INCLUDED

#include <meme/string.h>
#include <meme/unsafe/string_view.h>
#include <mego/predef/os/linux.h>
#include <mego/predef/os/windows.h>
#include <mego/util/get_exec_path.h>
#include <mego/util/posix/sys/stat.h>

#include <stdio.h>

#if MEGO_OS__WINDOWS__AVAILABLE
#include <winioctl.h>
#elif MEGO_OS__LINUX__AVAILABLE
#include <sys/statvfs.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif // __cppplusplus

struct mmu_harddisk_freespace
{
    uint32_t st_size;
    uint32_t load;
    uint64_t total;
    uint64_t free;
    uint64_t avail;
};

mmsstk_t mmu_get_harddisk_path_by_path(const char* _filepath, size_t _len);

mmsstk_t mmu_get_harddisk_mountpoint_by_path(const char* _filepath, size_t _len);

int mmu_get_harddisk_freespace_by_path(
    const char* _filepath, size_t _len, mmu_harddisk_freespace* _freespace);


inline mmsstk_t mmu_get_harddisk_path_by_path(const char* _filepath, size_t _len)
{
    mmsstk_t s;
#if MEGO_OS__LINUX__AVAILABLE
    struct mgu_stat file_stat;

    mmsstk_init(&s, MMS__OBJECT_SIZE);

    if (mgu_get_stat(_filepath, _len, &file_stat) != 0)
        return s;
    
    FILE* fp = fopen("/proc/mounts", "r");
    if (fp == NULL)
        return s;

    char line[PATH_MAX * 2] = { 0 };
    char device[PATH_MAX];
    char mount_point[PATH_MAX];
    char fs_type[PATH_MAX];
    char options[PATH_MAX];
    int dump, pass;
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (sscanf(line, "%s %s %s %s %d %d", 
            device, 
            mount_point, 
            fs_type,
            options, 
            &dump, &pass) != 6)
            continue;
        struct mgu_stat mount_stat;
        if (mgu_get_stat(mount_point, strlen(mount_point), &mount_stat) != 0)
            continue;
        if (file_stat.st_dev == mount_stat.st_dev)
        {
            mms_assign_by_utf8((mms_t)&s, 
                (const uint8_t*)device, strlen(device));
            break;
        }
    }
    fclose(fp);

    return s;
#elif MEGO_OS__WINDOWS__AVAILABLE
    mmsstk_t path;
    MemeInteger_t pos = -1;
    // The format for the drive name is "\\.\X:"
    char driveName[16] = { 0 };
    char physicalDrivePath[64] = { 0 };
    mmsstk_init(&s, MMS__OBJECT_SIZE);
    MemeStringViewUnsafeStack_init(&path, MMS__OBJECT_SIZE, (const uint8_t*)_filepath, _len);
    pos = MemeString_indexOfWithUtf8bytes(
        (mms_t)&path, 0, (const uint8_t*)":", -1, MemeFlag_AllSensitive);
    if (pos == -1) {
        mmsstk_uninit(&path, MMS__OBJECT_SIZE);
        return s;
    }
    else {
        char temp[8] = { 0 };
        strncpy_s(temp, sizeof(temp), _filepath, pos);
        sprintf_s(driveName, sizeof(driveName), "\\\\.\\%s:", temp);
    }

    // Open a handle to the drive
    HANDLE hDrive = CreateFileA(
        driveName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, 
        NULL, OPEN_EXISTING, 0, NULL);
    if (hDrive == INVALID_HANDLE_VALUE) {
        mmsstk_uninit(&path, MMS__OBJECT_SIZE);
        return s;
    }

    // Get the device number for the drive
    STORAGE_DEVICE_NUMBER deviceNumber;
    DWORD bytesReturned;
    if (!DeviceIoControl(
        hDrive, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, 
        &deviceNumber, sizeof(deviceNumber), &bytesReturned, NULL)) {
        CloseHandle(hDrive);
        mmsstk_uninit(&path, MMS__OBJECT_SIZE);
        return s;
    }
    
    // Get the device path for the drive
    sprintf_s(physicalDrivePath, sizeof(physicalDrivePath), 
        "\\\\.\\PhysicalDrive%d", deviceNumber.DeviceNumber);
    CloseHandle(hDrive);

    mms_assign_by_utf8((mms_t)&s, (const uint8_t*)physicalDrivePath, strlen(physicalDrivePath));

    mmsstk_uninit(&path, MMS__OBJECT_SIZE);
    return s;
#else // MEGO_OS__Linux__AVAILABLE
    mmsstk_init(&s, MMS__OBJECT_SIZE);
    return s;
#endif // MEGO_OS__Linux__AVAILABLE
}

inline mmsstk_t mmu_get_harddisk_mountpoint_by_path(const char* _filepath, size_t _len)
{
    mmsstk_t s;
#if MEGO_OS__LINUX__AVAILABLE
    struct mgu_stat file_stat;

    mmsstk_init(&s, MMS__OBJECT_SIZE);

    if (mgu_get_stat(_filepath, _len, &file_stat) != 0)
        return s;

    FILE* fp = fopen("/proc/mounts", "r");
    if (fp == NULL)
        return s;

    char line[PATH_MAX * 2] = { 0 };
    char device[PATH_MAX];
    char mount_point[PATH_MAX];
    char fs_type[PATH_MAX];
    char options[PATH_MAX];
    int dump, pass;
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (sscanf(line, "%s %s %s %s %d %d",
            device, 
            mount_point, 
            fs_type, 
            options,
            &dump, &pass) != 6)
            continue;
        struct mgu_stat mount_stat;
        if (mgu_get_stat(mount_point, strlen(mount_point), &mount_stat) != 0)
            continue;
        if (file_stat.st_dev == mount_stat.st_dev)
        {
            mms_assign_by_utf8((mms_t)&s, 
                (const uint8_t*)mount_point, strlen(mount_point));
            break;
        }
    }
    fclose(fp);

    return s;
#elif MEGO_OS__WINDOWS__AVAILABLE
    mmsstk_t path;
    MemeInteger_t pos = -1;
    // The format for the drive name is "\\.\X:"
    char driveName[16] = { 0 };
    char physicalDrivePath[64] = { 0 };
    mmsstk_init(&s, MMS__OBJECT_SIZE);
    MemeStringViewUnsafeStack_init(&path, MMS__OBJECT_SIZE, (const uint8_t*)_filepath, _len);
    pos = MemeString_indexOfWithUtf8bytes(
        (mms_t)&path, 0, (const uint8_t*)":", -1, MemeFlag_AllSensitive);
    if (pos == -1) {
    }
    else {
        mms_assign_by_utf8((mms_t)&s, (const uint8_t*)_filepath, pos + 1);
    }

    mmsstk_uninit(&path, MMS__OBJECT_SIZE);
    return s;
#else // MEGO_OS__Linux__AVAILABLE
    mmsstk_init(&s, MMS__OBJECT_SIZE);
    return s;
#endif // MEGO_OS__Linux__AVAILABLE
}

inline int mmu_get_harddisk_freespace_by_path(
    const char* _filepath, size_t _len, mmu_harddisk_freespace* _freespace)
{
    mmsstk_t mountpoint = mmu_get_harddisk_mountpoint_by_path(_filepath, _len);
#if MEGO_OS__LINUX__AVAILABLE
    if (MemeString_isEmpty((mms_t)&mountpoint))
    {
        mmsstk_uninit(&mountpoint, MMS__OBJECT_SIZE);
        return -1;
    }
    
    struct statvfs buf;
    if (statvfs(MemeString_cStr((mms_t)&mountpoint), &buf) != 0)
    {
        mmsstk_uninit(&mountpoint, MMS__OBJECT_SIZE);
        return -1;
    }
    
    _freespace->total = buf.f_blocks * buf.f_frsize;
    _freespace->free  = buf.f_bfree * buf.f_frsize;
    _freespace->avail = buf.f_bavail * buf.f_frsize;
    _freespace->load  = _freespace->free * 100 / _freespace->total;
#elif MEGO_OS__WINDOWS__AVAILABLE
    ULARGE_INTEGER freeBytesAvailable;
    ULARGE_INTEGER totalNumberOfBytes;
    ULARGE_INTEGER totalNumberOfFreeBytes;
    if (GetDiskFreeSpaceExA(
        MemeString_cStr((mms_t)&mountpoint),
        &freeBytesAvailable,
        &totalNumberOfBytes,
        &totalNumberOfFreeBytes) == 0)
    {
        mmsstk_uninit(&mountpoint, MMS__OBJECT_SIZE);
        return -1;
    }
    
    _freespace->total = totalNumberOfBytes.QuadPart;
    _freespace->free  = totalNumberOfFreeBytes.QuadPart;
    _freespace->avail = freeBytesAvailable.QuadPart;
    _freespace->load  = _freespace->free * 100 / _freespace->total;

#endif
    mmsstk_uninit(&mountpoint, MMS__OBJECT_SIZE);
    return 0;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !MMUTILS_HARD_DISK_H_INCLUDED
