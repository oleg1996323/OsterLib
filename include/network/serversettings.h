#pragma once
#include <string>
#include <cstdint>
#include <thread>
#include "definitions/protocol.h"
#include "commonsocket.h"
#include "connection_options.h"
        /* AcceptConnections = SO_ACCEPTCONN,  //Socket is accepting connections.
        BroadCast = SO_BROADCAST,           //Transmission of broadcast messages is supported.
        Debug = SO_DEBUG,                   //Debugging information is being recorded.
        DontRoute = SO_DONTROUTE,           //Bypass normal routing.
        ErrorState = SO_ERROR,              //Socket error status.
        KeepAlive = SO_KEEPALIVE,           //Connections are kept alive with periodic messages.
        Lingers = SO_LINGER,                //Socket lingers on close.
        OutOfBand = SO_OOBINLINE,           //Out-of-band data is transmitted in line.
        BufferSizeIn = SO_RCVBUF,           //Receive buffer size.
        LowWaterMarkIn = SO_RCVLOWAT,       //Receive ``low water mark''.
        TimeOutIn = SO_RCVTIMEO,            //Receive timeout.
        ReuseAddress = SO_REUSEADDR,        //Reuse of local addresses is supported.
        BufferSizeOut = SO_SNDBUF,          //Send buffer size.
        LowWaterMarkOut = SO_SNDLOWAT,      //Send ``low water mark''.
        TimeOutOut = SO_SNDTIMEO,           //Send timeout.
        SocketType = SO_TYPE                //Socket type. */
namespace network::server{
struct Settings{

    std::string host_;
    std::string service_;
    Protocol protocol_ = Protocol::TCP;
    Timeout timeout_seconds_processes_=30;
    Port port_{0};
    uint32_t num_threads_pool_=std::thread::hardware_concurrency();
    uint32_t number_events_{10};
    ConnectionOptions options_ = {};
    
    Settings() = default;
    Settings(
        std::string host,
        std::string service,
        Protocol proto,
        int timeout,
        int32_t port,
        ConnectionOptions options,
        uint32_t num_threads_pool=std::thread::hardware_concurrency()):
            host_(host),
            service_(service),
            protocol_(proto),
            timeout_seconds_processes_(timeout),
            port_(port),
            num_threads_pool_(num_threads_pool),
            options_(std::move(options)){}
    Settings(const Settings& other)
    {
        if(this!=&other){
            host_=other.host_;
            service_=other.service_;
            port_=other.port_;
            num_threads_pool_=other.num_threads_pool_;
            protocol_=other.protocol_;
            timeout_seconds_processes_=other.timeout_seconds_processes_;
            std::memcpy(&options_,&other.options_,sizeof(options_));
        }
    }
    Settings(Settings&& other)
    {   
        if(this!=&other){
            host_=std::move(other.host_);
            service_=std::move(other.service_);
            port_=other.port_;
            protocol_=other.protocol_;
            timeout_seconds_processes_=other.timeout_seconds_processes_;
            num_threads_pool_=other.num_threads_pool_;
            std::memmove(&options_,&other.options_,sizeof(options_));
        }
    }
    Settings& operator=(const Settings& other){
        if(this!=&other){
            host_=other.host_;
            service_=other.service_;
            port_=other.port_;
            protocol_=other.protocol_;
            timeout_seconds_processes_=other.timeout_seconds_processes_;
            num_threads_pool_=other.num_threads_pool_;
            std::memcpy(&options_,&other.options_,sizeof(options_));
        }
        return *this;
    }
    Settings& operator=(Settings&& other){
        if(this!=&other){
            host_.swap(other.host_);
            service_.swap(other.service_);
            port_ = other.port_;
            protocol_ = other.protocol_;
            timeout_seconds_processes_=other.timeout_seconds_processes_;
            num_threads_pool_=other.num_threads_pool_;
            std::memmove(&options_,&other.options_,sizeof(options_));
        }
        return *this;
    }  
};
}

#include "boost_functional/json.h"

template<>
boost::json::value to_json(const network::server::Settings& val);

template<>
std::expected<network::server::Settings,std::exception> from_json(const boost::json::value& val);