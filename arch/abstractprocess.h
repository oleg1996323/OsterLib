#pragma once
#include <thread>
#include <future>
#include <stdexcept>
#include <chrono>
#include <functional>
#include <any>
#include <tuple>
#include <execution>
#include <expected>
#include "OsterLib/concepts.h"
#include <pthread.h>
#include <cassert>

namespace network{

enum class TaskMode{
    Sync,
    Thread
};

class AbstractTaskHandler{
    protected:
    TaskMode mode_;
    AbstractTaskHandler(TaskMode mode):
        mode_(mode){}
    AbstractTaskHandler(const AbstractTaskHandler& other) = delete;
    AbstractTaskHandler(AbstractTaskHandler& other) noexcept{
        operator=(std::move(other));
    }
    AbstractTaskHandler& operator=(const AbstractTaskHandler& other) = delete;
    AbstractTaskHandler& operator=(AbstractTaskHandler&& other) noexcept = default;
    public:
    virtual ~AbstractTaskHandler() = default;
    virtual bool is_ready(std::error_code& err) const noexcept = 0;
    virtual bool is_busy(std::error_code& err) const noexcept = 0;
    TaskMode mode() const noexcept{
        return mode_;
    }
    virtual bool request_stop(
                bool wait,
                uint16_t timeout_sec,
                std::error_code& err) noexcept = 0;
    virtual bool request_stop(std::error_code& err) noexcept{
        return request_stop(false,0,err);
    }
    virtual TaskMode get_task_mode() noexcept = 0;
    virtual bool is_stoppable() noexcept = 0;
    virtual bool is_blocking() noexcept = 0;
};

template<TaskMode MODE,typename Result>
class TypedTaskHandler:public AbstractTaskHandler{
    public:
    using result_type = Result;
    using result_return_t = std::conditional_t<!std::is_same_v<result_type,void>,
                std::optional<result_type>,void>;
    protected:
    mutable std::shared_future<result_type> result_;
    public:
    
    TypedTaskHandler(std::shared_future<Result>&& result):
        AbstractTaskHandler(MODE),
        result_(std::move(result)){}
    TypedTaskHandler(TypedTaskHandler&& other) noexcept:
        result_(std::move(other.result_)){}
    TypedTaskHandler& operator=(TypedTaskHandler&& other) noexcept{
        if(this!=&other){
            result_ = std::move(other.result_);
        }
        return *this;
    }
    TypedTaskHandler& operator=(const volatile TypedTaskHandler& other) = delete;
    virtual ~TypedTaskHandler() = default;
    bool is_ready(std::error_code& err) const noexcept final{
        if (!result_.valid()) {
            err = std::make_error_code(std::errc::no_such_process);
            return false;
        }
        return result_.wait_for(std::chrono::nanoseconds())==
                std::future_status::ready;
    }
    bool is_busy(std::error_code& err) const noexcept final{
        return !is_ready(err);
    }
    virtual TaskMode get_task_mode() noexcept override final{
        return MODE;
    }
    virtual bool is_stoppable() noexcept override final{
        if constexpr(MODE==TaskMode::Thread)
            return true;
        else return false;
    }
    virtual bool is_blocking() noexcept override final{
        if constexpr(MODE==TaskMode::Sync)
            return true;
        else return false;
    }
    virtual bool request_stop(
                bool wait,
                uint16_t timeout_sec,
                std::error_code& err) noexcept override final
    {
        err = std::make_error_code(std::errc::operation_not_supported);
        return false;
    }

    result_return_t get_result(std::error_code& err) noexcept{
        return get_result_timeout(-1,err);
    }
    result_return_t
                get_result_timeout(int32_t timeout_sec,
                        std::error_code& err) noexcept
    {
        if (!result_.valid()) {
            err = std::make_error_code(std::errc::no_such_process);
            if constexpr (!std::is_same_v<result_type, void>)
                return std::nullopt;
            else
                return;
        }
        std::future_status status;
        status = result_.wait_for(std::chrono::seconds(timeout_sec));
        if (status == std::future_status::ready){
            try {
                if constexpr (!std::is_same_v<result_type, void>) {
                    auto value = result_.get();
                    err.clear();
                    return value;
                } else {
                    result_.get();
                    err.clear();
                    return err;
                }
            } catch (...) {
                err = std::make_error_code(std::errc::io_error);
                if constexpr (!std::is_same_v<result_type, void>)
                    return std::nullopt;
                else return;
            }
        }
        else if(status == std::future_status::deferred){
            auto start = std::chrono::system_clock::now();
            auto result = result_.get();
            auto end = std::chrono::system_clock::now();
            if(end-start>std::chrono::seconds(timeout_sec))
                err = std::make_error_code(std::errc::timed_out);
            return result;
        }
        else{
            err = std::make_error_code(std::errc::timed_out);
            if constexpr (!std::is_same_v<result_type, void>)
                return std::nullopt;
            else return;
        }
    }
};

template<typename Result>
class TypedTaskHandler<TaskMode::Thread,Result>:public AbstractTaskHandler{
    public:
    using result_type = Result;
    using result_return_t = std::conditional_t<!std::is_same_v<result_type,void>,
                std::optional<result_type>,void>;
    protected:
    mutable std::shared_future<result_type> result_;
    std::jthread thread_;
    public:
    
    TypedTaskHandler():
        AbstractTaskHandler(TaskMode::Thread){}
    TypedTaskHandler(TypedTaskHandler&& other) noexcept:
        result_(std::move(other.result_)){}
    TypedTaskHandler operator=(TypedTaskHandler&& other) noexcept{
        if(this!=&other){
            result_ = std::move(other.result_);
        }
        return *this;
    }
    virtual ~TypedTaskHandler() = default;
    bool is_ready(std::error_code& err) const noexcept final{
        if (!result_.valid()) {
            err = std::make_error_code(std::errc::no_such_process);
            return false;
        }
        return result_.wait_for(std::chrono::nanoseconds())==
                std::future_status::ready;
    }
    bool is_busy(std::error_code& err) const noexcept final{
        return !is_ready(err);
    }
    virtual TaskMode get_task_mode() noexcept override final{
        return TaskMode::Thread;
    }
    virtual bool is_stoppable() noexcept override final{
        return true;
    }
    virtual bool is_blocking() noexcept override final{
        return false;
    }
    virtual bool request_stop(
                bool wait,
                uint16_t timeout_sec,
                std::error_code& err) noexcept override final
    {
        if(thread_.joinable()){
            if(wait && 
                result_.valid() &&
                result_.wait_for(
                    std::chrono::seconds(timeout_sec))==
                    std::future_status::ready)
            {
                err.clear();
                return true;
            }
            else{
                err = std::make_error_code(std::errc::timed_out);
                return thread_.request_stop();
            }
        }
        else return false;
    }

    virtual result_return_t get_result(std::error_code& err) noexcept{
        return get_result_timeout(-1,err);
    }
    virtual result_return_t
                get_result_timeout(int32_t timeout_sec,
                        std::error_code& err) noexcept
    {
        if (!result_.valid() || !thread_.joinable()) {
            err = std::make_error_code(std::errc::no_such_process);
            if constexpr (!std::is_same_v<result_type, void>)
                return std::nullopt;
            else
                return;
        }
        auto status = result_.wait_for(std::chrono::seconds(timeout_sec));
        if (status == std::future_status::ready){
            try {
                if constexpr (!std::is_same_v<result_type, void>) {
                    auto value = result_.get();
                    err.clear();
                    return value;
                } else {
                    result_.get();
                    err.clear();
                    return;
                }
            } catch (...) {
                err = std::make_error_code(std::errc::io_error);
                if constexpr (!std::is_same_v<result_type, void>)
                    return std::nullopt;
                else return;
            }
        }
        else{
            err = std::make_error_code(std::errc::timed_out);
            if constexpr (!std::is_same_v<result_type, void>)
                return std::nullopt;
            else return;
        }
    }
};

template<typename F,typename... ARGS>
class TaskHandler:
    public TypedTaskHandler<TaskMode::Sync,
                                std::invoke_result_t<F,ARGS...>>
{
    private:
    using Base = TypedTaskHandler<TaskMode::Sync,
                                std::invoke_result_t<F,ARGS...>>;
    public:
    using result_type = typename Base::result_type;
    using result_return_t = typename Base::result_return_t;
    TaskHandler(F function,
                    ARGS&&... args):
        Base(std::async(std::launch::deferred,
                function,
                std::forward<ARGS>(args)...).share()){}
    template<typename OBJ>
    TaskHandler(F function,
                    OBJ&& obj,
                    ARGS&&... args):
        Base(std::async(std::launch::deferred,
                &function,
                obj,
                std::forward<ARGS>(args)...).share()){}
    TaskHandler(const TaskHandler& other) = delete;
    TaskHandler(TaskHandler&& other) noexcept:
        Base(std::move(other)){}
    TaskHandler& operator=(const TaskHandler& other) = delete;
    TaskHandler& operator=(TaskHandler&& other) noexcept{
        Base::operator=(std::move(other));
        return *this;
    }
    virtual ~TaskHandler() = default;
};

template<typename F,typename... ARGS>
class ThreadedTaskHandler:
    public TypedTaskHandler<TaskMode::Thread,
            std::invoke_result_t<F,
                                ARGS...>>
{
    static_assert(std::is_invocable_v<F,ARGS...>,
            "Uninvokable function with presented arguments");
    static constexpr bool stop_token_from_thread = 
        std::is_invocable_v<F,std::stop_token,ARGS...>;
    private:
    using Base = TypedTaskHandler<TaskMode::Thread,
            std::invoke_result_t<F,
                                ARGS...>>;
    public:
    using result_type = typename Base::result_type;
    using result_return_t = typename Base::result_return_t;
    ThreadedTaskHandler(    F&& funct,
                    ARGS&&... args)
    {
            std::promise<result_type> promise;
            Base::result_ = promise.get_future().share();
            Base::thread_ = std::move(std::jthread([
                        prom = std::move(promise),
                        function = funct](
                        std::stop_token stop,
                        ARGS&&... args) mutable
            {
                if constexpr (stop_token_from_thread){
                    if constexpr (std::is_same_v<result_type,void>){
                        std::invoke(function,std::forward<ARGS>(args)...);
                        prom.set_value_at_thread_exit();
                    }
                    else prom.set_value_at_thread_exit(
                        std::invoke(function,std::forward<ARGS>(args)...));
                }
                else {
                    if constexpr (std::is_same_v<result_type,void>){
                        std::invoke(function,std::forward<ARGS>(args)...);
                            prom.set_value_at_thread_exit();
                    }
                    else prom.set_value_at_thread_exit(
                            std::invoke(function,std::forward<ARGS>(args)...));
                }
            },
            std::forward<ARGS>(args)...));
    }
    ThreadedTaskHandler(const ThreadedTaskHandler& other) = delete;
    ThreadedTaskHandler(ThreadedTaskHandler&& other) noexcept:
        Base(std::move(other)){}
    ThreadedTaskHandler& operator=(const ThreadedTaskHandler& other) = delete;
    ThreadedTaskHandler& operator=(ThreadedTaskHandler&& other) noexcept{
        Base::operator=(std::move(other));
        return *this;
    }
};

template<typename F,typename... ARGS>
class StoppableThreadedTaskHandler:
    public TypedTaskHandler<TaskMode::Thread,
            std::invoke_result_t<F,
                                std::stop_token,
                                ARGS...>>
{
    static_assert(std::is_invocable_v<F,std::stop_token,ARGS...>,
            "Uninvokable function with presented arguments");
    static constexpr bool stop_token_from_thread = 
        std::is_invocable_v<F,std::stop_token,ARGS...>;
    private:
    using Base = TypedTaskHandler<TaskMode::Thread,
                            std::invoke_result_t<F,
                                std::stop_token,
                                ARGS...>>;
    public:
    using result_type = typename Base::result_type;
    using result_return_t = typename Base::result_return_t;
    StoppableThreadedTaskHandler(    F&& funct,
                    ARGS&&... args)
    {
            std::promise<result_type> promise;
            Base::result_ = promise.get_future().share();
            Base::thread_ = std::move(std::jthread([
                        prom = std::move(promise),
                        function = funct](
                        std::stop_token stop,
                        ARGS&&... args) mutable
            {
                if constexpr (stop_token_from_thread){
                    if constexpr (std::is_same_v<result_type,void>){
                        std::invoke(function,std::forward<ARGS>(args)...);
                        prom.set_value_at_thread_exit();
                    }
                    else prom.set_value_at_thread_exit(
                        std::invoke(function,std::forward<ARGS>(args)...));
                }
                else {
                    if constexpr (std::is_same_v<result_type,void>){
                        std::invoke(function,std::forward<ARGS>(args)...);
                            prom.set_value_at_thread_exit();
                    }
                    else prom.set_value_at_thread_exit(
                            std::invoke(function,std::forward<ARGS>(args)...));
                }
            },
            std::forward<ARGS>(args)...));
    }
    StoppableThreadedTaskHandler(const StoppableThreadedTaskHandler& other) = delete;
    StoppableThreadedTaskHandler(StoppableThreadedTaskHandler&& other) noexcept:
        Base(std::move(other)){}
    StoppableThreadedTaskHandler& operator=(const StoppableThreadedTaskHandler& other) = delete;
    StoppableThreadedTaskHandler& operator=(StoppableThreadedTaskHandler&& other) noexcept{
        Base::operator=(std::move(other));
        return *this;
    }
};

template<bool stop_token_arg,
        typename F,
        typename OBJ,
        typename... ARGS>
struct BindedThreadedTaskInvokeTypeImpl;

template<typename F,
        typename OBJ,
        typename... ARGS>
struct BindedThreadedTaskInvokeTypeImpl<
        true,
        F,
        OBJ,
        ARGS...>{
    using type = std::invoke_result_t<F,
                                OBJ,
                                std::stop_token,
                                ARGS...>;
};

template<typename F,
        typename OBJ,
        typename... ARGS>
struct BindedThreadedTaskInvokeTypeImpl<
        false,
        F,
        OBJ,
        ARGS...>{
    using type = std::invoke_result_t<
                                F,
                                OBJ,
                                ARGS...>;
};

template<
        typename F,
        typename OBJ,
        typename... ARGS>
using BindedThreadedTaskInvokeType = 
    typename BindedThreadedTaskInvokeTypeImpl<
        std::is_invocable_v<F,OBJ,
        std::stop_token, ARGS...>, F,OBJ, ARGS...>::type;

template<typename F,typename OBJ,typename... ARGS>
class BindedThreadedTaskHandler:
    public TypedTaskHandler<TaskMode::Thread,
            BindedThreadedTaskInvokeType<F,OBJ,ARGS...>>
{
    static_assert(std::is_invocable_v<F,OBJ,std::stop_token,ARGS...>,
            "Uninvokable function with presented arguments");
    static constexpr bool stop_token_from_thread = 
        std::is_invocable_v<F,OBJ,std::stop_token,ARGS...>;
    private:
    using Base = TypedTaskHandler<TaskMode::Thread,
            BindedThreadedTaskInvokeType<F,OBJ,ARGS...>>;
    public:
    using result_type = typename Base::result_type;
    using result_return_t = typename Base::result_return_t;
    BindedThreadedTaskHandler(F&& funct,
                    OBJ&& obj,
                    ARGS&&... args)
    {
        std::promise<result_type> promise;
        Base::result_ = promise.get_future().share();
        Base::thread_ = std::move(std::jthread([
                    prom = std::move(promise),
                    function = funct](
                    std::stop_token stop,
                    OBJ&& obj,
                    ARGS&&... args) mutable
        {
            if constexpr (stop_token_from_thread) {
                if constexpr (std::is_same_v<result_type, void>) {
                    std::invoke(function, obj, stop, std::forward<ARGS>(args)...);
                    prom.set_value_at_thread_exit();
                } else {
                    prom.set_value_at_thread_exit(
                        std::invoke(function, obj, stop, std::forward<ARGS>(args)...));
                }
            } else {
                if constexpr (std::is_same_v<result_type, void>) {
                    std::invoke(function, obj, std::forward<ARGS>(args)...);
                    prom.set_value_at_thread_exit();
                } else {
                    prom.set_value_at_thread_exit(
                        std::invoke(function, obj, std::forward<ARGS>(args)...));
                }
            }
        },
        std::forward<OBJ>(obj),
        std::forward<ARGS>(args)...));
    }
    BindedThreadedTaskHandler(const BindedThreadedTaskHandler& other) = delete;
    BindedThreadedTaskHandler(BindedThreadedTaskHandler&& other) noexcept:
        Base(std::move(other)){}
    BindedThreadedTaskHandler& operator=(const BindedThreadedTaskHandler& other) = delete;
    BindedThreadedTaskHandler& operator=(BindedThreadedTaskHandler&& other) noexcept{
        Base::operator=(std::move(other));
        return *this;
    }
};

class Process{
    protected:
    std::unique_ptr<AbstractTaskHandler> task_;
    public:
    bool is_ready(std::error_code& err) const{
        return has_task()?task_->is_ready(err):true;
    }
    bool is_busy(std::error_code& err) const{
        return has_task()?task_->is_busy(err):false;
    }
    bool has_task() const{
        return task_.get()!=nullptr?true:false;
    }
    Process() = default;
    Process(const Process&) = delete;
    Process(Process&& other) noexcept = delete;
    Process& operator=(const Process&) = delete;
    Process& operator=(Process&& other) noexcept = delete;
    virtual ~Process(){
        task_.reset();
    }
    template<typename F,typename... ARGS>
    void emplace_task(  std::error_code& err,
                        TaskMode mode,
                        F function,
                        ARGS&&...args){
        static_assert(std::is_invocable_v<F,
                                        ARGS...> ||
                    std::is_invocable_v<F,std::stop_token,ARGS...>);
        switch (mode)
        {
        case TaskMode::Sync:
            task_ = std::move(std::make_unique<TaskHandler<F,ARGS...>>(
                    function,
                    std::forward<ARGS>(args)...));
            break;
        case TaskMode::Thread:
            task_ = std::move(std::make_unique<ThreadedTaskHandler<F,ARGS...>>(
                    function,
                    std::forward<ARGS>(args)...));
            break;
        default:
            assert(("Unexpected task mode",false));
            break;
        }
    }
    template<typename F,typename OBJ,typename... ARGS>
    void emplace_binded_task(  std::error_code& err,
                        TaskMode mode,
                        F function,
                        OBJ&& obj,
                        ARGS&&...args){
        
        switch (mode)
        {
        case TaskMode::Sync:
            task_ = std::move(std::make_unique<TaskHandler<F,ARGS...>>(
                    function,
                    std::forward<OBJ>(obj),
                    std::forward<ARGS>(args)...));
            break;
        case TaskMode::Thread:
            task_ = std::move(std::make_unique<BindedThreadedTaskHandler<F,OBJ,ARGS...>>(
                    function,
                    std::forward<OBJ>(obj),
                    std::forward<ARGS>(args)...));
            break;
        default:
            assert(("Unexpected task mode",false));
            break;
        }
    }
    template<typename RESULT>
    bool holds() const{
        if( dynamic_cast<TypedTaskHandler<TaskMode::Sync,RESULT>*>(
                task_.get())!=nullptr ||
            dynamic_cast<TypedTaskHandler<TaskMode::Thread,RESULT>*>(
                task_.get())!=nullptr)
            return true;
        else return false;
    }
    bool request_stop(
                bool wait,
                uint16_t timeout_sec,
                std::error_code& err) noexcept{
        if(auto ptr = task_.get();
            ptr!=nullptr)
            return ptr->request_stop(wait,timeout_sec,err);
        else{
            err = std::make_error_code(
                std::errc::no_such_process);
            return false;
        }
    }
    virtual bool requestable() const noexcept{
        return false;
    }
};
}

#include "multiplexor/events.h"
#include "connectionhandle.h"
#include "connectionIO.h"
namespace network{
class Connection;

class AbstractConnectionProcess:public Process{
    mutable ConnectionIO* io_;
    ConnectionHandle hconn_;
    protected:
    ConnectionIO& io_context() const noexcept{
        assert(io_);
        return *io_;
    }
    ConnectionHandle connection_handle() const noexcept{
        return hconn_;
    }
    public:
    virtual void on_read(std::error_code& err) noexcept = 0;
    virtual void on_write(std::error_code& err) noexcept = 0;
    virtual void on_task_done(std::error_code& err) noexcept = 0;
    virtual void on_stop_requested(std::error_code& err) noexcept = 0;
    virtual void at_fatal_error(std::error_code& err) noexcept{
        err.clear();
        io_context().clear_buffers();
    }
    bool handle_sending_error(std::error_code& err) noexcept{
        if(err){
            switch(static_cast<std::errc>(err.value())){
                case std::errc::operation_in_progress:
                case std::errc::resource_unavailable_try_again:
                    err.clear();
                    return true;
                    break;
                case std::errc::no_buffer_space:
                    io_context().enable_writable(false,err);
                    err.clear();
                    return true;
                    break;
                default:
                    at_fatal_error(err);
                    return false;
            }
        }
        else return true;
    }

    bool handle_receive_error(std::error_code& err) noexcept{
        if(err){
            switch(static_cast<std::errc>(err.value())){
                case std::errc::operation_in_progress:
                case std::errc::resource_unavailable_try_again:
                    err.clear();
                    return true;
                    break;
                case std::errc::no_buffer_space:
                    io_context().enable_writable(false,err);
                    err.clear();
                    return true;
                    break;
                default:
                    at_fatal_error(err);
                    return false;
            }
        }
        else return true;
    }
    
    AbstractConnectionProcess(
            ConnectionHandle hconn,
            std::error_code& err) noexcept:
        hconn_(hconn){}
    void set_connectionIO(ConnectionIO* connIO) noexcept{
        io_=connIO;
    }
    virtual ~AbstractConnectionProcess() = default;
    virtual void handle_event(
                Event event,
                std::error_code& err) noexcept = 0;
    virtual bool requestable() const noexcept override{
        return false;
    }
};
}
#include <queue>
#include <memory>
#include "worker/command_types.h"
#include "frame/dataframe.h"
#include "frame/frames.h"

class AbstractFrame;


namespace network{
template<CommandType,typename...>
class Command;
class AbstractRequestableConnectionProcess:public AbstractConnectionProcess{
    network::AbstractFrame& __internal_get_receiving_data__(
        std::shared_ptr<Command<CommandType::RequestData>> req) noexcept;
    network::AbstractFrame& __internal_get_sending_data__(
        std::shared_ptr<Command<CommandType::RequestData>> req) noexcept;
    
    void mark_error_request(
            std::shared_ptr<Command<CommandType::RequestData>> cmd,
            std::error_code err) noexcept;
    void mark_notify_request(
            std::shared_ptr<Command<CommandType::RequestData>> cmd) noexcept;
    protected:
    void complete_current_request(
            std::error_code err) noexcept;
    public:
    std::queue<std::shared_ptr<Command<CommandType::RequestData>>> requests_;
    std::shared_ptr<Command<CommandType::RequestData>> active_request_{};
    virtual ~AbstractRequestableConnectionProcess() = default;    
    AbstractRequestableConnectionProcess(
            ConnectionHandle hconn,
            std::error_code& err) noexcept;
    template<typename DATA,typename START,typename END>
    bool received_data(DataFrame<START,DATA,END>& data) noexcept{
        if(active_request_){
            data = static_cast<Frame<START,DATA,END>&>(
                    __internal_get_receiving_data__(active_request_));
            return true;
        }
        else return false;
    }
    template<typename DATA,typename START,typename END>
    bool sending_data(DataFrame<START,DATA,END>& data) noexcept{
        if(active_request_){
            data = static_cast<Frame<START,DATA,END>&>(
                    __internal_get_sending_data__(active_request_));
            return true;
        }
        else return false;
    }
    virtual void at_fatal_error(std::error_code& err) noexcept{
        complete_current_request(std::make_error_code(std::errc::bad_message));
        err.clear();
        io_context().clear_buffers();
    }
    bool active_request(std::error_code& err){
        if(!active_request_ && !next_request()){
            //prstd::cout<<"Requests not found"<<std::endl;
            io_context().enable_writable(false,err);
            return false;
        }
        else return true;
    }
    bool handle_receive_error(
            std::error_code& err) noexcept
    {
        if(err)
        switch(static_cast<std::errc>(err.value())){
            case std::errc::operation_in_progress:
            case std::errc::resource_unavailable_try_again:
            case std::errc::no_buffer_space:
                err.clear();
                return true;
            default:
                at_fatal_error(err);
                return false;
        }
        return true;
    }
    bool handle_sending_error(
            std::error_code& err) noexcept
    {
        if(err){
            switch(static_cast<std::errc>(err.value())){
                case std::errc::operation_in_progress:
                case std::errc::resource_unavailable_try_again:
                    err.clear();
                    return true;
                    break;
                case std::errc::no_buffer_space:
                    io_context().enable_writable(false,err);
                    err.clear();
                    return true;
                    break;
                default:
                    at_fatal_error(err);
                    return false;
            }
        }
        else return true;
    }
    virtual bool requestable() const noexcept override final;
    void reset_requests(std::error_code& err) noexcept;
    virtual void on_read(std::error_code& err) noexcept override = 0;
    virtual void on_write(std::error_code& err) noexcept override = 0;
    virtual void on_task_done(std::error_code& err) noexcept override = 0;
    virtual void on_stop_requested(std::error_code& err) noexcept override = 0;
    bool next_request() noexcept;
    void push_request(std::shared_ptr<Command<CommandType::RequestData>> request,
            std::error_code& err) noexcept;
};
}