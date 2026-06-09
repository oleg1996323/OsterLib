#include "buffers/vectorized_buffer.h"

namespace network{
    #ifdef __unix__
    
        void VectorizedBuffer::clear_buffer() noexcept{
            vbuf_.clear();
            bufs_.clear();
            active_el_=0;
        }
        std::pair<iovec*, size_t> VectorizedBuffer::remaining() noexcept{
            if (active_el_ >= vbuf_.size())
                return {nullptr, 0};
            return {&vbuf_[active_el_], bufs_.size() - active_el_};
        }
        
        void VectorizedBuffer::consume(size_t bytes_sent) noexcept{
            while (bytes_sent > 0 && active_el_ < vbuf_.size()) {
                auto& current = vbuf_[active_el_];
                if (bytes_sent >= current.iov_len) {
                        bytes_sent -= current.iov_len;
                        ++active_el_;
                } else {
                        current.iov_base = static_cast<char*>(current.iov_base) + bytes_sent;
                        current.iov_len -= bytes_sent;
                        bytes_sent = 0;
                }
            }
        }
        void VectorizedBuffer::compact() noexcept{
            if (active_el_ > 0) {
                vbuf_.erase(vbuf_.begin(), vbuf_.begin() + active_el_);
                bufs_.erase(bufs_.begin(), bufs_.begin() + active_el_);
                active_el_ = 0;
            }
        }
        bool VectorizedBuffer::has_to_write() const noexcept{
            return active_el_<vbuf_.size();
        }
        bool VectorizedBuffer::empty() const noexcept{
            return bufs_.empty();
        }
    #endif
}