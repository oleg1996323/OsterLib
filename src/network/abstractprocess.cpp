#include "abstractprocess.h"
#include "worker/command.h"

namespace network{
    AbstractRequestableConnectionProcess::AbstractRequestableConnectionProcess(
            ConnectionHandle hconn,
            std::error_code& err) noexcept:
        AbstractConnectionProcess(hconn,err){}
    
    bool AbstractRequestableConnectionProcess::requestable() const noexcept{
        return true;
    }
    void AbstractRequestableConnectionProcess::complete_current_request(
            std::error_code err) noexcept
    {
        if (active_request_)
            active_request_->set_error(err);
        active_request_.reset();
    }
    void AbstractRequestableConnectionProcess::reset_requests(
            std::error_code& err) noexcept
    {
        std::cout<<"reset_requests"<<std::endl;
        while(!requests_.empty())
            requests_.pop();
        complete_current_request(std::make_error_code(std::errc::interrupted));
        err.clear();
        active_request_.reset();
    }
    bool AbstractRequestableConnectionProcess::next_request() noexcept
    {
        std::cout<<"next_request"<<std::endl;
        if(active_request())
            return false;
        while(!requests_.empty()){
            active_request_ = requests_.front();
            requests_.pop();
            if(!active_request_)
                continue;
            else break;
        }
        return active_request_.get()!=nullptr;
    }
    void AbstractRequestableConnectionProcess::push_request(
            std::shared_ptr<Command<CommandType::RequestData>> request,
            std::error_code& err) noexcept{
        std::cout<<"(Process) push_request"<<std::endl;
        requests_.push(request);
        on_push_request(err);
    }
    bool AbstractRequestableConnectionProcess::active_request() const noexcept{
        return active_request_ && !active_request_->ready();
    }
    network::AbstractFrame& AbstractRequestableConnectionProcess::__internal_get_receiving_data__(
        std::shared_ptr<Command<CommandType::RequestData>> req) noexcept
    {
        return *req->received();
    }
    void AbstractRequestableConnectionProcess::on_push_request(std::error_code& err) noexcept{
        err.clear();
        on_write(err);
    }
    network::AbstractFrame& AbstractRequestableConnectionProcess::__internal_get_sending_data__(
        std::shared_ptr<Command<CommandType::RequestData>> req) noexcept
    {
        return *req->sent();
    }
}