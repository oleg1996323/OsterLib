#include "receive.h"

namespace network{
    #ifdef __unix__
    size_t receive_to_ring_buffer(
          std::error_code& err,
          const Socket& socket,
          RingBuffer<char>& buffer) noexcept
    {
        err.clear();
        auto to_read = buffer.write_vectored();
        if(to_read.first.iov_len==0 && to_read.second.iov_len==0){
            err = std::make_error_code(std::errc::no_buffer_space);
            return 0;
        }
        if(auto res = readv(socket.native(),
                reinterpret_cast<const iovec*>(&to_read),
                2);res==-1)
        {
            err = std::make_error_code(static_cast<std::errc>(errno));
            errno = 0;
            if(err == std::errc::resource_unavailable_try_again ||
                err == std::errc::operation_in_progress){
                    err.clear();
                    return 0;
            }
            else return 0;
        }
        else{
            if (res == 0) {
                err = std::make_error_code(std::errc::connection_reset);
                return 0;
            }
            buffer.commit_write(res);
            err.clear();
            return res;
        }
    }
    #endif
}