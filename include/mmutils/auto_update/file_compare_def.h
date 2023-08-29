
#ifndef MMUTILS_AUTOUPDATE_FILE_COMPARE_DEF_H_INCLUDED
#define MMUTILS_AUTOUPDATE_FILE_COMPARE_DEF_H_INCLUDED

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

static inline int mmu_file_compare(const char *file1_path, intptr_t _s1len, const char *file2_path, intptr_t _s2len);
static inline int mmu_file_compare_by_mmap(FILE* _fp1, FILE* _fp2);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !MMUTILS_AUTOUPDATE_FILE_COMPARE_DEF_H_INCLUDED
