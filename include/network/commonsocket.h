#pragma once
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <memory>
#include <stdexcept>
#include "definitions.h"
#include <cstring>
#include <sys/un.h>
#include <cassert>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <type_traits>
#include <atomic>
#include "address.h"

namespace network{

enum class SEND_FLAGS;
enum class RECV_FLAGS;

class CommonServer;
class Multiplexor;
class AbstractWorker;
class AbstractServer;
class ConnectionAcceptor;
class Connection;

class Socket{
    public:
    enum Options{
        AcceptConnections = SO_ACCEPTCONN,  //Socket is accepting connections.
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
        ReusePort = SO_REUSEPORT,           //Permits multiple AF_INET or AF_INET6 sockets to be bound to
                                            //an identical socket address. 
        BufferSizeOut = SO_SNDBUF,          //Send buffer size.
        LowWaterMarkOut = SO_SNDLOWAT,      //Send ``low water mark''.
        TimeOutOut = SO_SNDTIMEO,           //Send timeout.
        SocketType = SO_TYPE                //Socket type.
    };
    enum Type{
        Datagramm = SOCK_DGRAM,             //Datagram socket.
        Raw = SOCK_RAW,                     //Raw Protocol Interface.
        SequencedPack = SOCK_SEQPACKET,     //Sequenced-packet socket.
        Stream = SOCK_STREAM,               //Byte-stream socket.
        NonBlock = SOCK_NONBLOCK            //Non-block socket type         
    };
    struct BaseOption{
        virtual bool assign(Socket& socket,std::error_code& err) noexcept = 0;
        virtual bool set_option(Socket& socket,std::error_code& err) noexcept = 0;
        ~BaseOption() = default;
    };

    template<typename T>
    struct Option:BaseOption{
        T value_;
        Options opt_;
        Option() = default;
        Option(const T& value,
            Options option):
            value_(value),opt_(option){}
        Option(T&& value,
            Options option) noexcept:
            value_(std::forward<T>(value)),opt_(option){}
        virtual bool set_option(Socket& socket,std::error_code& err) noexcept override;
        virtual bool assign(Socket& socket,std::error_code& err) noexcept override;
    };
    private:
    friend class ConnectionAcceptor;
    friend class Connection;
    friend Socket socket(
            const Address& storage,
            Socket::Type type,
            Protocol proto,
            std::error_code& err)
            noexcept;
    int fd_{-1};
    std::atomic<bool> closed_ = true;
    
    int fd() const{
        return fd_;
    }
    void bind(std::error_code& err) noexcept;
    Socket(int raw_socket_id) noexcept;
    public:
    Socket& operator=(Socket&& other) noexcept;
    Socket(Socket&& other) noexcept;
    Socket& operator=(const Socket& other) = delete;
    Socket(const Socket& other) = delete;
    bool operator==(const Socket& other) const noexcept
    {return fd_==other.fd_;}
    virtual ~Socket(){
        close();
    }
    bool bind(const Address& addr,std::error_code& err) noexcept;
    bool set_no_block(bool noblock,std::error_code& err) noexcept;
    bool is_non_block(std::error_code& err) const noexcept;
    int native() const noexcept { return fd_; }
    public:
    bool valid() const noexcept
    { 
        return !closed_.load(
            std::memory_order::relaxed) && 
            fd_ >= 0;
    }
    void close() noexcept {
        if (fd_ >= 0) {
            closed_.store(
                true,
                std::memory_order::release
            );
            int old = std::exchange(fd_,-1);
            ::close(old);
        }
    }
    std::error_code error(std::error_code& err) noexcept{
        int err_conn = 0;
        socklen_t len = sizeof(err_conn);
        if(getsockopt(native(),
                SOL_SOCKET, SO_ERROR, &err_conn, &len)==-1)
        {
            err = std::make_error_code(static_cast<std::errc>(errno));
            errno = 0;
            std::cout<<"Error at getting socket error"<<err.message()<<std::endl;
            return err;
        }
        else{
            err.clear();
            if(err_conn!=0)   
                return std::make_error_code(static_cast<std::errc>(err_conn));
            else return {};
        }
    }
    bool set_option(const std::shared_ptr<Socket::BaseOption>& option,std::error_code& err) noexcept{
        if(option)
            return option->set_option(*this,err);
        else return false;
    }
    bool set_options(std::error_code& err,const std::span<
                const std::shared_ptr<Socket::BaseOption>> options) noexcept{
        for(auto& option:options){
            if(!set_option(option,err))
                return false;
            else continue;
        }
        err.clear();
        return true;
    }
    bool get_option(std::error_code& err,std::shared_ptr<Socket::BaseOption> option) noexcept{
        if(option)
            return option->assign(*this,err);
        else return false;
    }
    bool shutdown_read(std::error_code& err) noexcept{
        bool result = ::shutdown(fd_,SHUT_RD)==0;
        if(errno!=0){
            err = std::make_error_code(static_cast<std::errc>(errno));
            errno=0;
        }
        return result;
    }
    bool shutdown_write(std::error_code& err) noexcept{
        bool result = ::shutdown(fd_,SHUT_WR)==0;
        if(errno!=0){
            err = std::make_error_code(static_cast<std::errc>(errno));
            errno=0;
        }
        return result;
    }
    bool shutdown_all(std::error_code& err) noexcept{
        bool result = ::shutdown(fd_,SHUT_RDWR)==0;
        if(errno!=0){
            err = std::make_error_code(static_cast<std::errc>(errno));
            errno=0;
        }
        return result;
    }
};
template<typename T>
bool Socket::Option<T>::set_option(Socket& socket,std::error_code& err) noexcept{
    int fd_ = socket.native();
    if(fd_>=0){
        if(setsockopt(fd_,SOL_SOCKET,static_cast<int>(opt_),
            (const char*)&value_,sizeof(T))!=0)
        {
            err = std::make_error_code(static_cast<std::errc>(errno));
            std::cout<<"SockOpt: "<<err.message()<<std::endl;
            errno = 0;
            return false;
        }
        return true;
    }
    else{
        err = std::make_error_code(std::errc::not_a_socket);
        return false;
    }
}

template<typename T>
bool Socket::Option<T>::assign(Socket& socket,std::error_code& err) noexcept{
    int fd_ = socket.native();
    if(fd_>=0){
        socklen_t sz = sizeof(T);
        if(getsockopt(fd_,SOL_SOCKET,static_cast<int>(opt_),
            (char*)&value_,&sz)!=0)
        {
            err = std::make_error_code(static_cast<std::errc>(errno));
            std::cout<<"SockOpt: "<<err.message()<<std::endl;
            errno = 0;
            return false;
        }
        return true;
    }
    else{
        err = std::make_error_code(std::errc::not_a_socket);
        return false;
    }
}

Socket socket(
    const Address& storage,
    Socket::Type type,
    Protocol proto,
    std::error_code& err)
    noexcept;
}

network::Socket::Options operator|(
        network::Socket::Options lhs,
        network::Socket::Options rhs) noexcept;
network::Socket::Options operator&(
        network::Socket::Options lhs,
        network::Socket::Options rhs) noexcept;
network::Socket::Options operator^(
        network::Socket::Options lhs,
        network::Socket::Options rhs) noexcept;
network::Socket::Options operator~(
        network::Socket::Options val) noexcept;
network::Socket::Options operator&(
        network::Socket::Options lhs,
        int rhs) noexcept;
network::Socket::Options operator&(
        std::underlying_type_t<network::Socket::Options> lhs,
        network::Socket::Options rhs) noexcept;
network::Socket::Options operator|(
        network::Socket::Options lhs,
        std::underlying_type_t<network::Socket::Options> rhs) noexcept;
network::Socket::Options operator|(
        std::underlying_type_t<network::Socket::Options> lhs,
        network::Socket::Options rhs) noexcept;
network::Socket::Type operator|(
        network::Socket::Type lhs,
        network::Socket::Type rhs) noexcept;
network::Socket::Type operator&(
        network::Socket::Type lhs,
        network::Socket::Type rhs) noexcept;
network::Socket::Type operator^(
        network::Socket::Type lhs,
        network::Socket::Type rhs) noexcept;
network::Socket::Type operator~(
        network::Socket::Type val) noexcept;
network::Socket::Type operator&(
        network::Socket::Type lhs,int rhs) noexcept;
network::Socket::Type operator&(
        std::underlying_type_t<network::Socket::Type> lhs,
        network::Socket::Type rhs) noexcept;
network::Socket::Type operator|(
        network::Socket::Type lhs,
        std::underlying_type_t<network::Socket::Type> rhs) noexcept;
network::Socket::Type operator|(
        std::underlying_type_t<network::Socket::Type> lhs,
        network::Socket::Type rhs) noexcept;
template<>
struct std::equal_to<network::Socket>{
    using is_transparent = std::true_type;
    bool operator()(const network::Socket& lhs,const network::Socket& rhs) const{
        return lhs.native()==rhs.native();
    }
    bool operator()(const network::Socket& socket,int raw_socket) const{
        return socket.native()==raw_socket;
    }
    bool operator()(int raw_socket,const network::Socket& socket) const{
        return socket.native()==raw_socket;
    }
};
template<>
struct std::hash<network::Socket>{
    using is_transparent = std::true_type;
    size_t operator()(const network::Socket& socket) const{
        return socket.native();
    }
    size_t operator()(int raw_socket) const{
        return raw_socket;
    }
};