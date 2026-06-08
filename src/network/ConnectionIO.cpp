#include "connectionIO.h"
#include "abstractworker.h"

namespace network{
    void ConnectionIO::enable_writable(
            bool enable,
            std::error_code& err) noexcept{
        if((!enable && ((*sock_events_)&Event::Out)!=0) ||
            enable && ((*sock_events_)&Event::Out)==0)
            hconn_.owner()->enable_writing(hconn_,enable,err);
        writable = enable;
    }
    void ConnectionIO::enable_readable(
            bool enable,
            std::error_code& err) noexcept{
        if((!enable && ((*sock_events_)&Event::In)!=0) ||
            enable && ((*sock_events_)&Event::In)==0)
            hconn_.owner()->enable_readable(hconn_,enable,err);
    }
    size_t ConnectionIO::read_exact(
            std::span<char> buffer,
            size_t n,
            std::error_code& err) noexcept
    {
        if(n == 0){
            err.clear();
            return 0;
        }
        auto sock = socket_.lock();
        size_t to_receive{n};
        if(buffer.size()<n)
            to_receive = buffer.size();
        if(!sock || !sock->valid()){
            err = std::make_error_code(std::errc::bad_file_descriptor);
            return 0;
        }
        if(buffer.size()==0)
        {
            err = std::make_error_code(std::errc::no_buffer_space);
            enable_readable(false,err);
            return 0;
        }
        if(auto recv_res = ::network::receive(err,
                *sock,buffer,to_receive);
                err!=std::error_code())
        {
            switch(static_cast<std::errc>(err.value())){
                case std::errc::resource_unavailable_try_again:
                case std::errc::operation_in_progress:
                    if(recv_res==to_receive && 
                        to_receive<n){
                        err = std::make_error_code(std::errc::no_buffer_space);
                        enable_readable(false,err);
                    }
                    else err.clear();
                    return recv_res;
                    break;
                case std::errc::no_buffer_space:
                    enable_readable(false,err);
                    return recv_res;
                default:{
                    return recv_res;
                }
            }
        }
        else{
            if(recv_res==to_receive && 
                to_receive<n)
            {
                err = std::make_error_code(std::errc::no_buffer_space);
                enable_readable(false,err);
            }
            else err.clear();
            return recv_res;
        }
    }

    template<>
    void ConnectionIO::send(std::error_code& err,AbstractFrame& frame) noexcept{
        auto sock = socket_.lock();
        if(serialize(frame)!=serialization::SerializationEC::NONE){
            enable_writable(false, err); // попытка отключить, но игнорируем ошибку
            clear_send_buffers();
            err=std::make_error_code(std::errc::operation_canceled);
            return;
        }

        if(!sock || !sock->valid()){
            err = std::make_error_code(std::errc::bad_file_descriptor);
            return;
        }
        if(send_buffer_.has_to_write())
            enable_writable(true,err);
        while (send_buffer_.has_to_write()) {
            auto send_res = ::network::send_vectorized(
                err, *sock, send_buffer_);
            if (err != std::error_code()) {
                switch (static_cast<std::errc>(err.value())) {
                    case std::errc::resource_unavailable_try_again:
                    case std::errc::operation_in_progress:
                        send_buffer_.consume(send_res);
                        err.clear();
                        return; // ждём следующего Out
                    default:{
                        std::error_code tmp_err = err;
                        enable_writable(false, err); // попытка отключить, но игнорируем ошибку
                        clear_send_buffers();
                        err=tmp_err;
                        return; // возвращаем ошибку
                    }
                }
            }
            send_buffer_.consume(send_res);
        }
        if (!send_buffer_.has_to_write()) {
            enable_writable(false, err);
        }
    }
    template<>
    void ConnectionIO::receive(std::error_code& err,AbstractFrame& frame) noexcept
    {
        auto sock = socket_.lock();
        assert(recv_buffer_.capacity()>0);
        if(recv_buffer_.size()>0)
            auto ser_res = deserialize(frame);
        if(!sock || !sock->valid()){
            err = std::make_error_code(std::errc::bad_file_descriptor);
            return;
        }
        while(true){
            if(free_space()==0)
            {
                err = std::make_error_code(std::errc::no_buffer_space);
                enable_readable(false,err);
                return;
            }
            if(auto recv_res = ::network::receive_to_ring_buffer(err,
                    *sock,recv_buffer_);
                    err!=std::error_code())
            {
                switch(static_cast<std::errc>(err.value())){
                    case std::errc::resource_unavailable_try_again:
                    case std::errc::operation_in_progress:
                        err.clear();
                        return;
                        break;
                    case std::errc::no_buffer_space:
                        enable_readable(false,err);
                        return;
                    default:{
                        return;
                    }
                }
            }
            else{
                if(recv_res==0)
                {
                    if(deserialize(frame)==serialization::SerializationEC::NONE){
                        err.clear();
                        return;
                    }
                    else{
                        err = std::make_error_code(std::errc::resource_unavailable_try_again);
                        return;
                    }
                }
                else{
                    err = std::make_error_code(std::errc::resource_unavailable_try_again);
                    return;
                }
            }
        }
    }
}