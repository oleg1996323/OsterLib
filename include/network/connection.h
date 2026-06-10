#pragma once
#include "commonsocket.h"
#include <vector>
#include "multiplexor.h"
#include "address.h"
#include "serialization.h"

namespace network{
class Connection;
class AbstractWorker;

std::unique_ptr<ConnectionAcceptor> make_connection_acceptor(
        AbstractServer* owner,
        const std::string& host,
        Port port,
        Socket::Type type,
        Protocol proto,
        uint32_t max_accept_queue,
        std::vector<std::shared_ptr<Socket::BaseOption>>&& acceptor_options,
        std::vector<std::shared_ptr<Socket::BaseOption>>&& accepted_socket_options,
        std::error_code& err);
std::unique_ptr<Connection> make_connection(
                AbstractWorker* owner,
                const std::string& host,
                Port port,
                Socket::Type type,
                Protocol proto,
                std::error_code& err) noexcept;

class Connection{
    public:
    enum class State: uint8_t
    {
        Closed,
        Connecting,
        Active,
        Shutdowned,
        Closing,
    };
    private:
    friend class AbstractWorker;
    friend class FileSender;
    friend class Multiplexor;
    private:
    Address storage_;
    std::atomic<State> state_;
    public:
    struct Properties{
        Address address;
        State state;
    };
    Connection(Address addr) noexcept:
        storage_(std::move(addr)){}
    Connection(Connection&& other) = delete;
    Connection(const Connection& other) = delete;
    Connection& operator=(Connection&& other) = delete;
    Connection& operator=(const Connection& other) = delete;
    ~Connection() = default;
    State state() noexcept{
        return state_.load(std::memory_order::relaxed);
    }
    const Address& address() const{
        return storage_;
    }
    void print_address_info(std::ostream& stream) const;
    std::string ip_to_text() const{
        return network::ip_to_text(storage_);
    }
    std::string port_to_text() const{
        return network::port_to_text(storage_);
    }
};
}

#include "abstractworker.h"
#include "sys/select.h"
namespace network{
    class AbstractServer;
    class ConnectionAcceptor final
    {
        struct AbstractProcessFabrique{
            virtual std::unique_ptr<AbstractConnectionProcess> 
                    make_process(
                        ConnectionHandle hconn,
                        std::error_code& err) const noexcept = 0;
        };
        template<typename T>
        requires (std::is_base_of_v<AbstractConnectionProcess,T>)
        struct MakeProcess:public AbstractProcessFabrique{
            using process_v = T;
            virtual std::unique_ptr<AbstractConnectionProcess> 
                    make_process(
                        ConnectionHandle hconn,
                        std::error_code& err) const noexcept override
            {
                return std::make_unique<T>(hconn,err);
            }
        };
        
        std::unique_ptr<Socket> socket_;
        std::unique_ptr<Multiplexor> event_handler_;
        std::vector<std::shared_ptr<Socket::BaseOption>> acceptor_options_;
        std::vector<std::shared_ptr<Socket::BaseOption>> sock_accepted_options_;
        Socket::Type sock_type_{Socket::Type::Stream};
        Protocol used_protocol_{Protocol::TCP};
        Connection conn_;
        AbstractServer* owner_;
        uint32_t number_listened_{0};
        std::unique_ptr<AbstractProcessFabrique> process_fabrique_{};
        std::mutex m_;
        std::unique_ptr<std::jthread> thread_;
        
        void accept_error_handling(
                std::error_code& err) noexcept;
        protected:
        virtual void before_listen(std::error_code& err){}
        virtual void after_listen(std::error_code& err){}
        void listen(std::error_code& err) noexcept;
        virtual void before_accept(){}
        virtual void after_accept(){}
        void accept(std::stop_token token,std::error_code& err) noexcept;
        std::unique_ptr<Socket> make_socket(
            std::error_code& err,
            const std::vector<std::shared_ptr<Socket::BaseOption>>& options={}) noexcept;
        void graceful_close(std::error_code& err) noexcept;
        public:
        ConnectionAcceptor(
                std::error_code& err,
                AbstractServer* owner,
                Address addr,
                std::vector<std::shared_ptr<Socket::BaseOption>>&& acceptor_options,
                std::vector<std::shared_ptr<Socket::BaseOption>>&& sock_accepted_options,
                uint32_t accept_queue):
                owner_(owner),
                event_handler_(std::make_unique<Multiplexor>(accept_queue,err)),
                conn_(addr),
                acceptor_options_(std::move(acceptor_options)),
                sock_accepted_options_(std::move(sock_accepted_options)){}
        ConnectionAcceptor(
                ConnectionAcceptor&& other) noexcept = delete;
        ConnectionAcceptor& operator=(
                ConnectionAcceptor&& other) noexcept = delete;
        ConnectionAcceptor(
                const volatile ConnectionAcceptor&) = delete;
        ConnectionAcceptor& operator=(
                const ConnectionAcceptor&) = delete;
        ~ConnectionAcceptor(){
            if(thread_)
                thread_->request_stop();
            if(event_handler_){
                event_handler_->interrupt();
            }
        }
        template<typename T>
        requires (std::is_base_of_v<AbstractConnectionProcess,T>)
        void set_processes_at_connections() noexcept{
            std::lock_guard lock(m_);
            process_fabrique_ = std::make_unique<MakeProcess<T>>();
        }
        void set_listened_backlog(uint32_t backlog) noexcept{
            std::lock_guard lock(m_);
            number_listened_ = backlog;
        }
        void launch() noexcept;
        bool stop(std::error_code& err) noexcept;
        bool set_option(
                const std::shared_ptr<Socket::BaseOption>& option,
                std::error_code& err) noexcept;
        bool set_options(std::error_code& err,std::span<
                    std::shared_ptr<Socket::BaseOption>> options) noexcept;
        bool stopped() const noexcept;
        bool shutdown_read(std::error_code& err) noexcept;
        bool shutdown_write(std::error_code& err) noexcept;
        bool shutdown_all(std::error_code& err) noexcept;
    };
}