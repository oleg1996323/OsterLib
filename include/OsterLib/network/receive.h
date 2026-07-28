#pragma once
#include <span>
#include <vector>
#include <ranges>
#include <type_traits>
#include <netdb.h>
#include "OsterLib/network/definitions.h"
#include "OsterLib/network/commonsocket.h"
#include <thread>
#include "OsterLib/network/buffers/ring_buffer.h"
namespace network{
    enum class RECV_FLAGS: int{
        /**
         * (recvmsg() only; since Linux 2.6.23)
            Set the close-on-exec flag for the file descriptor received
            via a UNIX domain file descriptor using the SCM_RIGHTS
            operation (described in unix(7)).  This flag is useful for
            the same reasons as the O_CLOEXEC flag of open(2).
         */
        MsgCloseExec = MSG_CMSG_CLOEXEC,
        /**
         * @brief Enables nonblocking operation; if the operation would
              block, the call fails with the error EAGAIN or EWOULDBLOCK.
              This provides similar behavior to setting the O_NONBLOCK
              flag (via the fcntl(2) F_SETFL operation), but differs in
              that MSG_DONTWAIT is a per-call option, whereas O_NONBLOCK
              is a setting on the open file description (see open(2)),
              which will affect all threads in the calling process as
              well as other processes that hold file descriptors
              referring to the same open file description.
         */
        DontWait = MSG_DONTWAIT,
        /**
         * @brief This flag specifies that queued errors should be received
              from the socket error queue.
         */
        ErrorQueue = MSG_ERRQUEUE,
        /**
         * @brief This flag requests receipt of out-of-band data that would
              not be received in the normal data stream.  Some protocols
              place expedited data at the head of the normal data queue,
              and thus this flag cannot be used with such protocols.
         */
        OutOfBand = MSG_OOB,
        /**
         *  @brief This flag causes the receive operation to return data from
            the beginning of the receive queue without removing that
            data from the queue.  Thus, a subsequent receive call will
            return the same data.
         */
        Peek = MSG_PEEK,        
        /**
         *  @brief(since Linux 2.2)
            For raw (AF_PACKET), Internet datagram (since Linux
            2.4.27/2.6.8), netlink (since Linux 2.6.22), and UNIX
            datagram as well as sequenced-packet (since Linux 3.4)
            sockets: return the real size of the packet or datagram,
            even when it was longer than the passed buffer.
            For use with Internet stream sockets (TCP).*/
        Truncate = MSG_TRUNC, 
        WaitAll = MSG_WAITALL
    };

    /**
     * @brief If there is an error, -1 is returned.
     */
    size_t receive(std::error_code& err,
                const Socket& socket,
                auto&& buffer, uint64_t n,
                RECV_FLAGS flags) noexcept requires(
        (std::ranges::view<std::decay_t<decltype(buffer)>> && 
        std::ranges::random_access_range<std::decay_t<decltype(buffer)>> &&
        std::is_rvalue_reference_v<decltype(buffer)>) || 
        (std::ranges::random_access_range<std::decay_t<decltype(buffer)>> &&
        std::is_lvalue_reference_v<decltype(buffer)>))
    {
        if(n==0)
            return 0;
        assert(buffer.size()>=n);
        ssize_t read_num = 0;
        read_num = recv(socket.native(),buffer.data(),n,static_cast<int>(flags));
        if(read_num<0){
            err = std::make_error_code(static_cast<std::errc>(errno));
            errno= 0;
            switch(static_cast<std::errc>(err.value())){
                case std::errc::resource_unavailable_try_again:
                case std::errc::operation_in_progress:
                    err.clear();
                    read_num = 0;
                    return 0;
                default:
                    return 0;
            }
        }
        else {
            return read_num;
        }
        return 0;
    }

    size_t receive(std::error_code& err,const Socket& socket,auto&& buffer, uint64_t n) noexcept{
        return receive(err,socket,std::forward<decltype(buffer)>(buffer),n,static_cast<RECV_FLAGS>(0));
    }

    #ifdef __unix__
    size_t receive_to_ring_buffer(
          std::error_code& err,
          const Socket& socket,
          RingBuffer<char>& buffer) noexcept;
    #endif
}