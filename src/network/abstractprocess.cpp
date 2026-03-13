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
        make_active_request();
    }
    void AbstractRequestableConnectionProcess::try_receive(
            std::error_code& err) noexcept
    {
        std::cout<<"try_receive"<<std::endl;
        if(io_context().receive_buffer_size()==0)
            io_context().resize_receive_buffer(8096);
        if(!active_request_ && !make_active_request())
            return;
        io_context().receive(err,
                active_request_->weak_to_receive_->min_initial_size());
        if(err!=std::error_code())
        {
            std::cout<<"receiving error: "<<err.message()<<std::endl;
            complete_current_request(std::make_error_code(std::errc::bad_message));
            return;
        }
        else {
            if(io_context().received()<
                active_request_->weak_to_receive_->min_initial_size())
                return;
            io_context().deserialize(*active_request_->weak_to_receive_);
            if(err!=std::error_code()){
                std::cout<<"deserialize error: "<<err.message()<<std::endl;
                return;
            }
            else{
                err.clear();
                complete_current_request(err);
            }
        }
    }
    void AbstractRequestableConnectionProcess::try_send(
            std::error_code& err) noexcept
    {
        std::cout<<"try_send"<<std::endl;
        if(!active_request_ && !make_active_request())
            return;
        if(auto ser_res = io_context().serialize(
                *active_request_->weak_to_send_);
            ser_res!=serialization::SerializationEC::NONE)
        {
            err=std::make_error_code(std::errc::bad_message);
            std::cout<<"serialization error: "<<err.message()<<std::endl;
            complete_current_request(std::make_error_code(std::errc::bad_message));
            return;
        }
        else {
            io_context().send(err);
            if(err!=std::error_code()){
                std::cout<<"trying sending error: "<<err.message()<<std::endl;
                complete_current_request(err);
            }
        }
    }
    void AbstractRequestableConnectionProcess::reset_requests(
            std::error_code& err) noexcept
    {
        std::cout<<"reset_requests"<<std::endl;
        while(!requests_.empty())
            requests_.pop();
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
            try_send(err);
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