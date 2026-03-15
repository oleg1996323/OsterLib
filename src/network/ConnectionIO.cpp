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
}