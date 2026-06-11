#include "connection.h"
#include "abstractserver.h"
#include "abstractworker.h"

namespace network{

std::unique_ptr<ConnectionAcceptor> make_connection_acceptor(
        AbstractServer* owner,
        const std::string& host,
        Port port,
        Socket::Type type,
        Protocol proto,
        uint32_t max_accept_queue,
        std::vector<std::shared_ptr<Socket::BaseOption>>&& acceptor_options,
        std::vector<std::shared_ptr<Socket::BaseOption>>&& accepted_socket_options,
        std::error_code& err)
{
    if(!owner)
        throw std::invalid_argument("nullptr owner");
    Address addr = make_address(host,port,err);
    return std::make_unique<ConnectionAcceptor>(err,
            owner,
            addr,
            std::move(acceptor_options),
            std::move(accepted_socket_options),
            max_accept_queue);
}
std::unique_ptr<Connection> make_connection(
            AbstractWorker* owner,
            const std::string& host,
            Port port,
            Socket::Type type,
            Protocol proto,
            std::error_code& err) noexcept
{
    Address addr = make_address(host,port,err);
    return std::make_unique<Connection>(addr);
}

void ConnectionAcceptor::listen(std::error_code& err) noexcept{
    if(!socket_){
        err = std::make_error_code(std::errc::no_such_device);
        return;
    }
    before_listen(err);
    if(auto res = ::listen(socket_->native(),number_listened_);res==-1){
        auto err = std::make_error_code(static_cast<std::errc>(errno));
        errno = 0;
        return;
    }
    after_listen(err);
}

void ConnectionAcceptor::graceful_close(std::error_code& err) noexcept
{
    std::lock_guard lock(m_);
    if(socket_){
        socket_->shutdown_all(err);
        event_handler_->remove(socket_->native(),err);
        socket_.reset();
    }
    if(thread_)
        thread_->request_stop();
}

void ConnectionAcceptor::accept_error_handling(
        std::error_code& err) noexcept
{
    err = 
        std::make_error_code(static_cast<std::errc>(errno));
    errno = 0;
    std::printf("%s",err.message().c_str());
}

void ConnectionAcceptor::launch() noexcept{
    std::lock_guard lock(m_);
    thread_ = std::make_unique<std::jthread>([this]
        (std::stop_token st)
    {
        //std::cout<<"acceptor launched"<<std::endl;
        std::error_code err;
        socket_=std::move(make_socket(err));
        if(!socket_)
        {
            err = std::make_error_code(
                std::errc::bad_file_descriptor);
            //std::cout<<"Acceptor: "<<err.message()<<std::endl;
            return;
        }
        else if(err!=std::error_code()){
            //std::cout<<"Acceptor: "<<err.message()<<std::endl;
            return;
        }
        else if(!event_handler_->add(
                socket_->native(),
                Event::In | Event::EdgeTrigger,
                err))
        {
            //std::cout<<"Acceptor: "<<err.message()<<std::endl;
            graceful_close(err);
            return;
        }
        listen(err);
        accept(st,err);
    });
}

std::unique_ptr<Socket> ConnectionAcceptor::make_socket(
        std::error_code& err) noexcept
{
    Address addr_;
    {
        std::lock_guard lock(m_);
        addr_ = conn_.address();
    }
    if(Socket sock = socket(addr_,
        sock_type_,
        used_protocol_,
        err);
        sock.native()==-1)
    {
        err = std::make_error_code(static_cast<std::errc>(errno));
        //std::cout<<"Acceptor "<<err.message()<<std::endl;
        return {};
    }
    else {
        sock.bind(addr_,err);
        if(err!=std::error_code())
            return {};
        if(sock.set_options(err,std::span(acceptor_options_)))
        {
            err.clear();
            return std::make_unique<Socket>(std::move(sock));
        }
        else
        {
            err = std::make_error_code(static_cast<std::errc>(errno));
            //std::cout<<"Acceptor "<<err.message()<<std::endl;
            return {};
        }
    }
}

bool ConnectionAcceptor::stop(std::error_code& err) noexcept
{
    if(thread_){
        std::lock_guard lock(m_);
        bool stopped = thread_->request_stop();
        event_handler_->interrupt();
        thread_.reset();
        return stopped;
    }
    else return false;
}
bool ConnectionAcceptor::set_options(std::error_code& err,std::span<
            std::shared_ptr<Socket::BaseOption>> options) noexcept
{
    if(!socket_){
        err = std::make_error_code(std::errc::no_such_device);
        return false;
    }
    std::lock_guard lock(m_);
    return socket_->set_options(err,std::move(options));
}
bool ConnectionAcceptor::stopped() const noexcept{
    return thread_.get()==nullptr;
}
bool ConnectionAcceptor::shutdown_read(std::error_code& err) noexcept
{
    if(!socket_){
        err = std::make_error_code(std::errc::no_such_device);
        return false;
    }
    std::lock_guard lock(m_);
    return socket_->shutdown_read(err);
}
bool ConnectionAcceptor::shutdown_write(std::error_code& err) noexcept
{
    if(!socket_){
        err = std::make_error_code(std::errc::no_such_device);
        return false;
    }
    std::lock_guard lock(m_);
    return socket_->shutdown_write(err);
}
bool ConnectionAcceptor::shutdown_all(std::error_code& err) noexcept
{
    if(!socket_){
        err = std::make_error_code(std::errc::no_such_device);
        return false;
    }
    std::lock_guard lock(m_);
    return socket_->shutdown_all(err);
}

void ConnectionAcceptor::accept(std::stop_token stop,std::error_code& err) noexcept
{
    while(!stop.stop_requested()){
        for(auto event: event_handler_->wait(err,-1)){
            if(event.get_as_fd()==socket_->native()){
                sockaddr_storage storage;
                socklen_t sz = sizeof(sockaddr_in6);
                before_accept();
                if(int raw_sock = ::accept(socket_->native(),
                        reinterpret_cast<sockaddr*>(&storage),&sz);
                        raw_sock==-1)
                {
                    //std::cout<<"Connection refused: ";
                    Address addr(storage,err);
                    //print_ip_port(std::cout,addr);
                    accept_error_handling(err);
                    continue;
                }
                else
                {
                    Address addr(storage,err);
                    Socket socket(raw_sock);
                    socket.set_no_block(true,err);
                    if(err!=std::error_code()){
                        //std::cout<<"set non-block socket error"<<std::endl;
                        continue;
                    }
                    
                    
                    auto hconn = owner_->attach_connection(
                        addr,
                        std::move(socket),
                        err);

                    std::unique_ptr<AbstractConnectionProcess> process;
                    {
                        std::lock_guard lock(m_);
                        if(process_fabrique_)
                            process = process_fabrique_->make_process(hconn,err);
                            
                    }
                    if(process)
                        hconn.execute_command(
                        std::make_shared<Command<CommandType::AttachProcess>>(
                            hconn,std::move(process)),err);
                    if(hconn.is_valid_handler())
                    {
                        after_accept();
                        //std::cout<<"Connection accepted: "<<std::endl;
                        //print_ip_port(std::cout,addr);
                        continue;
                    }
                    else{
                        //std::cout<<"Connection refused: "<<std::endl;
                        //print_ip_port(std::cout,addr);
                        continue;
                    }
                }
            }
        }
    }
}
}