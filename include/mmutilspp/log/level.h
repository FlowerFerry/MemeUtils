
#ifndef MMUPP_LOG_LEVEL_H_INCLUDED
#define MMUPP_LOG_LEVEL_H_INCLUDED

#include <stdint.h>

namespace mmupp {
namespace log {

enum class level : uint8_t
{
    trace = 0,
    debug = 1,
    info  = 2,
    warn  = 3,
    error = 4,
    fatal = 5,
    off   = 6,
};

inline const char* level_to_str(level _level) noexcept
{
    switch (_level) {
    case level::trace: {
        static const char* str = "TRACE";
        return str;
    }
    case level::debug: {
        static const char* str = "DEBUG";
        return str;
    }
    case level::info: {
        static const char* str = "INFO";
        return str;
    }
    case level::warn: {
        static const char* str = "WARN";
        return str;
    }
    case level::error: {
        static const char* str = "ERROR";
        return str;
    }
    case level::fatal: {
        static const char* str = "FATAL";
        return str;
    }
    case level::off: {
        static const char* str = "OFF";
        return str;
    }
    default:
        return "UNKNOWN";
    }
}

}
}

#endif // !MMUPP_LOG_LEVEL_H_INCLUDED