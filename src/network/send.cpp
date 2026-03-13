#include "send.h"

namespace network{
    #ifdef __unix__
    size_t send_vectorized(
                std::error_code& err,
                const Socket& socket,
                VectorizedBuffer& buffer,
                bool stand) noexcept
    {    
        if(auto res = writev(socket.native(),
                buffer.vbuf_.data()+buffer.active_el_,
                buffer.vbuf_.size());res==-1)
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
            if(!stand)
                buffer.consume(res);
            err.clear();
            return res;
        }
    }
    #endif
}