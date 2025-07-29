
#ifndef MMU_SYNC_OS_WINDOWS_PROCESS_MUTEX_H_INCLUDED
#define MMU_SYNC_OS_WINDOWS_PROCESS_MUTEX_H_INCLUDED

#include <stdint.h>

#include <mego/util/os/windows/windows_simplify.h>
#include <mego/util/math.h>

#ifdef __cplusplus
extern "C" {
#endif

#if MEGO_OS__WINDOWS__AVAILABLE

typedef struct {
    HANDLE handle;
} mmu_pmtx_t;

static int mmu_pmtx__init     (mmu_pmtx_t *_mtx, const char *_name, size_t _name_len);
static int mmu_pmtx__destroy  (mmu_pmtx_t *_mtx);
static int mmu_pmtx__lock     (mmu_pmtx_t *_mtx);
static int mmu_pmtx__unlock   (mmu_pmtx_t *_mtx);
static int mmu_pmtx__timedlock(mmu_pmtx_t *_mtx, uint64_t _timeout_ms);


static inline int mmu_pmtx__init(mmu_pmtx_t *_mtx, const char *_name, size_t _name_len)
{
    char full_name[MAX_PATH] = "Global\\";
    if (_name == NULL || _name_len == 0) {
        return -1;
    }
    if (_name_len > MAX_PATH - 8) {
        return -1;
    }
    if (strncat(full_name, _name, MGU_MATH__MIN(_name_len, sizeof(full_name) - 1 - 8)) == NULL) 
    {
        return -1;
    }
    full_name[sizeof(full_name) - 1] = '\0';

    _mtx->handle = CreateMutexA(NULL, FALSE, full_name);
    if (_mtx->handle == NULL) {
        return -1;
    }
    return 0;
}

static inline int mmu_pmtx__destroy(mmu_pmtx_t *_mtx)
{
    if (CloseHandle(_mtx->handle) == 0) {
        return -1;
    }
    return 0;
}

static inline int mmu_pmtx__lock(mmu_pmtx_t *_mtx)
{
    if (WaitForSingleObject(_mtx->handle, INFINITE) != WAIT_OBJECT_0) {
        return -1;
    }
    return 0;
}

static inline int mmu_pmtx__unlock(mmu_pmtx_t *_mtx)
{
    if (ReleaseMutex(_mtx->handle) == 0) {
        return -1;
    }
    return 0;
}

static inline int mmu_pmtx__timedlock(mmu_pmtx_t *_mtx, uint64_t _timeout_ms)
{
    if (WaitForSingleObject(_mtx->handle, (DWORD)_timeout_ms) != WAIT_OBJECT_0) {
        return -1;
    }
    return 0;
}

#endif // MEGO_OS__WINDOWS__AVAILABLE

#ifdef __cplusplus
}
#endif


#endif // !MMU_SYNC_OS_WINDOWS_PROCESS_MUTEX_H_INCLUDED
