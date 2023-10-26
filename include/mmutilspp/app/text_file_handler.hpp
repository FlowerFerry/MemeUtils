
#ifndef MMUPP_APP_TXTFILE_HANDLER_HPP_INCLUDED
#define MMUPP_APP_TXTFILE_HANDLER_HPP_INCLUDED

namespace mmupp::app {
    
    template<typename _Object>
    class txtfile_handler
    {
        //using object_t = _Object;
        //using object_ptr_t = std::shared_ptr<object_t>;

        //inline object_ptr_t default_object()
        //{
        //    return std::make_shared<object_t>();
        //}

        //inline outcome::checked<object_ptr_t, mgpp::err> deserialize(const std::string& _str)
        //{
        //    return outcome::failure(mgpp::err{ MGEC__ERR });
        //}

        //inline outcome::checked<std::string, mgpp::err> serialize(const object_t& _obj)
        //{
        //    return outcome::failure(mgpp::err{ MGEC__ERR });
        //}
    };

}

#endif // !MMUPP_APP_TXTFILE_HANDLER_HPP_INCLUDED
