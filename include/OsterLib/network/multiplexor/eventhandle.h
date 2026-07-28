#pragma once
#include <sys/epoll.h>
#include "OsterLib/network/definitions.h"
#include "events.h"
#include <variant>

namespace network{
    class EventHandle{
        public:
        EventHandle() = default;
        explicit constexpr EventHandle(epoll_event ev) noexcept:
        ev_(ev){}
        explicit constexpr EventHandle(
            FileDescriptor fd,
            Event events) noexcept
        {
            ev_.data.fd = fd;
            ev_.events = events;
        }
        explicit constexpr EventHandle(
            uint32_t value,
            Event events) noexcept
        {
            ev_.events = events;
            ev_.data.u32 = value;
        }
        explicit constexpr EventHandle(
            uint64_t value,
            Event events) noexcept
        {
            ev_.events = events;
            ev_.data.u64 = value;
        }
        explicit constexpr EventHandle(
            void* ptr,
            Event events) noexcept
        {
            ev_.events = events;
            ev_.data.ptr = ptr;
        }
        template<typename T>
        constexpr T* get_as_pointer() const noexcept{
            return reinterpret_cast<T*>(ev_.data.ptr);
        }
        constexpr uint64_t get_as_64() const noexcept{
            return ev_.data.u64;
        }
        constexpr uint32_t get_as_32() const noexcept{
            return ev_.data.u32;
        }
        constexpr FileDescriptor get_as_fd() const noexcept{
            return ev_.data.fd;
        }
        constexpr Event events() const noexcept{
            return static_cast<Event>(ev_.events);
        }
        constexpr void set_as_fd(FileDescriptor value) noexcept{
            ev_.data.u64 = 0;
            ev_.data.fd = value;
        }
        constexpr void set_as_64(uint64_t value) noexcept{
            ev_.data.u64 = value;
        }
        constexpr void set_as_32(uint32_t value) noexcept{
            ev_.data.u64 = 0;
            ev_.data.u32 = value;
        }
        template<typename T>
        constexpr void set_as_ptr(T* ptr) noexcept{
            ev_.data.ptr = ptr;
        }
        constexpr void set_events(Event events) noexcept{
            ev_.events = events;
        }
        epoll_event* native() noexcept{
            return &ev_;
        }
        const epoll_event* native() const noexcept{
            return &ev_;
        }
        private:
        epoll_event ev_;
    };
}