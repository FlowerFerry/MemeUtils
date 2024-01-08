
#ifndef MMUPP_OLIB_NUMBER_H_INCLUDED
#define MMUPP_OLIB_NUMBER_H_INCLUDED

#include <yyjson.h>
#include <meme/string_fwd.h>

namespace mmupp{
namespace olib {

    template<typename T>
    inline T get_num_with_default(yyjson_val *_val, const char* _key, mmint_t _slen, T _default) 
    {
        if (_slen < 0) 
            _slen = strlen(_key);
        yyjson_val *v = yyjson_obj_getn(_val, _key, static_cast<size_t>(_slen));
        if (v == nullptr) 
            return _default;
            
        switch (yyjson_get_type(v)) 
        {
        case YYJSON_TYPE_BOOL: 
            return yyjson_get_bool(v) ? 1 : 0;
        case YYJSON_TYPE_NUM: 
            return static_cast<T>(yyjson_get_num(v));
        case YYJSON_TYPE_STR: 
            return static_cast<T>(std::stod(yyjson_get_str(v)));
        default: 
            return _default;
        }
    }

}
}

#endif // !MMUPP_OLIB_NUMBER_H_INCLUDED
