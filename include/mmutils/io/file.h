
#ifndef MMU_IO_FILE_H_INCLUDED
#define MMU_IO_FILE_H_INCLUDED

#include <meme/utf/converter.h>
#include <mego/predef/os/windows.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

inline FILE* mmu_fopen(
    const char* _path, mmint_t _slen, const char* _mode, mmint_t _mlen)
{
    FILE* fp = NULL;
#if MEGO_OS__WINDOWS__AVAILABLE
    mmint_t u16len = 0;
    wchar_t* path = NULL;
    wchar_t* mode = NULL;
#else
    const char* path = NULL;
    const char* mode = NULL;
#endif

    if (_slen < 0)
        _slen = strlen(_path);

    if (_mlen < 0)
        _mlen = strlen(_mode);

#if MEGO_OS__WINDOWS__AVAILABLE
    u16len = mmutf_char_size_u16from8((const uint8_t*)_path, _slen);
    if (u16len < 1)
        return NULL;
    
    path = (wchar_t*)malloc(sizeof(wchar_t) * (u16len + 1));
    if (!path)
        return NULL;
    
    if (mmutf_convert_u8to16((const uint8_t*)_path, _slen, (uint16_t*)path) == 0)
    {
        free(path);
        return NULL;
    }
    path[u16len] = L'\0';

    u16len = mmutf_char_size_u16from8((const uint8_t*)_mode, _mlen);
    if (u16len < 1) {
        free(path);
        return NULL;
    }

    mode = (wchar_t*)malloc(sizeof(wchar_t) * (u16len + 1));
    if (!mode) {
        free(path);
        return NULL;
    }

    if (mmutf_convert_u8to16((const uint8_t*)_mode, _mlen, (uint16_t*)mode) == 0) 
    {
        free(path);
        free(mode);
        return NULL;
    }
    mode[u16len] = L'\0';

    fp = _wfopen(path, mode);
    free(path);
    free(mode);
    return fp;
#else
    if (_path[_slen] == '\0')
        path = _path;
    else {
        path = (char*)malloc(sizeof(char) * (_slen + 1));
        if (!path)
            return NULL;
        memcpy((void*)path, (const void*)_path, _slen);
        path[_slen] = '\0';
    }

    if (_mode[_mlen] == '\0')
        mode = _mode;
    else {
        mode = (char*)malloc(sizeof(char) * (_mlen + 1));
        if (!mode) {
            if (path != _path)
                free(path);
            return NULL;
        }
        memcpy((void*)mode, (const void*)_mode, _mlen);
        mode[_mlen] = '\0';
    }

    fp = fopen(path, mode);
    if (path != _path)
        free(path);
    if (mode != _mode)
        free(mode);
    return fp;
#endif
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !MMU_IO_FILE_H_INCLUDED
