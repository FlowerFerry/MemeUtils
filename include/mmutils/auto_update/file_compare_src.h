
#ifndef MMUTILS_AUTOUPDATE_FILE_COMPARE_SRC_H_INCLUDED
#define MMUTILS_AUTOUPDATE_FILE_COMPARE_SRC_H_INCLUDED

#include "file_compare_def.h"
#include <string.h>

#include "../io/file.h"

#if MEGO_OS__WINDOWS__AVAILABLE
#else
# include <sys/mman.h>
# include <unistd.h>
# include <sys/stat.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// 小于1mb的文件，直接读取到内存中比较
// 大于1mb的文件，使用mmap比较
inline int mmu_file_compare(const char *file1_path, intptr_t _s1len, const char *file2_path, intptr_t _s2len)
{
    FILE* fp2 = NULL;
    FILE* fp1 = mmu_fopen(file1_path, _s1len, "rb", -1);
    if (!fp1)
        return -1;
    
    fp2 = mmu_fopen(file2_path, _s2len, "rb", -1);
    if (!fp2) {
        fclose(fp1);
        return -1;
    }

    fseek(fp1, 0, SEEK_END);
    fseek(fp2, 0, SEEK_END);

    mmint_t len1 = ftell(fp1);
    mmint_t len2 = ftell(fp2);

    if (len1 != len2) {
        fclose(fp1);
        fclose(fp2);
        return 1;
    }

    if (len1 > 1024 * 1024) {
        int result = mmu_file_compare_by_mmap(fp1, fp2);
        fclose(fp1);
        fclose(fp2);
        return result;
    }
    else {
        mmint_t prelen = 512;
        char buf1[512];
        char buf2[512];

        if (len1 <= prelen)
            prelen = len1; 
        len1 -= prelen;

        fseek(fp1, len1, SEEK_SET);
        fseek(fp2, len1, SEEK_SET);

        fread(buf1, 1, prelen, fp1);
        fread(buf2, 1, prelen, fp2);

        if (memcmp(buf1, buf2, prelen) != 0) {
            fclose(fp1);
            fclose(fp2);
            return 0;
        }

        fseek(fp1, 0, SEEK_SET);
        fseek(fp2, 0, SEEK_SET);

        while (len1 > 0) {
            mmint_t readlen = len1 > 512 ? 512 : len1;

            fread(buf1, 1, readlen, fp1);
            fread(buf2, 1, readlen, fp2);

            if (memcmp(buf1, buf2, readlen) != 0) {
                fclose(fp1);
                fclose(fp2);
                return 0;
            }

            len1 -= readlen;
        }

        fclose(fp1);
        fclose(fp2);
        return 1;
    }
}

inline int mmu_file_compare_by_mmap(FILE* _fp1, FILE* _fp2)
{
#if MEGO_OS__WINDOWS__AVAILABLE
    int result = -1;
    HANDLE map1 = NULL;
    HANDLE map2 = NULL;
    LPVOID p1 = NULL;
    LPVOID p2 = NULL;
    HANDLE h1 = (HANDLE)_get_osfhandle(_fileno(_fp1));
    HANDLE h2 = (HANDLE)_get_osfhandle(_fileno(_fp2));
    mmint_t len1 = 0;
    mmint_t len2 = 0;
    mmint_t prelen = 1024 * 1024;

    if (h1 == INVALID_HANDLE_VALUE || h2 == INVALID_HANDLE_VALUE)
        return -1;

    len1 = GetFileSize(h1, NULL);
    len2 = GetFileSize(h2, NULL);

    if (len1 != len2)
        return 0;

    if (len1 == 0)
        return 1;

    do {
        mmint_t offset = 0;
        mmint_t block_size = 1024 * 1024;
        if (prelen) {
            if (len1 <= prelen)
                prelen = len1;

            len1 -= prelen;
            offset = len1;
            block_size = prelen;
            prelen = 0;
        }

        map1 = CreateFileMapping(h1, NULL, PAGE_READONLY, 0, 0, NULL);
        if (!map1)
            return -1;
        
        map2 = CreateFileMapping(h2, NULL, PAGE_READONLY, 0, 0, NULL);
        if (!map2) {
            CloseHandle(map1);
            return -1;
        }

        p1 = MapViewOfFile(map1, FILE_MAP_READ, 0, offset, block_size);
        if (!p1) {
            CloseHandle(map1);
            CloseHandle(map2);
            return -1;
        }

        p2 = MapViewOfFile(map2, FILE_MAP_READ, 0, offset, block_size);
        if (!p2) {
            UnmapViewOfFile(p1);
            CloseHandle(map1);
            CloseHandle(map2);
            return -1;
        }

        if (memcmp(p1, p2, block_size) != 0) {
            UnmapViewOfFile(p1);
            UnmapViewOfFile(p2);
            CloseHandle(map1);
            CloseHandle(map2);
            return 0;
        }

        UnmapViewOfFile(p1);
        UnmapViewOfFile(p2);
        CloseHandle(map1);
        CloseHandle(map2);

    } while(len1 > 0);

    return 1;
#else
    void* map1 = NULL;
    void* map2 = NULL;
    int fd1 = fileno(_fp1);
    int fd2 = fileno(_fp2);
    mmint_t len1 = 0;
    mmint_t len2 = 0;
    mmint_t prelen = 1024 * 1024;
    struct stat st;

    if (fstat(fd1, &st) != 0)
        return -1;
    len1 = st.st_size;

    if (fstat(fd2, &st) != 0)
        return -1;
    len2 = st.st_size;
    
    if (len1 != len2)
        return 0;
    
    if (len1 == 0)
        return 1;

    do {
        mmint_t offset = 0;
        mmint_t block_size = 1024 * 1024;
        if (prelen) {
            if (len1 <= prelen)
                prelen = len1;

            len1 -= prelen;
            offset = len1;
            block_size = prelen;
            prelen = 0;
        }

        map1 = mmap(NULL, block_size, PROT_READ, MAP_SHARED, fd1, offset);
        if (map1 == MAP_FAILED)
            return -1;

        map2 = mmap(NULL, block_size, PROT_READ, MAP_SHARED, fd2, offset);
        if (map2 == MAP_FAILED) {
            munmap(map1, block_size);
            return -1;
        }

        if (memcmp(map1, map2, block_size) != 0) {
            munmap(map1, block_size);
            munmap(map2, block_size);
            return 0;
        }

        munmap(map1, block_size);
        munmap(map2, block_size);

    } while(len1 > 0);

    return 1;
#endif 
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !MMUTILS_AUTOUPDATE_FILE_COMPARE_SRC_H_INCLUDED
