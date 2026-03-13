#include "address.h"
#include <arpa/inet.h>

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
}