
#include <mmutilspp/fs/rec_dir_path.hpp>
#include <megopp/util/scope_cleanup.h>

#if MG_OS__WIN_AVAIL
#include <io.h>
#include <fcntl.h>
#else
#endif

#if MG_OS__WIN_AVAIL
#define TFUNCPRE(x) w##x
#else
#define TFUNCPRE(x) x
#endif

#include <iostream>

int main()
{
#if MG_OS__WIN_AVAIL
    auto omode = _setmode(_fileno(stdout), _O_U16TEXT);
    if (omode == -1) {
        TFUNCPRE(printf)(MMN_TEXT("Unable to set console output mode\n"));
    }

    auto imode = _setmode(_fileno(stdin), _O_U16TEXT);
    if (imode == -1) {
        TFUNCPRE(printf)(MMN_TEXT("Unable to set console input mode\n"));
    }

    MEGOPP_UTIL__ON_SCOPE_CLEANUP([&] {
        if (omode != -1)
            _setmode(_fileno(stdout), omode);
        if (imode != -1)
            _setmode(_fileno(stdin),  imode);
    });
#endif

    auto str = mmupp::fs::rec_writable_dir_path();
    TFUNCPRE(printf)(MMN_TEXT("%s\n"), mm_into<memepp::native_string>(str).data());
    return 0;
}
