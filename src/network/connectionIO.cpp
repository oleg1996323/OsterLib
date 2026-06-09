#include "connectionIO.h"
#include "abstractworker.h"

namespace network{
    void ConnectionIO::enable_writable(
            bool enable,
            std::error_code& err) noexcept{
        if((!enable && ((*sock_events_)&Event::Out)!=0) ||
            enable && ((*sock_events_)&Event::Out)==0)
            hconn_.owner()->enable_writing(hconn_,enable,err);
    }
    void ConnectionIO::enable_readable(
            bool enable,
            std::error_code& err) noexcept{
        if((!enable && ((*sock_events_)&Event::In)!=0) ||
            enable && ((*sock_events_)&Event::In)==0)
            hconn_.owner()->enable_readable(hconn_,enable,err);
    }
}