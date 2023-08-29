
#ifndef MMU_SYNC_OS_LINUX_PROCESS_MUTEX_H_INCLUDED
#define MMU_SYNC_OS_LINUX_PROCESS_MUTEX_H_INCLUDED

#include <mego/predef/os/linux.h>

#ifdef __cplusplus
extern "C" {
#endif

#if MEGO_OS__LINUX__AVAILABLE

typedef struct {
    int handle;
} mmu_pmtx_t;

static int mmu_pmtx__init     (mmu_pmtx_t *_mtx, const char *_name);
static int mmu_pmtx__destroy  (mmu_pmtx_t *_mtx);
static int mmu_pmtx__lock     (mmu_pmtx_t *_mtx);
static int mmu_pmtx__unlock   (mmu_pmtx_t *_mtx);
static int mmu_pmtx__timedlock(mmu_pmtx_t *_mtx, uint64_t _timeout_ms);


#endif // MEGO_OS__LINUX__AVAILABLE

#ifdef __cplusplus
}
#endif


#endif // !MMU_SYNC_OS_LINUX_PROCESS_MUTEX_H_INCLUDED
