#include "OsterLib/network/connectionIO.h"
#include "OsterLib/network/abstractworker.h"

namespace network{
    void ConnectionIO::enable_writable(
            bool enable,
            std::error_code& err) noexcept{
        hconn_.owner()->enable_writing(hconn_,enable,err);
    }
    void ConnectionIO::enable_readable(
            bool enable,
            std::error_code& err) noexcept{
        hconn_.owner()->enable_readable(hconn_,enable,err);
    }
}