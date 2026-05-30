#include "serversettings.h"

template<>
boost::json::value to_json(const network::server::Settings& val){
    using namespace boost;
    json::object map;
    map["host"] = val.host_;
    map["jobs"] = val.num_threads_pool_;
    map["events handled"]=val.number_events_;
    map["service"] = val.service_;
    map["port"] = val.port_;
    map["protocol"] = network::protocol::to_text(val.protocol_);
    map["timeout"] = val.timeout_seconds_processes_;
    map["options"]=to_json(val.options_);
    return map;
}

template<>
std::expected<network::server::Settings,std::exception> from_json(const boost::json::value& val){
    network::server::Settings result;
    if(val.is_object()){
        auto& c = val.as_object();
        if(c.contains("host")){
            if(auto host_res = from_json<decltype(result.host_)>(c.at("host"));host_res.has_value())
                result.host_=host_res.value();
            else return std::unexpected(host_res.error());
        }
        if(c.contains("jobs")){
            if(auto jobs_res = from_json<decltype(result.num_threads_pool_)>(
                c.at("jobs"));jobs_res.has_value())
                    result.num_threads_pool_=jobs_res.value();
            else return std::unexpected(jobs_res.error());
        }
        if(c.contains("service")){
            if(auto service_res = from_json<decltype(result.service_)>(c.at("service"));service_res.has_value())
                result.service_=service_res.value();
            else return std::unexpected(service_res.error());
        }
        if(c.contains("port")){
            if(auto port_res = from_json<decltype(result.port_)>(c.at("port"));port_res.has_value())
                result.port_=port_res.value();
            else return std::unexpected(port_res.error());
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
    }
    return result;
}