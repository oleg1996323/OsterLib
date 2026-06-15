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
        if (active_request_) {
            active_request_->set_error(err);
            active_request_->set_ready();
        }
        active_request_.reset();
    }
    void AbstractRequestableConnectionProcess::reset_requests(
            std::error_code& err) noexcept
    {
        //prstd::cout<<"reset_requests"<<std::endl;
        while(!requests_.empty())
            requests_.pop();
        complete_current_request(std::make_error_code(std::errc::interrupted));
        err.clear();
        active_request_.reset();
    }
    bool AbstractRequestableConnectionProcess::make_active_request() noexcept
    {
        //prstd::cout<<"make_active_request"<<std::endl;
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
        //prstd::cout<<"(Process) push_request"<<std::endl;
        if(requests_.empty()){
            active_request_=request;
            on_write(err);
        }
        else requests_.push(request);
    }

    void AbstractRequestableConnectionProcess::mark_error_request(
        std::shared_ptr<Command<CommandType::RequestData>> cmd,
        std::error_code err) noexcept
    {
        //prstd::cout<<"mark_error_request"<<std::endl;
        if(err!=std::error_code())
            //prstd::cout<<err.message()<<std::endl;
        if(cmd)
            cmd->set_error(err);
    }
    void AbstractRequestableConnectionProcess::mark_notify_request(
            std::shared_ptr<Command<CommandType::RequestData>> cmd) noexcept
    {
        //prstd::cout<<"mark_notify_request"<<std::endl;
        if(cmd)
            cmd->set_ready();
    }

    network::AbstractFrame& AbstractRequestableConnectionProcess::__internal_get_receiving_data__(
        std::shared_ptr<Command<CommandType::RequestData>> req) noexcept
    {
        return *req->received();
    }
    network::AbstractFrame& AbstractRequestableConnectionProcess::__internal_get_sending_data__(
        std::shared_ptr<Command<CommandType::RequestData>> req) noexcept
    {
        return *req->sent();
    }
}