#include "clientsettings.h"

template<>
boost::json::value to_json(const network::client::Settings& val){
    using namespace boost;
    json::object map;
    map["jobs"] = val.num_threads_pool_;
    map["events handled"]=val.number_events_;
    map["protocol"] = network::protocol::to_text(val.protocol_);
    map["timeout"] = val.timeout_seconds_processes_;
    if(val.binded_addr_.has_value())
        map["address bind"]=to_json(val.binded_addr_.value());
    map["options"]=to_json(val.options_);
    return map;
}

template<>
std::expected<network::client::Settings,std::exception> from_json(const boost::json::value& val){
    network::client::Settings result;
    if(val.is_object()){
        auto& c = val.as_object();
        try{
            if(c.contains("jobs")){
                if(auto jobs_res = from_json<decltype(result.num_threads_pool_)>(
                    c.at("jobs"));jobs_res.has_value())
                        result.num_threads_pool_=jobs_res.value();
                else return std::unexpected(jobs_res.error());
            }
            if(c.contains("protocol")){
                if(auto proto_res = from_json<std::string>(c.at("protocol"));proto_res.has_value()){
                    if(auto proto = network::protocol::to_number(proto_res.value());proto.has_value())
                        result.protocol_=network::protocol::to_number(proto_res.value()).value();
                    else result.protocol_ = network::Protocol::TCP;
                }
                else return std::unexpected(proto_res.error());
            }
            if(c.contains("timeout")){
                if(auto timeout_res = from_json<decltype(
                    result.timeout_seconds_processes_)>(c.at("timeout"));
                        timeout_res.has_value())
                    result.timeout_seconds_processes_=timeout_res.value();
                else return std::unexpected(timeout_res.error());
            }
            if(c.contains("options")){
                if(auto options_res = from_json<decltype(result.options_)>(c.at("options"));
                    options_res.has_value()){
                    result.options_ = std::move(options_res.value());
                }
                else return std::unexpected(options_res.error());
            }
            if(c.contains("address bind")){
                if(auto res = from_json<network::Address>(
                    c.at("address bind"));res.has_value())
                    result.binded_addr_=res.value();
            }
        }
        catch(const boost::container::out_of_range& err){
            return std::unexpected(err);
        }
    }
    return result;
}