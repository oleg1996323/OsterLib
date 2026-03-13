#pragma once
#include <streambuf>
#include "commonsocket.h"
#include "serialization.h"
#include "connectionhandle.h"
#include "send.h"
#include "receive.h"
#include <vector>
#include <queue>
#include "multiplexor/events.h"
#include "buffers/ring_buffer.h"

namespace network{

class ConnectionIO{
    friend size_t receive(
        std::error_code& err,
        const Socket& socket,
        auto&& buffer,
        uint64_t n,
        RECV_FLAGS flags) noexcept 
        requires(
        (std::ranges::view<std::decay_t<decltype(buffer)>> && 
        std::ranges::random_access_range<std::decay_t<decltype(buffer)>> &&
        std::is_rvalue_reference_v<decltype(buffer)>) || 
        (std::ranges::random_access_range<std::decay_t<decltype(buffer)>> &&
        std::is_lvalue_reference_v<decltype(buffer)>));
    friend size_t send(std::error_code& err,const Socket& socket,SEND_FLAGS flags,
        const std::ranges::random_access_range auto& buffers) noexcept;

    std::weak_ptr<Socket> socket_;
    ConnectionHandle hconn_;
    VectorizedBuffer send_buffer_;
    RingBuffer<char> recv_buffer_;
    const Event* sock_events_;
    public:
    ConnectionIO(
            std::shared_ptr<Socket> socket,
            const ConnectionHandle& hconn,
            const Event* events,
            size_t recv_buf_sz,
            std::error_code& err):
        socket_(socket),
        hconn_(hconn),
        recv_buffer_(recv_buf_sz),
        sock_events_(events)
    {
        assert(events);
        assert(hconn_.is_valid_handler());
        if(!socket || !socket->valid())
            err = std::make_error_code(std::errc::bad_file_descriptor);
        else err.clear();
    }
    void enable_writable(
            bool enable,
            std::error_code& err) noexcept;
    void send(std::error_code& err) noexcept
    {
        send(err,{});
    }
    void send(std::error_code& err,SEND_FLAGS flags) noexcept
    {
        auto sock = socket_.lock();
        if(!sock || !sock->valid()){
            err = std::make_error_code(std::errc::bad_file_descriptor);
            return;
        }
        if(send_buffer_.has_to_write())
            enable_writable(true,err);
        if(err!=std::error_code())
            return;
        while (send_buffer_.has_to_write()) {
            auto send_res = ::network::send_vectorized(
                err, *sock, send_buffer_);
            if (err != std::error_code()) {
                switch (static_cast<std::errc>(err.value())) {
                    case std::errc::resource_unavailable_try_again:
                    case std::errc::operation_in_progress:
                        err.clear();
                        return; // ждём следующего Out
                    default:
                        enable_writable(false, err); // попытка отключить, но игнорируем ошибку
                        clear_send_buffers();
                        return; // возвращаем ошибку
                }
            }
            send_buffer_.consume(send_res);
        }
        if (!send_buffer_.has_to_write()) {
            enable_writable(false, err);
        }
    }
    bool has_to_write() const noexcept{
        return send_buffer_.has_to_write();
    }
    size_t free_space() const noexcept{
        return recv_buffer_.capacity()-recv_buffer_.size();
    }
    void receive(std::error_code& err,
        uint64_t n,
        RECV_FLAGS flags) noexcept
    {
        auto sock = socket_.lock();
        if(!sock || !sock->valid()){
            err = std::make_error_code(std::errc::bad_file_descriptor);
            return;
        }
        while(true){
            if(free_space()==0)
        {
                err = std::make_error_code(std::errc::no_buffer_space);
                return;
            }
            if(auto send_res = ::network::receive_to_ring_buffer(err,
                    *sock,recv_buffer_);
                    err!=std::error_code())
            {
                err = std::make_error_code(static_cast<std::errc>(errno));
                errno = 0;
                switch(static_cast<std::errc>(err.value())){
                    case std::errc::resource_unavailable_try_again:
                    case std::errc::operation_in_progress:
                        err.clear();
                    default:{
                        return;
                    }
                }
            }
            else{
                if(send_res==0){
                    err.clear();
                    return;
                }
                else err.clear();
            }
        }
    }
    void receive(std::error_code& err,uint64_t n) noexcept
    {
        receive(err,n,{});
    }
    size_t received() const noexcept{
        return recv_buffer_.size();
    }
    template<typename T>
    serialization::SerializationEC deserialize(T& value) noexcept{
        if(auto ser_res = serialization::deserialize_network(
                value,
                recv_buffer_);ser_res!=serialization::SerializationEC::NONE){
            clear_recv_buffer(); //flush errorness sequence
            std::cout<<"deserialize error"<<std::endl;
            return ser_res;
        }
        else{
            recv_buffer_state_ = recv_buffer_state_.subspan(serialization::serial_size(value));
            return ser_res;
        }
    }
    template<typename T>
    serialization::SerializationEC serialize(T&& value) noexcept{
        return send_buffer_.serialize(std::forward<T>(value));
    }
    void clear_recv_buffer() noexcept{
        recv_buffer_.clear();
    }
    void clear_send_buffers() noexcept{
        send_buffer_.compact();
    }
    void resize_receive_buffer(size_t bufsiz) noexcept{
        recv_buffer_.set_capacity(bufsiz);
    }
    size_t receive_buffer_size() const noexcept{
        return recv_buffer_.size();
    }
};
}