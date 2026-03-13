#pragma once
#include "network/commonsocket.h"
#include "serialization.h"
#include <system_error>
#ifdef __unix__
#include <sys/uio.h>
#endif
#include <deque>
#include <cstdint>

namespace network{

#ifdef __unix__
    class VectorizedBuffer{
        std::vector<iovec> vbuf_;
        std::deque<std::vector<char>> bufs_;
        uint32_t active_el_ = 0;
        const iovec* native() const noexcept{
            return vbuf_.data();
        }
        friend size_t send_vectorized(
                std::error_code& err,
                const Socket& socket,
                VectorizedBuffer& buffer,
                bool stand) noexcept;
        public:
        VectorizedBuffer(){}
        void push_buffer(const std::ranges::random_access_range 
                auto&& range) noexcept{
            bufs_.push_back(std::forward<decltype(range)>(range));
        }
        template<typename T>
        serialization::SerializationEC serialize(T&& value) noexcept{
            serialization::SerializationEC ser_res;
            if(ser_res = serialization::serialize_network(std::forward<T>(value),
                bufs_.emplace_back());ser_res!=serialization::SerializationEC::NONE)
                bufs_.pop_back();
            else
                vbuf_.emplace_back(iovec{.iov_base=bufs_.back().data(),
                        .iov_len=bufs_.back().size()});
            return ser_res;
        }
        void clear_buffer() noexcept;
        std::pair<iovec*, size_t> remaining() noexcept;
        void consume(size_t bytes_sent) noexcept;
        void compact() noexcept;
        bool has_to_write() const noexcept;
        bool empty() const noexcept;
    };
#endif
}