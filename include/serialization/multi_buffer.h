#pragma once
#include <vector>
#include <span>
#include <cstdint>
#include <cstring>
#include <optional>
#include "definitions.h"
#include "byte_order.h"
#include "float_conv.h"
#include <deque>

namespace serialization{

class MultiBufferView {

    template<typename T,bool NETWORK_ORDER>
    bool __deserialize_trivial__(T& to_deserialize){
        if constexpr (std::is_empty_v<T>)
            return true;
        else if constexpr (numeric_types_concept<T>){
            if constexpr (std::is_integral_v<T> || std::is_enum_v<T>){
                using RawType = serialization::RawType_t<T>;
                RawType value;
                if(read(&value,sizeof(RawType))){
                    if constexpr (sizeof(RawType)>1){
                        if constexpr(NETWORK_ORDER){
                            if(is_little_endian())
                                value = std::byteswap(value);
                        }
                        else{
                            if(!is_little_endian())
                                value = std::byteswap(value);
                        }
                    }
                    to_deserialize = static_cast<T>(value);
                    return true;
                }
                else return false;
            }
            else if constexpr(std::is_floating_point_v<T>){
                using IntType = oster::detail::to_integer_type<sizeof(std::decay_t<T>)>;
                IntType int_val;
                if(__deserialize_trivial__<IntType,NETWORK_ORDER>(int_val)){
                    to_deserialize = to_float(int_val);
                    return true;
                }
                else return false;
            }
            else static_assert(false,"Not implemented");
        }
        else static_assert(false,"Not implemented");
    }

public:
    using Span = std::span<const char>;
    explicit MultiBufferView(std::span<const Span> buffers) noexcept
        : buffers_(buffers.begin(), buffers.end())
    {}
    template<typename... Args>
    requires ((std::ranges::random_access_range<std::decay_t<Args>> &&
           std::ranges::contiguous_range<std::decay_t<Args>>) && ...)
    MultiBufferView(auto&... args){
        (buffers_.push_back(args),...);
    }
    size_t available() const noexcept {
        size_t total = 0;
        for (size_t i = span_idx_; i < buffers_.size(); ++i)
            total += buffers_[i].size();
        total -= offset_;
        return total;
    }

    /**
     * @brief Прочитать ровно n байт в dest.
     * @param dest Указатель на буфер назначения.
     * @param n Количество байт.
     * @return true если данные прочитаны, false если не хватает (позиция не меняется).
     */
    bool read(void* dest, size_t n) noexcept {
        if (available() < n) return false;

        char* dest_ptr = static_cast<char*>(dest);
        size_t remaining = n;
        while (remaining > 0) {
            const Span& cur = buffers_[span_idx_];
            size_t avail_in_cur = cur.size() - offset_;
            size_t to_copy = std::min(remaining, avail_in_cur);
            std::memcpy(dest_ptr, cur.data() + offset_, to_copy);
            dest_ptr += to_copy;
            remaining -= to_copy;
            offset_ += to_copy;
            if (offset_ == cur.size()) {
                ++span_idx_;
                offset_ = 0;
            }
        }
        return true;
    }

    /**
     * @brief Пропустить n байт (без копирования).
     * @return true если удалось, false если не хватает данных.
     */
    bool advance(size_t n) noexcept {
        if (available() < n) return false;

        size_t remaining = n;
        while (remaining > 0) {
            const Span& cur = buffers_[span_idx_];
            size_t avail_in_cur = cur.size() - offset_;
            size_t to_skip = std::min(remaining, avail_in_cur);
            remaining -= to_skip;
            offset_ += to_skip;
            if (offset_ == cur.size()) {
                ++span_idx_;
                offset_ = 0;
            }
        }
        return true;
    }

    /**
     * @brief Создать контрольную точку (сохранить текущую позицию в стеке).
     */
    void checkpoint() noexcept {
        checkpoints_.emplace_back(span_idx_, offset_);
    }

    /**
     * @brief Откатиться к последней контрольной точке.
     * @return true если стек не пуст, false иначе (в этом случае позиция не меняется).
     */
    bool rollback() noexcept {
        if (checkpoints_.empty()) return false;
        auto [saved_idx, saved_offset] = checkpoints_.back();
        span_idx_ = saved_idx;
        offset_ = saved_offset;
        checkpoints_.pop_back();
        return true;
    }

    /**
     * @brief Удалить последнюю контрольную точку (подтвердить успешное чтение).
     * @return true если стек не пуст.
     */
    bool commit() noexcept {
        if (checkpoints_.empty()) return false;
        checkpoints_.pop_back();
        return true;
    }
    void push(auto& buffer) noexcept
    requires (std::ranges::random_access_range<std::decay_t<decltype(buffer)>> &&
        std::ranges::contiguous_range<std::decay_t<decltype(buffer)>>)
    {
        if(!std::empty(buffer))
            buffers_.push_back(std::span(buffer));
    }

    /**
     * @brief Текущая глобальная позиция (общее количество прочитанных байт от начала).
     */
    size_t position() const noexcept {
        size_t pos = 0;
        for (size_t i = 0; i < span_idx_; ++i)
            pos += buffers_[i].size();
        pos += offset_;
        return pos;
    }

    /**
     * @brief Прочитать тривиально копируемый тип.
     */
    template<typename T,bool NETWORK_ORDER>
    bool read_trivial(T& value) noexcept {
        static_assert(std::is_trivially_copyable_v<T>);
        checkpoint();
        if(__deserialize_trivial__<T,NETWORK_ORDER>(value)){
            commit();
            return true;
        }
        else{
            rollback();
            return false;
        }            
    }

    /**
     * @brief Прочитать тривиально копируемый тип и вернуть optional.
     */
    template<typename T,bool NETWORK_ORDER>
    std::optional<T> read_trivial() noexcept {
        static_assert(std::is_trivially_copyable_v<T>);
        T value;
        if (read_trivial<T,NETWORK_ORDER>(&value))
            return value;
        return std::nullopt;
    }

    size_t flush_deserialized() noexcept{
        size_t decrease_offset_{0};
        while(span_idx_>0){
            decrease_offset_+=buffers_.front().size();
            buffers_.pop_front();
            --span_idx_;
        }
        span_idx_=0;
        if(!buffers_.empty())
            buffers_.front()=buffers_.front().subspan(offset_);
        decrease_offset_+=offset_;
        offset_=0;
        return decrease_offset_;
    }

private:
    std::deque<Span> buffers_;
    size_t span_idx_ = 0;
    size_t offset_ = 0;
    std::vector<std::pair<size_t, size_t>> checkpoints_;
};
}