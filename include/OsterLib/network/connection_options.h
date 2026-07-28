#pragma once
#include <string>
#include <cstdint>
#include <thread>
#include "OsterLib/network/definitions/protocol.h"
#include "OsterLib/network/commonsocket.h"
#include "OsterLib/network/address.h"
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
    struct OptionType{
        static constexpr Socket::Options type = OPT;
        bool operator==(OptionType) noexcept{
            return true;
        }
    };

    bool operator==(const timeval& lhs,
                        const timeval& rhs) noexcept;

    bool operator==(const linger& lhs,
                        const linger& rhs) noexcept;

    template<typename VAL,network::Socket::Options OPT>
    bool operator==(const std::pair<VAL,OptionType<OPT>>& lhs,
                        const std::pair<VAL,OptionType<OPT>>& rhs) noexcept;

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
            
        template<Socket::Options OPT, typename VAL>
        static std::shared_ptr<Socket::BaseOption> make_option(const std::pair<VAL,
            OptionType<OPT>>& opt) noexcept{
            return std::make_shared<Socket::Option<VAL>>(opt.first,opt.second.type);
        }

        std::vector<std::shared_ptr<Socket::BaseOption>> options_sequence() const noexcept{
            std::vector<std::shared_ptr<Socket::BaseOption>> result;
            result.push_back(make_option(reuse_address_));
            result.push_back(make_option(reuse_port_));
            result.push_back(make_option(broadcast_socket_));
            result.push_back(make_option(dont_route_));
            result.push_back(make_option(keep_alive_));
            result.push_back(make_option(linger_));
            result.push_back(make_option(timeout_send_));
            result.push_back(make_option(timeout_input_));
            result.push_back(make_option(expect_min_bytes_available_));
            result.push_back(make_option(buffer_size_in_));
            result.push_back(make_option(buffer_size_out_));
            return result;
        }
        bool operator==(const ConnectionOptions& other) const noexcept;
    };
}

#include "OsterLib/boost_functional/json.h"

template<>
boost::json::value to_json(const network::ConnectionOptions& val);

template<>
std::expected<network::ConnectionOptions,std::exception> 
    from_json(const boost::json::value& val);