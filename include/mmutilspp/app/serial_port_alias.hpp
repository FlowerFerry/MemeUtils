
#ifndef MMUPP_APP_SERIAL_PORT_ALIAS_HPP_INCLUDED
#define MMUPP_APP_SERIAL_PORT_ALIAS_HPP_INCLUDED

#include <memepp/string.hpp>
#include <memepp/hash/std_hash.hpp>

#include <unordered_set>
#include <vector>
#include <memory>

namespace mmupp { namespace app {
    
    struct serport_alias
    {
        enum replace_mode_t {
            unknown,
            internal_jump_wire_cap_replace,
            internal_module_replace,
            external_shared,
        };
        
        serport_alias():
            replace_mode(unknown),
            id(-1)
        {}

        memepp::string name;
        memepp::string path;
        std::unordered_set<memepp::string> types; //< RS232, RS485, RS422, ...
        replace_mode_t replace_mode;
        int id;
    };
    using serport_alias_ptr = std::shared_ptr<serport_alias>;

    struct serport_alias_list
    {
        serport_alias_ptr find(int _id) const;

        std::vector<serport_alias_ptr> list;
    };
    using serport_alias_list_ptr = std::shared_ptr<serport_alias_list>;

    inline serport_alias_ptr serport_alias_list::find(int _id) const
    {
        auto it = std::find_if(list.begin(), list.end(), 
            [_id](const serport_alias_ptr& _alias) {
                return _alias->id == _id;
            });
        return it != list.end() ? *it : nullptr;
    }
    
}}

#endif // !MMUPP_APP_SERIAL_PORT_ALIAS_HPP_INCLUDED
