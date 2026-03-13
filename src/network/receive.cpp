#include "receive.h"

namespace network{
    #ifdef __unix__
    size_t receive_to_ring_buffer(
          std::error_code& err,
          const Socket& socket,
          RingBuffer<char>& buffer) noexcept
    {
        if(auto res = readv(socket.native(),
                reinterpret_cast<const iovec*>(&buffer.read_vectored()),
                2);res==-1)
        {
            err = std::make_error_code(static_cast<std::errc>(errno));
            errno = 0;
            using namespace std;
            switch(static_cast<std::errc>(err.value())){
                case std::errc::resource_unavailable_try_again:
                case std::errc::operation_in_progress:
                    return 0;
                default:
                        return 0;
            }
            return 0;
        }
        else{
            if(res==0)
                return res;
            buffer.commit_read(res);
            err.clear();
            return res;
        }
    }
    #endif
}