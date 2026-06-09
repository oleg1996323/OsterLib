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
        friend size_t send_to_ring_buffer(
                std::error_code& err,
                const Socket& socket,
                VectorizedBuffer& buffer,
                bool stand) noexcept;
        public:
        VectorizedBuffer(){}
        void push_buffer(const std::vector<char>& range) noexcept{
            if(std::empty(range))
                return;
            auto& pushed = bufs_.emplace_back(range);
            vbuf_.push_back(iovec{.iov_base=pushed.data(),.iov_len=pushed.size()});
        }
        void push_buffer(std::vector<char>&& range) noexcept{
            if(std::empty(range))
                return;
            auto& pushed = bufs_.emplace_back(std::move(range));
            vbuf_.push_back(iovec{.iov_base=pushed.data(),.iov_len=pushed.size()});
        }
        template<typename T>
        serialization::SerializationEC serialize(T&& value) noexcept{
            std::vector<char> buffer;
            if(auto ser_res = serialization::serialize_network(std::forward<T>(value),
                buffer);ser_res!=serialization::SerializationEC::NONE)
                return ser_res;
            else{
                push_buffer(std::move(buffer));
                return ser_res;
            }   
        }
        template<typename... ARGS>
        requires (sizeof...(ARGS)>1)
        serialization::SerializationEC serialize(ARGS&&... values) noexcept{
            std::vector<char> buffer;
            serialization::SerializationEC err = serialization::SerializationEC::NONE;
            (((err=serialization::serialize_network(std::forward<ARGS>(values), buffer))==
                serialization::SerializationEC::NONE) && ...);
            if(err!=serialization::SerializationEC::NONE)
                return err;
            else{
                push_buffer(std::move(buffer));
                return err;
            }   
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