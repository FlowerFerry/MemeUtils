#ifndef MMUPP_OPENEXT_RAPIDJSON_TO_YYJSON_VAL_H_INCLUDED
#define MMUPP_OPENEXT_RAPIDJSON_TO_YYJSON_VAL_H_INCLUDED

#include <yyjson.h>
#include <rapidjson/document.h>

namespace mmupp {
namespace openext {

    /**
     * @brief Convert a rapidjson::Value to a yyjson_mut_val.
     *
     * @param doc The target yyjson_mut_doc that owns the returned value's memory.
     * @param src The source rapidjson::Value.
     * @return yyjson_mut_val* The converted value, or nullptr on error.
     *
     * @note The caller must ensure that the yyjson_mut_doc outlives the returned value.
     */
    inline yyjson_mut_val* to_yyjson_val(yyjson_mut_doc* doc, const rapidjson::Value& src) {
        if (src.IsNull())   return yyjson_mut_null(doc);
        if (src.IsBool())   return yyjson_mut_bool(doc, src.GetBool());
        if (src.IsDouble()) return yyjson_mut_real(doc, src.GetDouble());
        if (src.IsInt64())  return yyjson_mut_sint(doc, src.GetInt64());
        if (src.IsUint64()) return yyjson_mut_uint(doc, src.GetUint64());
        if (src.IsString()) return yyjson_mut_strncpy(doc, src.GetString(), src.GetStringLength());

        if (src.IsArray()) {
            yyjson_mut_val* arr = yyjson_mut_arr(doc);
            if (!arr) return nullptr;
            for (rapidjson::SizeType i = 0; i < src.Size(); ++i) {
                yyjson_mut_val* elem = to_yyjson_val(doc, src[i]);
                if (!elem) return nullptr;
                if (!yyjson_mut_arr_append(arr, elem)) return nullptr;
            }
            return arr;
        }

        if (src.IsObject()) {
            yyjson_mut_val* obj = yyjson_mut_obj(doc);
            if (!obj) return nullptr;
            for (auto it = src.MemberBegin(); it != src.MemberEnd(); ++it) {
                yyjson_mut_val* key = yyjson_mut_strncpy(doc, it->name.GetString(), it->name.GetStringLength());
                if (!key) return nullptr;
                yyjson_mut_val* val = to_yyjson_val(doc, it->value);
                if (!val) return nullptr;
                if (!yyjson_mut_obj_add(obj, key, val)) return nullptr;
            }
            return obj;
        }

        return nullptr;
    }

} // namespace openext
} // namespace mmupp

#endif // MMUPP_OPENEXT_RAPIDJSON_TO_YYJSON_VAL_H_INCLUDED
