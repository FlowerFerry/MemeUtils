
#ifndef MMUTILS_HARDWARE_MEMORY_DEF_H_INCLUDED
#define MMUTILS_HARDWARE_MEMORY_DEF_H_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cppplusplus

struct mmu_memory_status
{
    uint32_t st_size;
    uint32_t load_physical;
    uint64_t total_physical;
    uint64_t available_physical;
    uint64_t total_virtual;
    uint64_t available_virtual;
};

inline int mmu_get_memory_status(struct mmu_memory_status *status);

#ifdef __cplusplus
}
#endif // __cppplusplus

#endif // !MMUTILS_HARDWARE_MEMORY_DEF_H_INCLUDED
