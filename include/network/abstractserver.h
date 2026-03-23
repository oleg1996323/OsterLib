#pragma once

#include <netinet/in.h>
#include <unistd.h>
#include <list>
#include <thread>
#include <sys/eventfd.h>
#include "definitions.h"
#include "serversettings.h"
#include "concepts.h"
#include "commonsocket.h"
#include "multiplexor.h"
#include <stdexcept>
#include <set>
#include "threadpool.h"
#include <unordered_set>

namespace network{
    class AbstractServer{
        std::unique_ptr<ConnectionAcceptor> accepter_;
        std::unique_ptr<ThreadPool> processes_pool_;

        public:
        template<typename T>
        requires (std::is_base_of_v<AbstractConnectionProcess,T>)
        void set_processes_at_connections() noexcept{
            if(accepter_)
                accepter_->set_processes_at_connections<T>();
        }

        ConnectionHandle attach_connection(
            const Address& addr,
            Socket&& socket,
            std::error_code& err)
        {
            return processes_pool_->attach_connection(
                addr,
                std::move(socket),
                err);
        }
        bool remove_connection(
                ConnectionHandle hconn,
                bool wait,
                uint16_t timeout_sec,
                std::error_code& err)
        {
            return hconn.execute_command(
                    std::make_shared<Command<CommandType::RemoveConnection>>(
                    hconn,timeout_sec,wait),
                    err);
        }
        bool modify_connection(
                ConnectionHandle hconn,
                std::vector<std::shared_ptr<
                    network::Socket::BaseOption>> conn_options,
                std::error_code& err)
        {
            return hconn.execute_command(
                    std::make_shared<Command<CommandType::ModifyConnection>>(
                        hconn,std::move(conn_options)),err);
        }

        template<typename CONN_PROC>
        requires (std::is_base_of_v<AbstractConnectionProcess,CONN_PROC> ||
                std::is_same_v<CONN_PROC,AbstractConnectionProcess>)
        bool set_process(std::error_code& err){
            return processes_pool_->setProcesses<CONN_PROC>(err);
        }
        
        AbstractServer(){}
        virtual ~AbstractServer(){
            if(accepter_)
                accepter_.reset();
        }
        void configure(const server::Settings& settings,
                std::vector<std::shared_ptr<Socket::BaseOption>>&& acceptor_options,
                std::vector<std::shared_ptr<Socket::BaseOption>>&& sock_accepted_options,
                std::error_code& err) noexcept{
            if(!is_launched()){
                processes_pool_ = std::move(
                    std::make_unique<ThreadPool>(settings.num_threads_pool_,
                        err));
                if(err!=std::error_code()){
                    processes_pool_.reset();
                    return;
                }
                
                accepter_ = make_connection_acceptor(
                        this,
                        settings.host_,
                        settings.port_,
                        Socket::Type::Stream,
                        settings.protocol_,
                        settings.number_events_,
                        std::move(acceptor_options),
                        std::move(sock_accepted_options),
                        err);
            }
            else{
                err = std::make_error_code(std::errc::connection_already_in_progress);
                return;
                //throw std::runtime_error("Server is not stopped for further configuration!");
            }
        }
        void close(bool wait_for_end_connections = false,
                uint16_t timeout_sec = 60) noexcept;
        void collapse(bool wait_for_end_connections = false,
                uint16_t timeout_sec = 60) noexcept;
        void launch(std::error_code& err) noexcept{
            if(!accepter_){
                err=std::make_error_code(std::errc::operation_not_permitted);
                //throw std::runtime_error("Server not configured");
                return;
            }
            accepter_->launch();
        }
        bool is_launched() const{
            return accepter_.get()!=nullptr;
        }
    };
}