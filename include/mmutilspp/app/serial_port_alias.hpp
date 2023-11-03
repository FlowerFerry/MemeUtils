
#ifndef MMUPP_APP_SERIAL_PORT_ALIAS_HPP_INCLUDED
#define MMUPP_APP_SERIAL_PORT_ALIAS_HPP_INCLUDED

#include <memepp/string.hpp>
#include <memepp/hash/std_hash.hpp>

#include <unordered_set>
#include <vector>
#include <memory>

namespace mmupp { namespace app {
    
    struct serial_port_alias
    {
        enum replace_mode_t {
            unknown,
            internal_jump_wire_cap_replace,
            internal_module_replace,
            external_shared,
        };
        
        serial_port_alias():
            replace_mode(unknown)
        {}

        memepp::string name;
        memepp::string path;
        std::unordered_set<memepp::string> types; //< RS232, RS485, RS422, ...
        replace_mode_t replace_mode;
    };

    struct serial_port_alias_list
    {
        std::vector<serial_port_alias> list;
    };
    using serial_port_alias_list_ptr = std::shared_ptr<serial_port_alias_list>;

}}

#endif // !MMUPP_APP_SERIAL_PORT_ALIAS_HPP_INCLUDED
