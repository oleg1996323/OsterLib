#pragma once
#include <sys/epoll.h>

namespace network{
    enum Event:uint32_t{
        /**
         * @brief The associated file is available for read() operations.
         */            
        In = EPOLLIN,
        /**
         * @brief The associated file is available for write() operations.
         */
        Out = EPOLLOUT,
        /**
         * @brief There is an exceptional condition on the file descriptor.
         * @details There is some exceptional condition on the file descriptor.
            @Possibilities include:
            -There is out-of-band data on a TCP socket (see tcp(7)).
            -A pseudoterminal master in packet mode has seen a state
            change on the slave (see ioctl_tty(2)).
            -A cgroup.events file has been modified (see cgroups(7)).
            */
        HighProrityIn = EPOLLPRI,
        /**
         * @brief Requests edge-triggered notification for the associated
             file descriptor.  The default behavior for epoll is level-
            triggered.  See epoll() for more detailed information
            about edge-triggered and level-triggered notification.
            */
        EdgeTrigger = EPOLLET,
        /**
         * @brief Only once the file-descriptor's event will be notified.
         * Further event-notifying of the fd needs the rearming by modify() method.
         */
        OneShot = EPOLLONESHOT,
        Error = EPOLLERR,
        #ifdef EPOLLWAKEUP
        /**
         * @brief Ensure that the system does not enter "suspend" or "hibernate" while this event is
             pending or being processed.
            */
        WakeUp = EPOLLWAKEUP,
        #endif
        #ifdef EPOLLEXCLUSIVE
        /**
         * @brief In multi-threads/processes applications exclude the event notifying
         * in all multiplexors (epoll).
         */
        Exclusive = EPOLLEXCLUSIVE,
        #endif
        /**
         * @brief 
         */
        HangUp = EPOLLHUP,
        CanReadButHangUp = EPOLLRDHUP
    };
}

#include <type_traits>

network::Event operator|(
        network::Event lhs,
        network::Event rhs) noexcept;
network::Event operator&(
        network::Event lhs,
        network::Event rhs) noexcept;
network::Event operator^(
        network::Event lhs,
        network::Event rhs) noexcept;
network::Event operator~(
        network::Event val) noexcept;
network::Event operator&(
        network::Event lhs,
        int rhs) noexcept;
network::Event operator&(
        std::underlying_type_t<network::Event> lhs,
        network::Event rhs) noexcept;
network::Event operator|(
        network::Event lhs,
        std::underlying_type_t<network::Event> rhs) noexcept;
network::Event operator|(
        std::underlying_type_t<network::Event> lhs,
        network::Event rhs) noexcept;