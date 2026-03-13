#pragma once
#include <vector>
#include <span>
#include <iterator>
#include <cstddef>
#ifdef __unix__
#include <sys/uio.h>
#endif

namespace network {

template<typename T>
class RingBuffer;

template<typename T>
class RingBufferIterator {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = T*;
    using reference         = T&;

    RingBufferIterator(RingBuffer<T>* buf, size_t logical_index)
        : buffer_(buf), logical_index_(logical_index) {}
    reference operator*() const {
        return buffer_->data_at(logical_index_);
    }
    pointer operator->() const {
        return &**this;
    }
    RingBufferIterator& operator++() {
        ++logical_index_;
        return *this;
    }
    RingBufferIterator operator++(int) {
        auto tmp = *this;
        ++*this;
        return tmp;
    }
    RingBufferIterator& operator--() {
        --logical_index_;
        return *this;
    }
    RingBufferIterator operator--(int) {
        auto tmp = *this;
        --*this;
        return tmp;
    }
    RingBufferIterator& operator+=(difference_type n) {
        logical_index_ += n;
        return *this;
    }
    RingBufferIterator operator+(difference_type n) const {
        auto tmp = *this;
        tmp += n;
        return tmp;
    }
    friend RingBufferIterator operator+(difference_type n, const RingBufferIterator& it) {
        return it + n;
    }
    RingBufferIterator& operator-=(difference_type n) {
        logical_index_ -= n;
        return *this;
    }
    RingBufferIterator operator-(difference_type n) const {
        auto tmp = *this;
        tmp -= n;
        return tmp;
    }
    difference_type operator-(const RingBufferIterator& other) const {
        return static_cast<difference_type>(logical_index_ - other.logical_index_);
    }
    reference operator[](difference_type n) const {
        return *(*this + n);
    }
    bool operator==(const RingBufferIterator& other) const {
        return logical_index_ == other.logical_index_;
    }
    bool operator!=(const RingBufferIterator& other) const {
        return !(*this == other);
    }
    bool operator<(const RingBufferIterator& other) const {
        return logical_index_ < other.logical_index_;
    }
    bool operator<=(const RingBufferIterator& other) const {
        return logical_index_ <= other.logical_index_;
    }
    bool operator>(const RingBufferIterator& other) const {
        return logical_index_ > other.logical_index_;
    }
    bool operator>=(const RingBufferIterator& other) const {
        return logical_index_ >= other.logical_index_;
    }

private:
    RingBuffer<T>* buffer_;
    size_t logical_index_;
};

template<typename T>
class RingBuffer {
public:
    using value_type = T;
    using iterator = RingBufferIterator<T>;
    using const_iterator = RingBufferIterator<const T>;

    RingBuffer() = default;
    explicit RingBuffer(size_t capacity) {
        set_capacity(capacity);
    }
    void set_capacity(size_t n) {
        buffer_.resize(n);
        head_ = tail_ = 0;
        full_ = false;
    }
    size_t capacity() const noexcept { return buffer_.size(); }
    size_t size() const noexcept {
        if (full_)
            return capacity();
        if (tail_ >= head_)
            return tail_ - head_;
        else
            return capacity() - head_ + tail_;
    }
    bool empty() const noexcept { return !full_ && (head_ == tail_); }
    bool full() const noexcept { return full_; }

    T& data_at(size_t logical_index) {
        size_t phys = (head_ + logical_index) % capacity();
        return buffer_[phys];
    }
    const T& data_at(size_t logical_index) const {
        size_t phys = (head_ + logical_index) % capacity();
        return buffer_[phys];
    }

    iterator begin() { return iterator(this, 0); }
    iterator end()   { return iterator(this, size()); }

    const_iterator begin() const { return const_iterator(const_cast<RingBuffer*>(this), 0); }
    const_iterator end() const   { return const_iterator(const_cast<RingBuffer*>(this), size()); }

    // Добавление одного символа (возвращает false, если буфер полон)
    bool push(T c) {
        if (full_)
            return false;
        buffer_[tail_] = c;
        tail_ = (tail_ + 1) % capacity();
        full_ = (tail_ == head_);
        return true;
    }

    // Удаление одного символа (возвращает false, если буфер пуст)
    bool pop() {
        if (empty())
            return false;
        head_ = (head_ + 1) % capacity();
        full_ = false;
        return true;
    }

    // Очистка буфера
    void clear() {
        head_ = tail_ = 0;
        full_ = false;
    }

    // ----- Методы для vectored I/O -----

    // Возвращает iovec для свободной области (куда можно писать)
    // Может быть до двух сегментов (из-за кольцевости)
    std::pair<struct iovec, struct iovec> write_vectored() const {
        std::pair<struct iovec, struct iovec> result{};
        if (full_) {
            // нет свободного места
            return result;
        }
        size_t cap = capacity();
        size_t free_start = tail_;
        size_t free_end = head_; // свободное место до head_, но с учётом кольца

        if (head_ <= tail_) {
            // свободное место: от tail_ до конца буфера
            result.first.iov_base = const_cast<T*>(buffer_.data()) + tail_;
            result.first.iov_len = cap - tail_;
            // возможно второй сегмент от начала до head_ (если head_ > 0 и есть место)
            if (head_ > 0) {
                result.second.iov_base = const_cast<T*>(buffer_.data());
                result.second.iov_len = head_;
            }
        } else {
            // head_ > tail_: свободное место непрерывно от tail_ до head_ - 1
            result.first.iov_base = const_cast<T*>(buffer_.data()) + tail_;
            result.first.iov_len = head_ - tail_;
            // второго сегмента нет
        }
        return result;
    }

    // Возвращает iovec для занятой области (данные для отправки)
    std::pair<struct iovec, struct iovec> read_vectored() const {
        std::pair<struct iovec, struct iovec> result{};
        if (empty()) {
            return result;
        }
        size_t cap = capacity();
        if (head_ < tail_) {
            // непрерывный участок от head_ до tail_ - 1
            result.first.iov_base = const_cast<T*>(buffer_.data()) + head_;
            result.first.iov_len = tail_ - head_;
        } else if (head_ > tail_ || full_) {
            // два сегмента: от head_ до конца и от начала до tail_ - 1
            result.first.iov_base = const_cast<T*>(buffer_.data()) + head_;
            result.first.iov_len = cap - head_;
            result.second.iov_base = const_cast<T*>(buffer_.data());
            result.second.iov_len = tail_;
        } else {
            // head_ == tail_ и не full? уже empty, но мы уже проверили empty()
        }
        return result;
    }

    // Уведомить, что записано n байт в свободную область (после readv)
    void commit_write(size_t n) {
        if (n == 0) return;
        // Предполагается, что n не превышает свободного места
        tail_ = (tail_ + n) % capacity();
        full_ = (tail_ == head_);
    }

    // Уведомить, что прочитано (отправлено) n байт из занятой области (после writev)
    void commit_read(size_t n) {
        if (n == 0) return;
        head_ = (head_ + n) % capacity();
        full_ = false; // после чтения уже не полный
    }

private:
    std::vector<T> buffer_;
    size_t head_ = 0;      // индекс первого байта для чтения
    size_t tail_ = 0;      // индекс первого свободного байта для записи
    bool full_ = false;    // отличает полный буфер от пустого при head_ == tail_
};

} // namespace network