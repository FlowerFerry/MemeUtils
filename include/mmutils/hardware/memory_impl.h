
#ifndef MMUTILS_HARDWARE_MEMORY_IMPL_H_INCLUDED
#define MMUTILS_HARDWARE_MEMORY_IMPL_H_INCLUDED

#include <mmutils/hardware/memory_def.h>
#include <mego/util/os/windows/windows_simplify.h>
#include <mego/predef/os/linux.h>

#ifdef __cplusplus
extern "C" {
#endif // __cppplusplus

inline int mmu_get_memory_status(struct mmu_memory_status *status)
{
    #if MEGO_OS__WINDOWS__AVAILABLE
    MEMORYSTATUSEX memstat = { sizeof(MEMORYSTATUSEX), 0 };
    int ret = GlobalMemoryStatusEx(&memstat);
    if (ret == 0)
        return -1;
    
    status->total_physical = memstat.ullTotalPhys;
    status->available_physical = memstat.ullAvailPhys;
    status->total_virtual = memstat.ullTotalVirtual;
    status->available_virtual = memstat.ullAvailVirtual;
    status->load_physical = memstat.dwMemoryLoad;
#elif MEGO_OS__LINUX__AVAILABLE
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL)
        return -1;
    
    char buf[256] = { 0 };
    uint64_t vmalloc_used  = 0;
    uint64_t vmalloc_total = 0;
    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        if (strncmp(buf, "MemTotal:", 9) == 0)
            status->total_physical = strtoull(buf + 9, NULL, 10) * 1024;
        else if (strncmp(buf, "MemAvailable:", 13) == 0)
            status->available_physical = strtoull(buf + 13, NULL, 10) * 1024;
        else if (strncmp(buf, "VmallocTotal:", 13) == 0)
            vmalloc_total = strtoull(buf + 13, NULL, 10) * 1024;
        else if (strncmp(buf, "VmallocUsed:", 12) == 0)
            vmalloc_used = strtoull(buf + 12, NULL, 10) * 1024;
        else if (strncmp(buf, "SwapTotal:", 10) == 0)
            status->total_virtual += strtoull(buf + 10, NULL, 10) * 1024;
        else if (strncmp(buf, "SwapFree:", 9) == 0)
            status->available_virtual += strtoull(buf + 9, NULL, 10) * 1024;
    }
    status->total_virtual += vmalloc_total;
    status->available_virtual += vmalloc_total - vmalloc_used;
    if (status->total_physical == 0)
        status->load_physical = 0;
    else
        status->load_physical = (status->total_physical - status->available_physical) * 100 / status->total_physical;
    fclose(fp);
#else
    #error "<mmu_get_memory_status> not implemented"
#endif // MEGO_OS__WINDOWS__AVAILABLE

    return 0;
}

#ifdef __cplusplus
}
#endif // __cppplusplus

#endif // !MMUTILS_HARDWARE_MEMORY_IMPL_H_INCLUDED
