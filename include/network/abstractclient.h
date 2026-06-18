#pragma once
#include <unistd.h>
#include <sys/eventfd.h>
#include <poll.h>
#include <variant>
#include <netdb.h>
#include "commonsocket.h"
#include "abstractprocess.h"
#include "threadpool.h"
#include "clientsettings.h"

namespace network{
    class ClientsHandler;
}
namespace network{
    class AbstractClient{
        std::unique_ptr<Worker> connections_;
        protected:
        /**
         * @brief don't block the GUI if integrated
         */
        virtual void before_connect(){}
        virtual void after_connect(ConnectionHandle hconn){}
        virtual AbstractClient& before_disconnect(
                    ConnectionHandle hconn,
                    std::error_code& err){return *this;}
        virtual AbstractClient& after_disconnect(
                    ConnectionHandle hconn,
                    std::error_code& err){return *this;}
        Worker* connections() const{
            return connections_.get();
        }
        public:
        AbstractClient(std::error_code& err,uint16_t events_in_order):
            connections_(std::make_unique<Worker>("client",events_in_order,err))
        {
                if(err){
                    std::cout<<err.message()<<std::endl;
                    connections_.reset();
                }
                else connections_->start();
        }
        AbstractClient(AbstractClient&& other) noexcept:
        connections_(std::move(other.connections_)){}
        AbstractClient& operator=(AbstractClient&& other) noexcept{
            if(this!=&other){
                connections_.swap(other.connections_);
            }
            return *this;
        }
        bool operator==(const AbstractClient& other) = delete;
        ~AbstractClient(){
            std::error_code err;
            connections_.reset();
        }
        std::shared_ptr<Command<
            CommandType::RemoveConnection>> 
                disconnect(ConnectionHandle hconn,std::error_code& err){
                before_disconnect(hconn,err);
            auto cmd = std::make_shared<
                Command<CommandType::RemoveConnection>>(hconn,0);
            connections_->push_command(cmd);
            return cmd;
        }
        ConnectionHandle connect(const std::string& host,
                uint16_t port,
                Socket::Type type,
                Protocol proto,
                const client::Settings& settings,
                std::error_code& err)
        {
            if(!connections_){
                return ConnectionHandle(nullptr);
            }
            ConnectionHandle hconn(connections_.get());
            auto cmd = std::make_shared<Command<CommandType::AddConnection>>(
                    hconn,
                    host,
                    port,
                    type,
                    proto,
                    settings);
            connections_->push_command(cmd);
            cmd->wait_ready(settings.timeout_seconds_processes_);
            if(cmd->successed()){
                err.clear();
                return hconn;
            }
            else {
                if(cmd->error().has_value())
                    err = cmd->error().value();
                else err = std::make_error_code(std::errc::not_connected);
                return ConnectionHandle(nullptr);
            }
        }
        template<typename RESULT_EXPECTED,
        typename DATA_FRAME_SEND,
		typename START_FRAME = std::monostate,
		typename END_FRAME = std::monostate>
        std::shared_ptr<RequestCommandSpec<
            RESULT_EXPECTED,
            START_FRAME,
            DATA_FRAME_SEND,
            END_FRAME>> request(
                ConnectionHandle hconn,
                START_FRAME start,
                DATA_FRAME_SEND data,
                END_FRAME end) noexcept
        {
            auto result = std::make_shared<
                    RequestCommandSpec<
                        RESULT_EXPECTED,
                        START_FRAME,
                        DATA_FRAME_SEND,
                        END_FRAME>>(
                        hconn,
                        std::forward<START_FRAME>(start),
                        std::forward<DATA_FRAME_SEND>(data),
                        std::forward<END_FRAME>(end));
            connections_->push_command(result);
            return result;
        }
    };
}