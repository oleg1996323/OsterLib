#include "network/abstractserver.h"

using namespace network;

void AbstractServer::close(Timeout timeout_sec) noexcept
{
    if(accepter_){
        std::error_code err;
        accepter_->stop(err);
        accepter_.reset();
    }
    this->processes_pool_.reset();
}
void AbstractServer::collapse(Timeout timeout_sec) noexcept
{
    if(processes_pool_){
        std::error_code err;
        this->processes_pool_->stopConnections(timeout_sec,err);
    }
}