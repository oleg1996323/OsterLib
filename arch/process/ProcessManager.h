#pragma once
#include "OsterLib/network/abstractprocess.h"
#include <unordered_map>
#include <thread>

using ProcessId = uint64_t;

namespace network{

class ProcessManager{
    std::atomic<ProcessId> id_gen_{0};
    std::unordered_map<ProcessId,std::unique_ptr<Process>> processes_;
    std::mutex m_;
    public:
    ProcessManager() = default;
    
    template<typename F,typename... ARGS>
    ProcessId addSync(std::error_code& err, F&& func, ARGS&&... args) noexcept{
        std::unique_ptr<Process> proc = 
            std::make_unique<Process>(err,func,std::forward<ARGS>(args)...);
        if(err!=std::error_code())
            return 0;
        else {
            auto id = id_gen_.fetch_add(1,std::memory_order::relaxed);
            {
                std::lock_guard lk(m_);
                auto inserted = processes_.insert(
                    std::make_pair(id,std::move(proc)));
                if(!inserted.second){
                    err = std::make_error_code(std::errc::operation_canceled);
                    return 0;
                }
                else return id;
            }
        }
    }
    template<typename F,typename... ARGS>
    bool addAsync(std::error_code& err, std::stop_token stop, F&& func, ARGS&&... args) noexcept{
        std::unique_ptr<Process> proc = std::make_unique<Process>();
        proc->emplace_task(err,func,std::forward<ARGS>(args)...);
    }
    template<typename F,typename... ARGS>
    bool addThreaded(std::error_code& err, std::stop_token stop, F&& func, ARGS&&... args) noexcept{
        std::unique_ptr<Process> proc = std::make_unique<Process>();
        proc->emplace_task(err,func,std::forward<ARGS>(args)...);
    }
    bool removeProcess() noexcept;

};

}