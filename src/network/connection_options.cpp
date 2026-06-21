#include "connection_options.h"

bool network::ConnectionOptions::operator==(const ConnectionOptions& other) const noexcept{
    return  reuse_address_==other.reuse_address_ &&
            reuse_port_==other.reuse_port_ &&
            broadcast_socket_==other.broadcast_socket_&&
            dont_route_==other.dont_route_&&
            keep_alive_==other.keep_alive_&&
            linger_==other.linger_&&
            timeout_input_==other.timeout_input_&&
            timeout_send_==other.timeout_send_&&
            expect_min_bytes_available_==other.expect_min_bytes_available_&&
            buffer_size_in_==other.buffer_size_in_&&
            buffer_size_out_==other.buffer_size_out_;
}

template<>
boost::json::value to_json(const network::ConnectionOptions& val){
    using namespace boost;
    json::object map;
    map["reuse address"] = static_cast<bool>(val.reuse_address_.first);
    map["reuse port"] = static_cast<bool>(val.reuse_port_.first);
    map["broadcast"]=static_cast<bool>(val.broadcast_socket_.first);
    map["don't route"] = static_cast<bool>(val.dont_route_.first);
    map["keep alive"] = static_cast<bool>(val.keep_alive_.first);
    map["linger"] = to_json(val.linger_.first);
    map["timeout sending"] = to_json(val.timeout_send_.first);
    map["timeout receive"] = to_json(val.timeout_input_.first);
    map["min bytes input"]=val.expect_min_bytes_available_.first;
    map["input buffer"]=val.buffer_size_in_.first;
    map["output buffer"]=val.buffer_size_out_.first;
    return map;
}

template<>
std::expected<network::ConnectionOptions,std::exception> 
    from_json(const boost::json::value& val){
    using namespace boost;
    network::ConnectionOptions result;

    if(!val.is_object())
        return std::unexpected(std::invalid_argument("not object"));
    auto& c = val.as_object();
    if(c.contains("reuse address")){
        if(auto res = from_json<decltype(network::ConnectionOptions::reuse_address_.first)>(
            c.at("reuse address"));res.has_value())
            result.reuse_address_={res.value(),{}};
    }
    if(c.contains("reuse port")){
        if(auto res = from_json<decltype(network::ConnectionOptions::reuse_port_.first)>(
            c.at("reuse port"));res.has_value())
            result.reuse_port_={res.value(),{}};
    }
    if(c.contains("broadcast")){
        if(auto res = from_json<decltype(network::ConnectionOptions::broadcast_socket_.first)>(
            c.at("broadcast"));res.has_value())
            result.broadcast_socket_={res.value(),{}};
    }
    if(c.contains("don't route")){
        if(auto res = from_json<decltype(network::ConnectionOptions::dont_route_.first)>(
            c.at("don't route"));res.has_value())
            result.dont_route_={res.value(),{}};
    }
    if(c.contains("keep alive")){
        if(auto res = from_json<decltype(network::ConnectionOptions::keep_alive_.first)>(
            c.at("keep alive"));res.has_value())
            result.keep_alive_={res.value(),{}};
    }
    if(c.contains("linger")){
        if(auto res = from_json<decltype(network::ConnectionOptions::linger_.first)>(
            c.at("linger"));res.has_value())
            result.linger_={res.value(),{}};
    }
    if(c.contains("timeout sending")){
        if(auto res = from_json<decltype(network::ConnectionOptions::timeout_send_.first)>(
            c.at("timeout sending"));res.has_value())
            result.timeout_send_={res.value(),{}};
    }
    if(c.contains("timeout receive")){
        if(auto res = from_json<decltype(network::ConnectionOptions::timeout_input_.first)>(
            c.at("timeout receive"));res.has_value())
            result.timeout_input_={res.value(),{}};
    }
    if(c.contains("min bytes input")){
        if(auto res = from_json<decltype(
            network::ConnectionOptions::expect_min_bytes_available_.first)>(
            c.at("min bytes input"));res.has_value())
            result.expect_min_bytes_available_={res.value(),{}};
    }
    if(c.contains("input buffer")){
        if(auto res = from_json<decltype(
            network::ConnectionOptions::buffer_size_in_.first)>(
            c.at("input buffer"));res.has_value())
            result.buffer_size_in_={res.value(),{}};
    }
    if(c.contains("output buffer")){
        if(auto res = from_json<decltype(
            network::ConnectionOptions::buffer_size_out_.first)>(
            c.at("output buffer"));res.has_value())
            result.buffer_size_out_={res.value(),{}};
    }
    return result;
}

bool network::operator==(const timeval& lhs,
                    const timeval& rhs) noexcept
{
    return lhs.tv_sec==rhs.tv_sec &&
            lhs.tv_usec==rhs.tv_usec;
}

bool network::operator==(const linger& lhs,
                    const linger& rhs) noexcept
{
    return lhs.l_linger==rhs.l_linger &&
            lhs.l_onoff==rhs.l_onoff;
}

template<typename VAL,network::Socket::Options OPT>
bool network::operator==(const std::pair<VAL,OptionType<OPT>>& lhs,
                    const std::pair<VAL,OptionType<OPT>>& rhs) noexcept
{
    return lhs.first==rhs.first;
}