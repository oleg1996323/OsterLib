#pragma once
#include <string>
#include <cstdint>
#include <thread>
#include "definitions/protocol.h"
#include "commonsocket.h"
#include "address.h"
#include "connection_options.h"
#include "definitions.h"

namespace network::client{
struct Settings{
    std::optional<Address> binded_addr_;
    Timeout timeout_seconds_processes_=30;
    uint32_t num_threads_pool_=std::thread::hardware_concurrency();
    uint32_t number_events_{10};
    ConnectionOptions options_ = {};
    Protocol protocol_ = Protocol::TCP;
    
    Settings() = default;
    Settings(
        Protocol proto,
        Timeout timeout,
        int32_t port,
        ConnectionOptions options,
        uint32_t num_threads_pool=std::thread::hardware_concurrency()):
            timeout_seconds_processes_(timeout),
            num_threads_pool_(num_threads_pool),
            options_(std::move(options)),
            protocol_(proto){}
    Settings(const Settings& other)
    {
        if(this!=&other){
            num_threads_pool_=other.num_threads_pool_;
            protocol_=other.protocol_;
            timeout_seconds_processes_=other.timeout_seconds_processes_;
            std::memcpy(&options_,&other.options_,sizeof(options_));
        }
    }
    Settings(Settings&& other)
    {   
        if(this!=&other){
            protocol_=other.protocol_;
            timeout_seconds_processes_=other.timeout_seconds_processes_;
            num_threads_pool_=other.num_threads_pool_;
            std::memmove(&options_,&other.options_,sizeof(options_));
        }
    }
    Settings& operator=(const Settings& other){
        if(this!=&other){
            protocol_=other.protocol_;
            timeout_seconds_processes_=other.timeout_seconds_processes_;
            num_threads_pool_=other.num_threads_pool_;
            std::memcpy(&options_,&other.options_,sizeof(options_));
        }
        return *this;
    }
    Settings& operator=(Settings&& other){
        if(this!=&other){
            protocol_ = other.protocol_;
            timeout_seconds_processes_=other.timeout_seconds_processes_;
            num_threads_pool_=other.num_threads_pool_;
            std::memmove(&options_,&other.options_,sizeof(options_));
        }
        return *this;
    }
    bool set_bind_address(const Address& addr) noexcept{
        if(addr.valid()){
            binded_addr_.emplace(addr);
            return true;
        }
        else return false;
    }
    bool operator==(const Settings& other) const noexcept{
        return  binded_addr_==other.binded_addr_ &&
                timeout_seconds_processes_==other.timeout_seconds_processes_&&
                num_threads_pool_==other.num_threads_pool_&&
                number_events_ == other.number_events_&&
                options_ ==other.options_&&
                protocol_ == other.protocol_;
    }
};
}

#include "boost_functional/json.h"

template<>
boost::json::value to_json(const network::client::Settings& val);

template<>
std::expected<network::client::Settings,std::exception> from_json(const boost::json::value& val);