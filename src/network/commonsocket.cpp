#include "commonsocket.h"
#include "byte_order.h"
#include <utility>

namespace network{

    Socket::Socket(Socket&& other) noexcept{
        *this = std::move(other);
    }
    Socket::Socket(int raw_socket_id) noexcept:
    fd_(raw_socket_id),closed_(false){}
    Socket& Socket::operator=(
            Socket&& other) noexcept{
        if(&other!=this){
            close();
            fd_ = std::exchange(other.fd_, -1);
            closed_.exchange(other.closed_,std::memory_order_acq_rel);
        }
        return *this;
    }
    bool Socket::set_no_block(bool noblock,std::error_code& err) noexcept{
        if (fd_ < 0){
            err = std::make_error_code(std::errc::bad_file_descriptor);
            errno=0;
            return false;
        }
        int flags = fcntl(fd_,F_GETFL);
        if(flags==-1){
            err = std::make_error_code(static_cast<std::errc>(errno));
            errno=0;
            return false;
        }
        else{
            if(noblock){
                if(fcntl(fd_,F_SETFL,flags|O_NONBLOCK)==-1){
                    err = std::make_error_code(static_cast<std::errc>(errno));
                    errno=0;
                    return false;
                }
                else{
                    err.clear();
                    return true;
                }
            }
            else{
                if(fcntl(fd_,F_SETFL,flags&~O_NONBLOCK)==-1){
                    err = std::make_error_code(static_cast<std::errc>(errno));
                    errno=0;
                    return false;
                }
                else{
                    err.clear();
                    return true;
                }
            }
        }
    }
    bool Socket::is_non_block(std::error_code& err) const noexcept{
        if(fd_>=0){
            int flags = 0;
            if(flags = fcntl(fd_,F_GETFL);flags==-1 ||
                fcntl(fd_,F_SETFL,flags|O_NONBLOCK)==-1)
            {
                #ifdef DEBUG
                    std::cout<<strerror(errno)<<std::endl;
                #endif
                err = std::make_error_code(
                    static_cast<std::errc>(errno));
                errno = 0;
                return false;
            }
            else{
                err.clear();
                return (flags&O_NONBLOCK)!=0;
            }
        }
        else{
            err = std::make_error_code(
                std::errc::bad_file_descriptor);
            errno = 0;
            return false;
        }
    }
    bool Socket::bind(const Address& addr,std::error_code& err) noexcept{
        if(::bind(fd_,
            reinterpret_cast<const sockaddr*>(addr.data()),
            addr.length())==-1){
            err = std::make_error_code(static_cast<std::errc>(errno));
            errno=0;
            return false;
        }
        else {
            err.clear();
            errno = 0;
            return true;
        }
    }
    Socket socket(
    const Address& storage,
    Socket::Type type,
    Protocol proto,
    std::error_code& err)
    noexcept
    {
        if(!storage.valid())
        {
            err = std::make_error_code(
                std::errc::address_family_not_supported);
            return -1;
        }
        if(int raw_socket = ::socket(
                static_cast<int>(
                storage.ip_version()),
                static_cast<int>(type),
                static_cast<int>(proto));
                raw_socket==-1){
            err = std::make_error_code(
                static_cast<std::errc>(errno));
            errno=0;
            return Socket(raw_socket);
        }
        else return Socket(raw_socket);
    }
}

network::Socket::Options operator|(network::Socket::Options lhs,network::Socket::Options rhs) noexcept{
    return static_cast<network::Socket::Options>(static_cast<std::underlying_type_t<network::Socket::Options>>(lhs)|
            static_cast<std::underlying_type_t<network::Socket::Options>>(rhs));
}
network::Socket::Options operator&(network::Socket::Options lhs,network::Socket::Options rhs) noexcept{
    return static_cast<network::Socket::Options>(static_cast<std::underlying_type_t<network::Socket::Options>>(lhs)&
            static_cast<std::underlying_type_t<network::Socket::Options>>(rhs));
}
network::Socket::Options operator^(network::Socket::Options lhs,network::Socket::Options rhs) noexcept{
    return static_cast<network::Socket::Options>(static_cast<std::underlying_type_t<network::Socket::Options>>(lhs)^
            static_cast<std::underlying_type_t<network::Socket::Options>>(rhs));
}
network::Socket::Options operator~(network::Socket::Options val) noexcept{
    return static_cast<network::Socket::Options>(~static_cast<std::underlying_type_t<network::Socket::Options>>(val));
}
network::Socket::Options operator&(network::Socket::Options lhs,int rhs) noexcept{
    return static_cast<network::Socket::Options>(static_cast<std::underlying_type_t<network::Socket::Options>>(lhs)&
            rhs);
}
network::Socket::Options operator&(std::underlying_type_t<network::Socket::Options> lhs, network::Socket::Options rhs) noexcept{
    return static_cast<network::Socket::Options>(lhs &
            static_cast<std::underlying_type_t<network::Socket::Options>>(rhs));
}
network::Socket::Options operator|(network::Socket::Options lhs,std::underlying_type_t<network::Socket::Options> rhs) noexcept{
    return static_cast<network::Socket::Options>(static_cast<std::underlying_type_t<network::Socket::Options>>(lhs)|
            rhs);
}
network::Socket::Options operator|(std::underlying_type_t<network::Socket::Options> lhs,network::Socket::Options rhs) noexcept{
    return static_cast<network::Socket::Options>(lhs |
            static_cast<std::underlying_type_t<network::Socket::Options>>(rhs));
}

network::Socket::Type operator|(network::Socket::Type lhs,network::Socket::Type rhs) noexcept{
    return static_cast<network::Socket::Type>(static_cast<std::underlying_type_t<network::Socket::Type>>(lhs)|
            static_cast<std::underlying_type_t<network::Socket::Type>>(rhs));
}
network::Socket::Type operator&(network::Socket::Type lhs,network::Socket::Type rhs) noexcept{
    return static_cast<network::Socket::Type>(static_cast<std::underlying_type_t<network::Socket::Type>>(lhs)&
            static_cast<std::underlying_type_t<network::Socket::Type>>(rhs));
}
network::Socket::Type operator^(network::Socket::Type lhs,network::Socket::Type rhs) noexcept{
    return static_cast<network::Socket::Type>(static_cast<std::underlying_type_t<network::Socket::Type>>(lhs)^
            static_cast<std::underlying_type_t<network::Socket::Type>>(rhs));
}
network::Socket::Type operator~(network::Socket::Type val) noexcept{
    return static_cast<network::Socket::Type>(~static_cast<std::underlying_type_t<network::Socket::Type>>(val));
}
network::Socket::Type operator&(network::Socket::Type lhs,int rhs) noexcept{
    return static_cast<network::Socket::Type>(static_cast<std::underlying_type_t<network::Socket::Type>>(lhs)&
            rhs);
}
network::Socket::Type operator&(std::underlying_type_t<network::Socket::Type> lhs, network::Socket::Type rhs) noexcept{
    return static_cast<network::Socket::Type>(lhs &
            static_cast<std::underlying_type_t<network::Socket::Type>>(rhs));
}
network::Socket::Type operator|(network::Socket::Type lhs,std::underlying_type_t<network::Socket::Type> rhs) noexcept{
    return static_cast<network::Socket::Type>(static_cast<std::underlying_type_t<network::Socket::Type>>(lhs)|
            rhs);
}
network::Socket::Type operator|(std::underlying_type_t<network::Socket::Type> lhs,network::Socket::Type rhs) noexcept{
    return static_cast<network::Socket::Type>(lhs |
            static_cast<std::underlying_type_t<network::Socket::Type>>(rhs));
}