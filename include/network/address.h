#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <system_error>
#include "definitions/family.h"
#include "definitions/protocol.h"
#include <cstring>
#include <utility>
#include <array>
#include <string>

namespace network{
    using Port = uint16_t;
    class Address{
        std::array<std::byte,sizeof(sockaddr_in6)> data_;
        socklen_t len_{0};
        void init_addr_internal(const sockaddr_storage& addr,std::error_code& err) noexcept{
            err.clear();
            len_ = 0;
            switch (static_cast<Family>(addr.ss_family))
            {
            case Family::IPv4:
            {
                std::memcpy(data_.data(),&addr,sizeof(sockaddr_in));
                len_ = sizeof(sockaddr_in);
                break;
            }
            case Family::IPv6:
                std::memcpy(data_.data(),&addr,sizeof(sockaddr_in6));
                len_ = sizeof(sockaddr_in6);
                break;
            default:
                err = std::make_error_code(std::errc::address_family_not_supported);
                break;
            }
        }
        
        friend Address make_address(
            const std::string& host,
            Port port,
            std::error_code& err) noexcept;
        public:
        Address() = default;
        Address(const Address& other):
        data_(other.data_),len_(other.len_){}
        Address(Address&& other):
        data_(std::move(other.data_)),len_(std::exchange(other.len_,0)){}
        Address& operator=(const Address& other) noexcept{
            if(this!=&other){
                std::memcpy(data_.data(),other.data_.data(),sizeof(data_));
                len_ = other.len_;
            }
            return *this;
        }
        Address& operator=(Address&& other) noexcept{
            if(this!=&other){
                data_ = std::move(other.data_);
                len_ = std::exchange(other.len_,0);
            }
            return *this;
        }
        Address(const sockaddr_storage& addr,std::error_code& err)
        {
            init_addr_internal(addr,err);
        }
        Address(const sockaddr_in& addr4,std::error_code& err) noexcept{
            err.clear();
            std::memcpy(data_.data(),&addr4,sizeof(sockaddr_in));
            len_ = sizeof(sockaddr_in);
        }
        Address(const sockaddr_in6& addr6,std::error_code& err) noexcept{
            err.clear();
            std::memcpy(data_.data(),&addr6,sizeof(sockaddr_in6));
            len_ = sizeof(sockaddr_in6);
        }
        Family ip_version() const noexcept {
            if (len_ == 0) return Family::UNDEF;
            sa_family_t family;
            std::memcpy(&family, data_.data(), sizeof(family));
            return static_cast<Family>(family);
        }
        const std::byte* data() const noexcept{
            return data_.data();
        }
        std::byte* data() noexcept{
            return data_.data();
        }
        sockaddr* get_sockaddr() noexcept
            { return reinterpret_cast<sockaddr*>(data_.data()); }
        const sockaddr* get_sockaddr() const noexcept
            { return reinterpret_cast<const sockaddr*>(data_.data()); }
        socklen_t length() const noexcept{
            return len_;
        }
        bool valid() const noexcept { return len_ > 0; }
    };

    Address make_address(
        const std::string& host,
        Port port,
        std::error_code& err) noexcept;
}