#pragma once
#include <vector>
#include <list>
#include "abstractworker.h"

namespace network{

class Worker:public AbstractWorker{
protected:
    std::string name_;
public:
    Worker(std::string worker_name,
            uint32_t order_length,
            std::error_code& err);
    virtual ~Worker();
protected:
    virtual bool connectInternal(
            const ConnectionHandle& hconn,
            std::unique_ptr<Connection> conn,
            const client::Settings& settings,
            Socket&& socket,
            std::error_code& err) noexcept override;
    virtual bool attachConnectionInternal(
			ConnectionHandle hconn,
			std::unique_ptr<Connection> addr,
            const server::Settings& settings,
			Socket&& socket,
			std::error_code& err
			) noexcept override;
    virtual bool removeConnectionInternal(
			const ConnectionHandle& hconn,
            Timeout timeout_sec,
			std::error_code& err) noexcept override;
    virtual bool modifyConnectionInternal(
			const ConnectionHandle& hconn,
            std::span<std::shared_ptr<Socket::BaseOption>> options,
			std::error_code& err) noexcept override;
    virtual bool addConnectionProcessInternal(const ConnectionHandle& hconn,
            std::unique_ptr<AbstractConnectionProcess> proc,
            std::error_code& err) noexcept override;
    virtual bool removeConnectionProcessInternal(
            const ConnectionHandle& hconn,
            Timeout timeout_sec,
            std::error_code& err) noexcept override;
    virtual void run(EventHandle ev,std::stop_token st,std::error_code& err) override;
    virtual void after_connection(
            ConnectionState* connstat,
            std::error_code& err) noexcept{}
    virtual void after_attach_connection(
            ConnectionState* connstat,
            std::error_code& err) noexcept{}
    virtual void after_remove_connection(
            ConnectionState* connstat,
            std::error_code& err) noexcept{}
    virtual void after_modify_connection(
            ConnectionState* connstat,
            std::error_code& err) noexcept{}
    virtual void after_add_connection_process(
            ConnectionState* connstat,
            std::error_code& err) noexcept{}
    virtual void after_remove_connection_process(
            ConnectionState* connstat,
            std::error_code& err) noexcept{}
private:
    virtual void handle_pending(std::error_code& err) noexcept override;
};

class ServerWorker:public Worker{
    public:
    ServerWorker(std::string worker_name,
            uint32_t order_length,
            std::error_code& err):
            Worker(worker_name,order_length,err){}
    ~ServerWorker() = default;
    template<typename CONN_PROC>
    requires (std::is_base_of_v<AbstractConnectionProcess,CONN_PROC> ||
                std::is_same_v<CONN_PROC,AbstractConnectionProcess>)
    void set_processes(
                std::error_code& err) noexcept
    {
        //prstd::cout<<"("<<name_<<")"<<"Number connections: "<<connections().size()<<std::endl;
        for(auto& [id,conn_stat]:connections()){
            ConnectionHandle hconn = this->connection_handle(id);
            auto proc = std::make_unique<CONN_PROC>(hconn,err);
            if(err)
                continue;
            hconn.execute_command(
                std::make_shared<Command<
                    CommandType::AttachProcess>>(hconn,
                        std::move(proc)),err);
        }
    }
};

class ThreadPool {
public:
    ThreadPool(size_t num_threads,std::error_code& err);

    ~ThreadPool();

    ConnectionHandle attach_connection(
        const Address& addr,
        const server::Settings& settings,
        Socket&& socket,
        std::error_code& err) noexcept;

    ConnectionHandle attach_connection(
        const Address& addr,
        server::Settings&& settings,
        Socket&& socket,
        std::error_code& err) noexcept;

    bool modifyConnection(
                ConnectionHandle hconn,
                std::vector<std::shared_ptr<Socket::BaseOption>>&& options,
                std::error_code& err) noexcept;

    template<typename CONN_PROC>
    requires (std::is_base_of_v<AbstractConnectionProcess,CONN_PROC> ||
                std::is_same_v<CONN_PROC,AbstractConnectionProcess>)
    bool setProcesses(std::error_code& err) noexcept
    {
        for(std::unique_ptr<ServerWorker>& worker:workers_){
            if(worker)
                worker->set_processes<CONN_PROC>(err);
        }
        if(err)
            return false;
        return true;
    }

    bool removeProcess(
            ConnectionHandle hconn,
            Timeout timeout_sec,
            std::error_code& err) noexcept;
    
    void stopConnections(
                Timeout timeout_sec,
                std::error_code& err) noexcept;
    void stop(
                Timeout timeout_sec) noexcept;
private:
    std::vector<std::unique_ptr<ServerWorker>> workers_;
    std::atomic<size_t> next_worker_{0};
};
}