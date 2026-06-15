#include "network/abstractserver.h"

using namespace network;

void AbstractServer::close(bool wait_for_end_connections,
        uint16_t timeout_sec) noexcept
{
    if(accepter_){
        std::error_code err;
        accepter_->stop(err);
        accepter_.reset();
    }
    this->processes_pool_.reset();
}
void AbstractServer::collapse(bool wait_for_end_connections,
        uint16_t timeout_sec) noexcept
{
    if(processes_pool_){
        std::error_code err;
        this->processes_pool_->stopConnections(wait_for_end_connections,timeout_sec,err);
    }
}