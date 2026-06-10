#include "send.h"

namespace network{
    #ifdef __unix__
    size_t send_vectorized(
                std::error_code& err,
                const Socket& socket,
                VectorizedBuffer& buffer,
                bool stand) noexcept
    {   
        auto remain = buffer.remaining();
        if(remain.first==nullptr)
            return 0;
        if(auto res = writev(socket.native(),
                remain.first,
                remain.second);res==-1)
        {
            err = std::make_error_code(static_cast<std::errc>(errno));
            errno = 0;
            using namespace std;
            switch(static_cast<std::errc>(err.value())){
                case std::errc::resource_unavailable_try_again:
                case std::errc::operation_in_progress:
                //std::cout<<"Error at send_vectorized: "<<err.message()<<std::endl;
                buffer.consume(res);
                if(!stand)
                    buffer.compact();                    
                return 0;
                default:
                    return 0;
            }
            return 0;
        }
        else{
            if(res==0)
                return res;
            buffer.consume(res);
            if(!stand)
                buffer.compact();
            err.clear();
            return res;
        }
    }
    #endif

    #ifdef __unix__
    size_t send_vectorized(
                std::error_code& err,
                const Socket& socket,
                RingBuffer<char>& buffer,
                bool stand) noexcept
    {   
        err.clear();
        auto to_write = buffer.read_vectored();
        if(to_write.first.iov_len==0 && to_write.second.iov_len==0){
            err = std::make_error_code(std::errc::no_buffer_space);
            return 0;
        }
        if(auto res = writev(socket.native(),
                reinterpret_cast<const iovec*>(&to_write),
                2);res==-1)
        {
            err = std::make_error_code(static_cast<std::errc>(errno));
            errno = 0;
            switch(static_cast<std::errc>(err.value())){
                case std::errc::resource_unavailable_try_again:
                case std::errc::operation_in_progress:
                case std::errc::no_buffer_space:
                //std::cout<<"Error at send_vectorized: "<<err.message()<<std::endl;
                if(!stand)
                    buffer.commit_read(res);
                return 0;
                default:
                    return 0;
            }
            return 0;
        }
        else{
            if(res==0)
                return 0;
            if(!stand)
                buffer.commit_read(res);
            err.clear();
            return res;
        }
    }
    #endif
}