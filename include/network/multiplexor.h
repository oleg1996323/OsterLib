#pragma once
#include "definitions.h"
#include <vector>
#include <utility>
#include <sys/epoll.h>
#include <span>
#include <sys/eventfd.h>
#include <set>
#include <unistd.h>
#include <memory>
#include "multiplexor/eventhandle.h"

namespace network{
    class Connection;
    /**
     * @public Add/remove/modify network::Connection
     */
    class Multiplexor{
        class Interruptor{
            friend Multiplexor;
            eventfd_t fd_=-1;
            Interruptor(eventfd_t fd):fd_(fd){}
            public:
            ~Interruptor(){
                int old = std::exchange(fd_,-1);
                ::close(old);
            }
        };

        class Semaphore{
            friend Multiplexor;
            eventfd_t fd_;
            Semaphore(eventfd_t fd):fd_(fd){}
        };
        //semaphore-interpreter control
        static constexpr uint64_t si_ctrl = static_cast<uint64_t>(0b1)<<32;
        using CustomEvent = std::variant<Interruptor,Semaphore>;

        std::vector<EventHandle> events_;
        std::unique_ptr<Interruptor> interruptor;
        FileDescriptor epollfd = -1;
        void __epoll_ctl_throw__(std::error_code& err);
        void __epoll_wait_throw__(std::error_code& err);
    private:
        bool __set_interruptor__(std::error_code& err);
    public:
        Multiplexor(size_t mp_controled,std::error_code& err) noexcept;
        ~Multiplexor();
        bool add(FileDescriptor fd,
                Event event,
                std::error_code& err) noexcept;
        bool add(FileDescriptor fd,
                EventHandle hevent,std::error_code& err) noexcept;
        bool add(FileDescriptor fd, uint32_t val,
                Event event,std::error_code& err) noexcept;
        bool add(FileDescriptor fd, uint64_t val,
                Event event,std::error_code& err) noexcept;
        bool add(FileDescriptor fd, void* ptr,
                Event event,std::error_code& err) noexcept;
        bool remove(FileDescriptor fd,
                    std::error_code& err) noexcept;
        bool modify(FileDescriptor fd,
            EventHandle hevent,
            std::error_code& err) noexcept;
        bool modify(FileDescriptor fd,
                    Event event,std::error_code& err) noexcept;
        void interrupt() noexcept;
        /**
         * @param timeout - in milliseconds
         * If timeout == -1 than infinite timeout is set.
         */
        std::span<EventHandle> wait(std::error_code& err,int32_t timeout = -1) noexcept;
    };
}