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
#include "concepts.h"
#include <pthread.h>
#include <cassert>
#include "definitions.h"
#include <memory>

namespace network{

enum class TaskMode{
    Sync,
    Thread
};

class AbstractTaskHandler{
    protected:
    std::function<bool()> assigner_;
    std::function<void()> callback_=[](){};
    TaskMode mode_;
    AbstractTaskHandler(TaskMode mode,std::function<void()> callback=[](){}):
        callback_(std::move(callback)),mode_(mode){}
    AbstractTaskHandler(const AbstractTaskHandler& other) = delete;
    AbstractTaskHandler(AbstractTaskHandler&& other) noexcept{
       mode_=other.mode_;
       callback_=std::move(other.callback_);
    }
    AbstractTaskHandler& operator=(const AbstractTaskHandler& other) = delete;
    AbstractTaskHandler& operator=(AbstractTaskHandler&& other) noexcept {
        if(this!=&other){
            mode_=other.mode_;
            callback_=std::move(other.callback_);
        }
        return *this;
    }
    public:
    virtual ~AbstractTaskHandler() = default;
    virtual bool is_ready(std::error_code& err) const noexcept = 0;
    virtual bool is_busy(std::error_code& err) const noexcept = 0;
    template<typename TYPE>
    std::optional<TYPE> get_as(std::error_code& err) noexcept;
    TaskMode mode() const noexcept{
        return mode_;
    }
    const std::function<void()>& callback() const noexcept{
        return callback_;
    }
    template<typename FUNCTION>
    void callback(FUNCTION&& func) noexcept{
        callback_ = std::forward<FUNCTION>(func);
    }
    virtual bool request_stop(
                Timeout timeout_sec,
                std::error_code& err) noexcept = 0;
    virtual bool request_stop(std::error_code& err) noexcept{
        return request_stop(0,err);
    }
    virtual TaskMode get_task_mode() noexcept = 0;
    virtual bool is_stoppable() noexcept = 0;
};

template<TaskMode MODE,typename Result>
class TypedTaskHandler:std::false_type{};

template<typename Result>
class TypedTaskHandler<TaskMode::Sync,Result>:public AbstractTaskHandler{
    public:
    using result_type = Result;
    using result_return_t = std::conditional_t<!std::is_same_v<result_type,void>,
                std::optional<result_type>,void>;
    protected:
    mutable std::shared_future<result_type> result_;
    public:
    
    TypedTaskHandler(
            std::shared_future<Result>&& result,
            std::function<void()> callback):
        AbstractTaskHandler(TaskMode::Sync,std::move(callback)),
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
    bool is_ready(std::error_code& err) const noexcept override final{
        if (!result_.valid()) {
            err = std::make_error_code(std::errc::no_such_process);
            return false;
        }
        return result_.wait_for(std::chrono::nanoseconds())==
                std::future_status::ready;
    }
    bool is_busy(std::error_code& err) const noexcept override final{
        return !is_ready(err);
    }
    virtual TaskMode get_task_mode() noexcept override final{
        return TaskMode::Sync;
    }
    virtual bool is_stoppable() noexcept override final{
        return false;
    }
    virtual bool request_stop(
                Timeout timeout_sec,
                std::error_code& err) noexcept override final
    {
        err = std::make_error_code(std::errc::operation_not_supported);
        return false;
    }

    result_return_t get_result(std::error_code& err) noexcept{
        return get_result_timeout(-1,err);
    }
    result_return_t
            get_result_timeout(Timeout timeout_sec,
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
    
    TypedTaskHandler(std::function<void()> callback = [](){}):
        AbstractTaskHandler(TaskMode::Thread,std::move(callback)){}
    TypedTaskHandler(TypedTaskHandler&& other) noexcept:
        result_(std::move(other.result_)),
        thread_(std::move(other.thread_)){}
    TypedTaskHandler operator=(TypedTaskHandler&& other) noexcept{
        if(this!=&other){
            result_ = std::move(other.result_);
            thread_ = std::move(other.thread_);
        }
        return *this;
    }
    virtual ~TypedTaskHandler() = default;
    bool is_ready(std::error_code& err) const noexcept final{
        err.clear();
        if (!result_.valid()) {
            err = std::make_error_code(std::errc::no_such_process);
            return false;
        }
        auto ready = result_.wait_for(std::chrono::nanoseconds())==
                std::future_status::ready;
        err.clear();
        return ready;
    }
    bool is_busy(std::error_code& err) const noexcept final{
        return !is_ready(err);
    }
    virtual TaskMode get_task_mode() noexcept override final{
        return TaskMode::Thread;
    }
    virtual bool is_stoppable() noexcept override final{
        return thread_.joinable();
    }
    virtual bool request_stop(
                Timeout timeout_sec,
                std::error_code& err) noexcept override final
    {
        if(thread_.joinable()){
            if(timeout_sec>0 && 
                result_.valid())
            {
                if(result_.wait_for(
                    std::chrono::seconds(timeout_sec))==
                    std::future_status::ready){
                    err.clear();
                    return true;
                }
                else{
                    thread_.request_stop();
                    err = std::make_error_code(std::errc::timed_out);
                    return false;
                }
            }
            else{
                err.clear();
                return thread_.request_stop();
            }
        }
        else {
            err = std::make_error_code(std::errc::no_such_process);
            return false;
        }
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

template<typename F, typename... ARGS>
class TaskHandler :
    public std::conditional_t<
        std::is_invocable_v<F, std::stop_token, ARGS...>,
        TypedTaskHandler<TaskMode::Sync,
                         decltype(std::declval<F>()(std::declval<std::stop_token>(), std::declval<ARGS>()...))>,
        TypedTaskHandler<TaskMode::Sync,
                         decltype(std::declval<F>()(std::declval<ARGS>()...))>
    >
{
private:
    static_assert(
        std::is_invocable_v<F, ARGS...> ||
        std::is_invocable_v<F, std::stop_token, ARGS...>
    );

    using Base = std::conditional_t<
        std::is_invocable_v<F, ARGS...>,
        TypedTaskHandler<TaskMode::Sync,
                         decltype(std::declval<F>()(std::declval<ARGS>()...))>,
        TypedTaskHandler<TaskMode::Sync,
                         decltype(std::declval<F>()(std::declval<std::stop_token>(), std::declval<ARGS>()...))>
    >;

public:
    using result_type = typename Base::result_type;
    using result_return_t = typename Base::result_return_t;

    TaskHandler(
            std::function<void()> callback,
            F&& function,
            ARGS&&... args):
        Base(std::async(std::launch::deferred,
                        std::forward<F>(function),
                        std::forward<ARGS>(args)...).share(),
                std::move(callback))
    {}
    TaskHandler(F&& function, ARGS&&... args):
        Base(std::async(std::launch::deferred,
                        std::forward<F>(function),
                        std::forward<ARGS>(args)...).share())
    {}
    template<typename OBJ>
    TaskHandler(std::function<void()> callback,
            F&& function,
            OBJ&& obj,
            ARGS&&... args):
        Base(std::async(std::launch::deferred,
                        std::forward<F>(function),
                        std::forward<OBJ>(obj),
                        std::forward<ARGS>(args)...).share(),
                std::move(callback))
    {}
    template<typename OBJ>
    TaskHandler(F&& function, OBJ&& obj, ARGS&&... args) :
        Base(std::async(std::launch::deferred,
                        std::forward<F>(function),
                        std::forward<OBJ>(obj),
                        std::forward<ARGS>(args)...).share())
    {}

    TaskHandler(const TaskHandler&) = delete;
    TaskHandler(TaskHandler&&) noexcept = default;
    TaskHandler& operator=(const TaskHandler&) = delete;
    TaskHandler& operator=(TaskHandler&&) noexcept = default;
    virtual ~TaskHandler() = default;
};

template<bool stop_token_arg, typename F, typename... ARGS>
struct ThreadedTaskInvokeTypeImpl;

template<typename F, typename... ARGS>
struct ThreadedTaskInvokeTypeImpl<true, F, ARGS...> {
    using type = decltype(std::declval<F>()(std::declval<std::stop_token>(), std::declval<ARGS>()...));
};

template<typename F, typename... ARGS>
struct ThreadedTaskInvokeTypeImpl<false, F, ARGS...> {
    using type = decltype(std::declval<F>()(std::declval<ARGS>()...));
};

template<typename F, typename... ARGS>
using ThreadedTaskInvokeType = 
    typename ThreadedTaskInvokeTypeImpl<
        std::is_invocable_v<F, std::stop_token, ARGS...>,
        F, ARGS...>::type;

template<typename F,typename... ARGS>
class ThreadedTaskHandler:
    public TypedTaskHandler<TaskMode::Thread,
            ThreadedTaskInvokeType<F,ARGS...>>
{
    static_assert(std::is_invocable_v<F, std::stop_token, ARGS...> ||
                std::is_invocable_v<F, ARGS...>,
                "Uninvokable function with presented arguments");
    static constexpr bool stop_token_from_thread =
        std::is_invocable_v<F, std::stop_token, ARGS...>;
    private:
    using Base = TypedTaskHandler<TaskMode::Thread,
            ThreadedTaskInvokeType<F,ARGS...>>;
    public:
    using result_type = typename Base::result_type;
    using result_return_t = typename Base::result_return_t;
    void set_thread(F&& funct, ARGS&&... args){
        std::promise<result_type> promise;
        Base::result_ = promise.get_future().share();

        Base::thread_ = std::jthread(
        [this,function = F(std::forward<F>(funct)),
        prom = std::move(promise),
        ... captured_args = ARGS(std::forward<ARGS>(args))]
        (std::stop_token stop) mutable
        {
            if constexpr (stop_token_from_thread)
            {
                if constexpr (std::is_same_v<result_type, void>)
                {
                    std::invoke(function, stop, captured_args...);
                    prom.set_value();
                    AbstractTaskHandler::callback()();
                }
                else
                {
                    auto result = std::invoke(function, stop, captured_args...);
                    prom.set_value(std::move(result));
                    AbstractTaskHandler::callback()();
                }
            }
            else
            {
                if constexpr (std::is_same_v<result_type, void>)
                {
                    std::invoke(function, captured_args...);
                    prom.set_value();
                    AbstractTaskHandler::callback()();
                }
                else
                {
                    auto result = std::invoke(function, captured_args...);
                    prom.set_value(std::move(result));
                    AbstractTaskHandler::callback()();
                }
            }
        });
    }
    ThreadedTaskHandler(std::function<void()> callback,F&& funct, ARGS&&... args):
    Base(std::move(callback))
    {
        set_thread(
            std::forward<F>(funct),
            std::forward<ARGS>(args)...);
    }
    ThreadedTaskHandler(F&& funct, ARGS&&... args)
    {
        set_thread(
            std::forward<F>(funct),
            std::forward<ARGS>(args)...);
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

template<bool stop_token_arg, typename F, typename OBJ, typename... ARGS>
struct BindedThreadedTaskInvokeTypeImpl;

template<typename F, typename OBJ, typename... ARGS>
struct BindedThreadedTaskInvokeTypeImpl<true, F, OBJ, ARGS...> {
    using type = std::invoke_result_t<F,OBJ,std::stop_token,ARGS...>;
};

template<typename F, typename OBJ, typename... ARGS>
struct BindedThreadedTaskInvokeTypeImpl<false, F, OBJ, ARGS...> {
    using type = std::invoke_result_t<F,OBJ,ARGS...>;
};

template<typename F, typename OBJ, typename... ARGS>
using BindedThreadedTaskInvokeType = 
    typename BindedThreadedTaskInvokeTypeImpl<
        std::is_invocable_v<F, OBJ, std::stop_token, ARGS...>,
        F, OBJ, ARGS...>::type;

template<typename F, typename OBJ, typename... ARGS>
class BindedThreadedTaskHandler :
    public TypedTaskHandler<TaskMode::Thread, BindedThreadedTaskInvokeType<F, OBJ, ARGS...>>
{
    static_assert(std::is_invocable_v<F, OBJ, std::stop_token, ARGS...> ||
                std::is_invocable_v<F, OBJ, ARGS...>,
              "Uninvokable function with presented arguments");
    static constexpr bool stop_token_from_thread =
    std::is_invocable_v<F, OBJ, std::stop_token, ARGS...>;

private:
    using Base = TypedTaskHandler<TaskMode::Thread, BindedThreadedTaskInvokeType<F, OBJ, ARGS...>>;

public:
    using result_type = typename Base::result_type;
    using result_return_t = typename Base::result_return_t;
    
    void set_thread(F&& funct, OBJ&& obj, ARGS&&... args){
        std::promise<result_type> promise;
        Base::result_ = promise.get_future().share();

        Base::thread_ = std::jthread(
        [this,function = F(std::forward<F>(funct)),
            obj_internal = OBJ(std::forward<OBJ>(obj)),
            prom = std::move(promise),
            ... captured_args = ARGS(std::forward<ARGS>(args))]
        (std::stop_token stop) mutable
        {
            if constexpr (stop_token_from_thread)
            {
                if constexpr (std::is_same_v<result_type, void>)
                {
                    std::invoke(function, obj_internal, stop, captured_args...);
                    prom.set_value();
                    AbstractTaskHandler::callback()();
                }
                else
                {
                    auto result = std::invoke(function, obj_internal, stop, captured_args...);
                    prom.set_value(std::move(result));
                    AbstractTaskHandler::callback()();
                }
            }
            else
            {
                if constexpr (std::is_same_v<result_type, void>)
                {
                    std::invoke(function, obj_internal, captured_args...);
                    prom.set_value();
                    AbstractTaskHandler::callback()();
                }
                else
                {
                    auto result = std::invoke(function, obj_internal, captured_args...);
                    prom.set_value(std::move(result));
                    AbstractTaskHandler::callback()();
                }
            }
        });
    }
    BindedThreadedTaskHandler(
        std::function<void()> callback,
        F&& funct, OBJ&& obj,
        ARGS&&... args):
    Base(std::move(callback))
    {
        set_thread(
            std::forward<F>(funct),
            std::forward<OBJ>(obj),
            std::forward<ARGS>(args)...);
    }

    BindedThreadedTaskHandler(F&& funct, OBJ&& obj, ARGS&&... args)
    {
        set_thread(
            std::forward<F>(funct),
            std::forward<OBJ>(obj),
            std::forward<ARGS>(args)...);
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

template<typename TYPE>
std::optional<TYPE> AbstractTaskHandler::get_as(std::error_code& err) noexcept{
    switch(mode_){
        case TaskMode::Sync:{
            if(TypedTaskHandler<TaskMode::Sync,TYPE>* casted = 
                dynamic_cast<TypedTaskHandler<TaskMode::Sync,TYPE>*>(this);
                casted!=nullptr)
                return casted->get_result(err);
            else return std::nullopt;
        }
        break;
        case TaskMode::Thread:{
            if(TypedTaskHandler<TaskMode::Thread,TYPE>* casted = 
                dynamic_cast<TypedTaskHandler<TaskMode::Thread,TYPE>*>(this);
                casted!=nullptr)
                return casted->get_result(err);
            else return std::nullopt;
        }
        break;
        default:
        return std::nullopt;
    }
}

class Process{
    protected:
    std::unique_ptr<AbstractTaskHandler> task_;
    public:
    bool is_ready(std::error_code& err) const noexcept{
        return has_task()?task_->is_ready(err):true;
    }
    bool is_busy(std::error_code& err) const noexcept{
        return has_task()?task_->is_busy(err):false;
    }
    bool has_task() const{
        std::error_code err;
        return task_.get()!=nullptr;
    }
    Process() = default;
    Process(const Process&) = delete;
    Process(Process&& other) noexcept = delete;
    Process& operator=(const Process&) = delete;
    Process& operator=(Process&& other) noexcept = delete;
    virtual ~Process(){
        task_.reset();
    }
    template<TaskMode mode, typename F, typename... ARGS>
    void emplace_task(std::function<void()> callback,std::error_code& err, F&& function, ARGS&&... args) {
        if constexpr (mode == TaskMode::Sync) {
            task_ = std::make_unique<TaskHandler<F, ARGS...>>(
                std::move(callback),
                std::forward<F>(function),
                std::forward<ARGS>(args)...);
        } else if constexpr (mode == TaskMode::Thread) {
            task_ = std::make_unique<ThreadedTaskHandler<F, ARGS...>>(
                std::move(callback),
                std::forward<F>(function),
                std::forward<ARGS>(args)...);
        } else {
            static_assert(mode == TaskMode::Sync || mode == TaskMode::Thread,
                        "Invalid TaskMode");
        }
    }
    template<TaskMode mode, typename F, typename... ARGS>
    void emplace_task(std::error_code& err, F&& function, ARGS&&... args) {
        if constexpr (mode == TaskMode::Sync) {
            task_ = std::make_unique<TaskHandler<F, ARGS...>>(
                std::forward<F>(function),
                std::forward<ARGS>(args)...);
        } else if constexpr (mode == TaskMode::Thread) {
            task_ = std::make_unique<ThreadedTaskHandler<F, ARGS...>>(
                std::forward<F>(function),
                std::forward<ARGS>(args)...);
        } else {
            static_assert(mode == TaskMode::Sync || mode == TaskMode::Thread,
                        "Invalid TaskMode");
        }
    }
    template<TaskMode mode, typename F, typename OBJ, typename... ARGS>
    void emplace_binded_task(std::function<void()> callback,std::error_code& err, F&& function, OBJ&& obj, ARGS&&... args) {
        if constexpr (mode == TaskMode::Sync) {
            task_ = std::make_unique<TaskHandler<F, ARGS...>>(
                std::move(callback),
                std::forward<F>(function),
                std::forward<OBJ>(obj),
                std::forward<ARGS>(args)...);
        } else if constexpr (mode == TaskMode::Thread) {
            task_ = std::make_unique<BindedThreadedTaskHandler<F, OBJ, ARGS...>>(
                std::move(callback),
                std::forward<F>(function),
                std::forward<OBJ>(obj),
                std::forward<ARGS>(args)...);
        } else {
            static_assert(mode == TaskMode::Sync || mode == TaskMode::Thread,
                        "Invalid TaskMode");
        }
    }
    template<TaskMode mode, typename F, typename OBJ, typename... ARGS>
    void emplace_binded_task(std::error_code& err, F&& function, OBJ&& obj, ARGS&&... args) {
        if constexpr (mode == TaskMode::Sync) {
            task_ = std::make_unique<TaskHandler<F, ARGS...>>(
                std::forward<F>(function),
                std::forward<OBJ>(obj),
                std::forward<ARGS>(args)...);
        } else if constexpr (mode == TaskMode::Thread) {
            task_ = std::make_unique<BindedThreadedTaskHandler<F, OBJ, ARGS...>>(
                std::forward<F>(function),
                std::forward<OBJ>(obj),
                std::forward<ARGS>(args)...);
        } else {
            static_assert(mode == TaskMode::Sync || mode == TaskMode::Thread,
                        "Invalid TaskMode");
        }
    }
    template<typename RESULT>
    bool holds() const{
        if( dynamic_cast<TypedTaskHandler<TaskMode::Sync,RESULT>*>(
                task_)!=nullptr ||
            dynamic_cast<TypedTaskHandler<TaskMode::Thread,RESULT>*>(
                task_)!=nullptr)
            return true;
        else return false;
    }
    bool request_stop(
                Timeout timeout_sec,
                std::error_code& err) noexcept{
        err.clear();
        if(auto ptr = task_.get();
            ptr!=nullptr)
            return ptr->request_stop(timeout_sec,err);
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
    virtual void on_init_connection(std::error_code& err) noexcept{}
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
                std::error_code& err) noexcept
    {
        if(event&Event::In) on_read(err);
        if(event&Event::Out) on_write(err);
        if(event&Event::EvTaskDone) on_task_done(err);
        if(event&Event::EvStopRequested) on_stop_requested(err);
    }
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
    bool is_active_request() const noexcept;
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
    virtual void on_push_request(std::error_code& err) noexcept;
    bool next_request() noexcept;
    void push_request(std::shared_ptr<Command<CommandType::RequestData>> request,
            std::error_code& err) noexcept;
};
}