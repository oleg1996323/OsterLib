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
    void AbstractRequestableConnectionProcess::try_receive(
            std::error_code& err) noexcept
    {
        std::cout<<"(client) try_receive"<<std::endl;
        if(io_context().receive_buffer_size()==0)
            io_context().resize_receive_buffer(8096);
        if(!active_request_ && !make_active_request())
            return;
        io_context().receive(err,
                *active_request_->weak_to_receive_);
        if(auto err_val = static_cast<std::errc>(err.value());
            err_val!=std::errc::operation_in_progress &&
            err!=std::error_code() &&
            err_val!=std::errc::resource_unavailable_try_again)
        {
            complete_current_request(err);
            err.clear();
            return;
        }
        else {
            err.clear();
            complete_current_request(err);
        }
    }
    bool AbstractRequestableConnectionProcess::try_send(
            std::error_code& err) noexcept
    {
        err.clear();
        std::cout<<"try_send"<<std::endl;
        if(!active_request_ && !make_active_request()){
            io_context().enable_writable(false,err);
            return false;
        }
        io_context().send(err,*active_request_->weak_to_send_);
        if(auto err_val = static_cast<std::errc>(err.value());
            err_val!=std::errc::operation_in_progress &&
            err!=std::error_code() &&
            err_val!=std::errc::resource_unavailable_try_again)
        {
            complete_current_request(std::make_error_code(std::errc::bad_message));
            io_context().enable_writable(false,err);
            err.clear();
            return false;
        }
        else 
        {
            io_context().send(err,*active_request_->weak_to_send_);
            if(err!=std::error_code()){
                complete_current_request(err);
                err.clear();
                if(make_active_request())
                    try_send(err);
                return false;
            }
            else return true;
        }
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
    bool AbstractRequestableConnectionProcess::make_active_request() noexcept
    {
        std::cout<<"make_active_request"<<std::endl;
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
        std::cout<<"mark_error_request"<<std::endl;
        if(err!=std::error_code())
            std::cout<<err.message()<<std::endl;
        if(cmd)
            cmd->set_error(err);
    }
    void AbstractRequestableConnectionProcess::mark_notify_request(
            std::shared_ptr<Command<CommandType::RequestData>> cmd) noexcept
    {
        std::cout<<"mark_notify_request"<<std::endl;
        if(cmd)
            cmd->set_ready();
    }
}