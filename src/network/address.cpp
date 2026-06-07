#include "address.h"
#include <arpa/inet.h>
#include <string>
#include <string_view>

namespace network{
    Address make_address(
        const std::string& host,
        Port port,
        std::error_code& err) noexcept
    {
        sockaddr_storage storage{};
        if(auto sock4 = reinterpret_cast<sockaddr_in*>(&storage);
            inet_pton(AF_INET,host.c_str(),&sock4->sin_addr)==1){
            sock4->sin_family = AF_INET;
            sock4->sin_port = htons(port);
        }
        else{
            if(auto* sock6 = reinterpret_cast<sockaddr_in6*>(&storage);
            inet_pton(AF_INET6,host.c_str(),&sock6->sin6_addr)==1){
                sock6->sin6_family = AF_INET6;
                sock6->sin6_port = htons(port);
            }
            else{
                err = std::make_error_code(std::errc::invalid_argument);
                return Address();
            }
        }
        return Address(storage,err);
    }

    using namespace std::string_literals;
    std::string ip_to_text(const Address& addr) noexcept{
        switch(static_cast<Family>(addr.ip_version())){
            case Family::IPv4:{
                char ipstr[INET_ADDRSTRLEN];
                return "IPv4:"s+inet_ntop(
                        static_cast<int>(addr.ip_version()),
                        &((sockaddr_in*)&addr)->sin_addr,
                            ipstr,
                            sizeof(ipstr));
                break;
            }
            case Family::IPv6:{
                char ipstr[INET6_ADDRSTRLEN];
                return "IPv6:"s+inet_ntop(
                        static_cast<int>(addr.ip_version()),
                        &((sockaddr_in6*)&addr)->sin6_addr,
                            ipstr,
                            sizeof(ipstr));
                break;
            }
            default:
                return "NaN"s;
                break;
        }
    }

    std::string port_to_text(const Address& addr) noexcept{
        switch(static_cast<Family>(addr.ip_version())){
            case Family::IPv4:
                return "port:"s+std::to_string(ntohs((
                    reinterpret_cast<const sockaddr_in*>(
                        addr.get_sockaddr()))->sin_port));
                break;
            case Family::IPv6:
                return "port:"s+std::to_string(ntohs((
                    reinterpret_cast<const sockaddr_in6*>(
                        addr.get_sockaddr()))->sin6_port));
                break;
            default:
                return "NaN"s;
                break;
        }
    }
    std::string protocol_to_text(const Address& addr) noexcept
    {
        switch(static_cast<Family>(addr.ip_version())){
            case Family::IPv4:{
                return "IPv4"s;
                break;
            }
            case Family::IPv6:{
                return "IPv6"s;
                break;
            }
            default:
                return "NaN"s;
                break;
        }
    }

    std::ostream& print_ip_port(std::ostream& stream,
            const Address& addr) noexcept
    {
        stream<<ip_to_text(addr)<<":"<<port_to_text(addr)<<std::endl;
        return stream;
    }

    bool is_correct_address(const std::string& text) noexcept{
        {
            sockaddr_in addr;
            if(inet_pton(AF_INET,text.c_str(),&addr)==1)
                return true;
        }
        {
            sockaddr_in6 addr;
            if(inet_pton(AF_INET6,text.c_str(),&addr)==1)
                return true;
        }
        return false;
    }
    bool is_correct_address(std::string_view text) noexcept{
        std::string copy(text);
        {
            sockaddr_in addr;
            if(inet_pton(AF_INET,copy.c_str(),&addr)==1)
                return true;
        }
        {
            sockaddr_in6 addr;
            if(inet_pton(AF_INET6,copy.c_str(),&addr)==1)
                return true;
        }
        return false;
    }

    socklen_t address_struct_size(const sockaddr_storage& storage) noexcept{
        return storage.ss_family==AF_INET?sizeof(sockaddr_in):
            (storage.ss_family==AF_INET6?sizeof(sockaddr_in6):
            sizeof(Address));
    }
}

template<>
boost::json::value to_json(const network::Address& val){
    boost::json::object result;
    result["host"]=to_json(network::ip_to_text(val));
    result["port"]=to_json(network::port_to_text(val));
    return result;
}

template<>
std::expected<network::Address,std::exception> 
    from_json(const boost::json::value& val){
    if(val.is_object()){
        auto& obj = val.as_object();
        std::string host;
        network::Port port=0;
        if(obj.contains("host"))
            if(auto tmp=from_json<std::string>(obj.at("host"));
                tmp.has_value())
                host = tmp.value();
        if(obj.contains("port")){
            if(auto tmp=from_json<network::Port>(obj.at("port"));
                tmp.has_value())
                port = tmp.value();
        }
        if(port!=0 || !host.empty()){
            std::error_code err;
            network::Address addr=network::make_address(host,port,err);
            if(err!=std::error_code())
                return std::unexpected(std::invalid_argument("invalid address/port"));
            else return addr;
        }
        return network::Address();
    }
    else return std::unexpected(std::invalid_argument("type error"));
}

namespace CLI {
namespace detail {
    template <>
    bool lexical_cast(const std::string& input, ::network::Address& output) {
        static const std::regex regex_val(
        "^(?:(?:(?:[0-9a-fA-F]{1,4}:){7,7}\
[0-9a-fA-F]{1,4}|(?:[0-9a-fA-F]{1,4}:){1,7}\
:|(?:[0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}\
|(?:[0-9a-fA-F]{1,4}:){1,5}(?::[0-9a-fA-F]{1,4}){1,2}\
|(?:[0-9a-fA-F]{1,4}:){1,4}(?::[0-9a-fA-F]{1,4}){1,3}\
|(?:[0-9a-fA-F]{1,4}:){1,3}(?::[0-9a-fA-F]{1,4}){1,4}\
|(?:[0-9a-fA-F]{1,4}:){1,2}(?::[0-9a-fA-F]{1,4}){1,5}\
|[0-9a-fA-F]{1,4}:(?:(?::[0-9a-fA-F]{1,4}){1,6})\
|:(?:(?::[0-9a-fA-F]{1,4}){1,7}|:)\
|fe80:(?::[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}\
|::(?:ffff(?::0{1,4}){0,1}:){0,1}\
(?:(?:25[0-5]|(?:2[0-4]|1{0,1}[0-9]){0,1}[0-9]).)\
{3,3}(?:25[0-5]|(?:2[0-4]|1{0,1}[0-9]){0,1}[0-9])\
|(?:[0-9a-fA-F]{1,4}:){1,4}:(?:(?:25[0-5]\
|(?:2[0-4]|1{0,1}[0-9]){0,1}[0-9]).)\
{3,3}(?:25[0-5]|(?:2[0-4]|1{0,1}[0-9]){0,1}[0-9])))\
(?::([0-9]{1,5}))?$");
        std::smatch match;
        if (std::regex_search(input.begin(),input.end(), match, regex_val)) {
            std::string host;
            uint64_t port;
            host=match[0].str();
            if(match.size()>1){
                std::string tmp=match[1].str();
                if(std::from_chars(tmp.data(),tmp.data()+tmp.size(),port).ec==std::errc() && 
                    port<=static_cast<uint64_t>(std::numeric_limits<uint16_t>::max()))
                {
                    std::error_code err;
                    output = network::make_address(host,port,err);
                    if(err!=std::error_code() || output.valid())
                        return true;
                    else throw std::runtime_error("invalid host/port input");
                }
                else throw std::runtime_error("invalid option value \"port\"");
            }
            else{
                std::error_code err;
                output = network::make_address(host,0,err);
                if(output.valid())
                    return true;
                else throw std::runtime_error("invalid host input");
            }
        }
        else throw std::runtime_error("invalid host input");
    }

    
}
}

std::ostream& operator<<(std::ostream& stream,const network::Address address){
    network::print_ip_port(stream,address);
    return stream;
}

std::istream& operator>>(std::istream& stream,network::Address& address){
    std::string input;
    CLI::detail::lexical_cast(input,address);
    return stream;
}