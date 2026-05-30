#pragma once
#include <string>
#include <cstdint>
#include <thread>
#include "definitions/protocol.h"
#include "commonsocket.h"
#include "address.h"
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
namespace network{
    template<network::Socket::Options OPT>
    struct OptionType{};

    struct ConnectionOptions{
        std::pair<int,
            OptionType<Socket::ReuseAddress>> reuse_address_ = {false,{}};
        std::pair<int,
            OptionType<Socket::ReusePort>> reuse_port_ = {false,{}};
        std::pair<int,OptionType<Socket::BroadCast>> broadcast_socket_ = {false,{}};
        std::pair<int,OptionType<Socket::DontRoute>> dont_route_ = {false,{}};
        std::pair<int,OptionType<Socket::KeepAlive>> keep_alive_ = {false,{}};
        std::pair<linger,OptionType<Socket::Lingers>> linger_ = {linger{.l_onoff=false,
                            .l_linger=-1},{}};
        std::pair<timeval,OptionType<Socket::TimeOutOut>> timeout_send_{};
        std::pair<timeval,OptionType<Socket::TimeOutIn>> timeout_input_{};
        std::pair<int,OptionType<Socket::LowWaterMarkIn>> expect_min_bytes_available_={1,{}};
        std::pair<int,OptionType<Socket::BufferSizeIn>> buffer_size_in_{1024,{}};
        std::pair<int,OptionType<Socket::BufferSizeOut>> buffer_size_out_{1024,{}};
    };
}

#include "boost_functional/json.h"

template<>
boost::json::value to_json(const network::ConnectionOptions& val);

template<>
std::expected<network::ConnectionOptions,std::exception> 
    from_json(const boost::json::value& val);