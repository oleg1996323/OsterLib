#include "threadpool.h"

namespace network{
    Worker::Worker(std::string worker_name, uint32_t order_length,std::error_code& err) :
    AbstractWorker(order_length,err),
    name_(worker_name){}
    Worker::~Worker(){
        std::cout<<"("<<name_<<")"<<"delete Worker "<<name_<<std::endl;
    }
    bool Worker::connectInternal(
            const ConnectionHandle& hconn,
            std::unique_ptr<Connection> conn,
            const client::Settings& settings,
            Socket&& socket,
            std::error_code& err) noexcept
    {
        if(!stop_requested() &&
            hconn.is_valid_handler())
        {
            {
                ConnectionState conn_stat = ConnectionState{
                        .conn_ = std::move(conn),
                        .proc_={},
                        .socket_ = std::make_shared<Socket>(
                            std::move(socket))};
                conn_stat.connIO_ = make_connectionIO(
                        conn_stat.socket_,hconn,1024*8,conn_stat.events_handled_,err);
                conn_stat.events_handled_=Event::Out|Event::Error|Event::HangUp;
                if(conn_stat.socket_->set_no_block(true,err)==false){
                    std::cout<<"("<<name_<<"):"<<"Connection add failed: \n";
                    auto err = std::make_error_code(
                        static_cast<std::errc>(errno));
                    return false;
                }
                EventHandle ev(hconn.id(),conn_stat.events_handled_);
                if(!add_tracking_event(conn_stat.socket_->native(),ev,err)){
                    std::cout<<"("<<name_<<"):"<<"(add tracking) Connection add failed: \n";
                    auto err = std::make_error_code(
                        static_cast<std::errc>(errno));
                    return false;
                }
                int conn_res =::connect(
                        conn_stat.socket_->native(),
                        conn_stat.conn_->address().get_sockaddr(),
                        conn_stat.conn_->address().length());
                if(conn_res == 0){
                    conn_stat.events_handled_=  Event::In|
                                                Event::EdgeTrigger|
                                                Event::HangUp|
                                                Event::Error;
                    ev.set_events(conn_stat.events_handled_);
                    if(!modify_tracking_event(conn_stat.socket_->native(),ev,err)){
                        std::cout<<"("<<name_<<"):"<<"(modify tracking) Connection add failed: \n";
                        auto err = std::make_error_code(
                            static_cast<std::errc>(errno));
                        return false;
                    }
                    auto& conn_tmp = *conn_stat.conn_;
                    auto inserted = connections().insert(std::make_pair(hconn.id(),
                        std::move(conn_stat)));
                    if(!inserted.second){
                        err = std::make_error_code(std::errc::already_connected);
                        return false;
                    }
                    set_connection_state(inserted.first->second.conn_.get(),
                        Connection::State::Active);
                    std::cout<<"("<<name_<<"):"<<" Connection add success (connected. Active): \n";
                    after_connection(&inserted.first->second,err);
                }
                else if (errno == EINPROGRESS){
                    auto inserted = connections().insert(std::make_pair(hconn.id(),
                        std::move(conn_stat)));
                    if(!inserted.second){
                        err = std::make_error_code(std::errc::already_connected);
                        return false;
                    }
                    set_connection_state(inserted.first->second.conn_.get(),
                        Connection::State::Connecting);
                    std::cout<<"("<<name_<<"):"<<"Connection add success (connecting. Connecting): \n";
                    after_connection(&inserted.first->second,err);
                }
                else{
                    std::cout<<"("<<name_<<"):"<<"Connection add failed: \n";
                    auto err = std::make_error_code(
                        static_cast<std::errc>(errno));
                    after_connection(&conn_stat,err);
                    return false;
                }
            }
            wake_event();
            return true;
        }
        else return false;
    }
    bool Worker::attachConnectionInternal(
			ConnectionHandle hconn,
			std::unique_ptr<Connection> conn,
            const server::Settings& settings,
			Socket&& socket,
			std::error_code& err
			) noexcept
    {
        if(auto found = connections().find(hconn.id());
            found==connections().end())
        {
            Event events = Event::In|Event::EdgeTrigger|Event::Error|Event::HangUp;
            EventHandle ev(hconn.id(),events);
                add_tracking_event(socket.native(),
                                ev,
                                err);
                if(err!=std::error_code()){
                    std::cout<<"("<<name_<<")"<<"(Connection attach/add_tracking_event): "<<err.message()<<std::endl;
                    return false;
                }
            auto socket_loc = std::make_shared<Socket>(std::move(socket));
            if(err!=std::error_code()){
                std::cout<<"("<<name_<<")"<<"(Connection attach/make_connectionIO): "<<err.message()<<std::endl;
                return false;
            }
            auto inserted = connections().insert(
                std::make_pair(
                    hconn.id(),
                    ConnectionState{
                        .conn_=std::move(conn),
                        .proc_={},
                        .socket_=socket_loc,
                        .events_handled_=events
                    }));
            
            if(!inserted.second){
                std::cout<<"("<<name_<<")"<<"Connection attach failed: \n";
                print_ip_port(std::cout,inserted.first->second.conn_->address());
                err = std::make_error_code(std::errc::already_connected);
                after_attach_connection(&inserted.first->second,err);
                return false;
            }
            else{
                auto connIO=make_connectionIO(
                    socket_loc,
                    hconn,
                    settings.options_.buffer_size_in_.first,
                    inserted.first->second.events_handled_,
                    err);
                inserted.first->second.connIO_=std::move(connIO);
                std::cout<<"("<<name_<<")"<<"Connection attach success: \n";
                std::cout<<"("<<name_<<") "<<"Number connections: "<<connections().size()<<std::endl;
                set_connection_state(inserted.first->second.conn_.get(),
                            Connection::State::Active);
                assert(inserted.first->second.socket_->is_non_block(err));
                print_ip_port(std::cout,inserted.first->second.conn_->address());
                after_attach_connection(&inserted.first->second,err);
                err.clear();
                return true;
            }
        }
        else{
            std::cout<<"("<<name_<<")"<<"Connection attach failed: \n";
            print_ip_port(std::cout,conn->address());
            err = std::make_error_code(std::errc::already_connected);
            after_attach_connection(&found->second,err);
            return false;
        }
    }
    bool Worker::removeConnectionInternal(
            const ConnectionHandle& hconn,
            bool wait_for_end_connections,
            uint16_t timeout_sec,
            std::error_code& err) noexcept
    {
        if(auto found = connections().find(hconn.id());
            found!=connections().end())
        {
            found->second.socket_->shutdown_all(err);
            if(err!=std::error_code()){
                std::cout<<"("<<name_<<")"<<"Erasing id="<<found->first<<std::endl;
                connections().erase(found);
                after_remove_connection(&found->second,err);
                return false;
            }
            else{
                auto res = remove_tracking_event(
                    found->second.socket_->native(),err);
                found->second.socket_->close();
                std::cout<<"("<<name_<<")"<<"Erasing id="<<found->first<<std::endl;
                after_remove_connection(&found->second,err);
                connections().erase(found);
                return res;
            }
        }
        else{
            err = std::make_error_code(
                std::errc::no_such_device);
            return false;
        }
    }
    bool Worker::modifyConnectionInternal(
            const ConnectionHandle& hconn,
            std::span<std::shared_ptr<Socket::BaseOption>> options,
            std::error_code& err) noexcept
    {
        auto connstat = connection_state_by_id(hconn.id());
        if(connstat==nullptr){
            err = std::make_error_code(std::errc::no_such_device);
            after_modify_connection(connstat,err);
            return false;
        }
        else{
            connstat->socket_->set_options(err,std::span(options));
            after_modify_connection(connstat,err);
            return true;
        }
        return true;
    }
    bool Worker::addConnectionProcessInternal(const ConnectionHandle& hconn,
            std::unique_ptr<AbstractConnectionProcess> proc,
            std::error_code& err) noexcept{
        if(!stop_requested())
        {
            ConnectionState* conn_stat=nullptr;
            {
                conn_stat = connection_state_by_id(hconn.id());
                if(conn_stat==nullptr){
                    err = std::make_error_code(std::errc::not_connected);
                    std::cout<<"("<<name_<<")"<<": (Attach process) "<<err.message()<<std::endl;
                    after_add_connection_process(conn_stat,err);
                    return false;
                }
                else{
                    if(conn_stat->conn_->state()==
                        Connection::State::Active ||
                        conn_stat->conn_->state()==
                        Connection::State::Connecting)
                    {
                        conn_stat->proc_=std::move(proc);
                        conn_stat->proc_->set_connectionIO(
                            conn_stat->connIO_.get());
                        conn_stat->events_handled_ = 
                            conn_stat->events_handled_&~(Event::EdgeTrigger|Event::Out);
                        EventHandle ev(hconn.id(),conn_stat->events_handled_);
                        if(!modify_tracking_event(conn_stat->socket_->native(),
                            ev,
                            err))
                            return false;
                        err.clear();
                        after_add_connection_process(conn_stat,err);
                        if(err)
                            return false;
                        return true;
                    }
                    else{
                        err =std::make_error_code(
                        std::errc::not_connected);
                        std::cout<<"("<<name_<<")"<<": (Attach process) "<<err.message()<<std::endl;
                        after_add_connection_process(conn_stat,err);
                        return false;
                    }
                }
            }
        }
        return false;
    }
    bool Worker::removeConnectionProcessInternal(
            const ConnectionHandle& hconn,
            bool wait_for_end_connections,
            uint16_t timeout_sec,
            std::error_code& err) noexcept
    {
        ConnectionState* conn_stat=nullptr;
        conn_stat = connection_state_by_id(hconn.id());
        if(conn_stat==nullptr){
            err =std::make_error_code(
                std::errc::not_connected);
                after_remove_connection_process(conn_stat,err);
                return false;
        }
        else{
            conn_stat->proc_.reset();
            EventHandle ev(hconn.id(),conn_stat->events_handled_|Event::EdgeTrigger);
            if(!modify_tracking_event(conn_stat->socket_->native(),ev,err))
                return false;
            if(err!=std::error_code()){
                std::cout<<"("<<name_<<")"<<"(Remove process) modify_tracking_event error: "
                <<err.message()<<std::endl;
                after_remove_connection_process(conn_stat,err);
                return false;
            }
            conn_stat->events_handled_=Event::In|Event::EdgeTrigger;
            after_remove_connection_process(conn_stat,err);
            return true;
        }
    }
    void Worker::run(std::stop_token st,std::error_code& err){
        while (!stop_requested()) {
            auto events = wait(err,3000);
            handle_worker_commands();
            if(st.stop_requested())
                break;
            this->handle_pending(err);
            for (const auto& ev : events) {
                if((ev.events()&Event::In)!=0)
                    std::cout<<"("<<name_<<") "<<"read event"<<std::endl;
                if((ev.events()&Event::HangUp)!=0)
                    std::cout<<"("<<name_<<") "<<"hangup event"<<std::endl;
                if((ev.events()&Event::Error)!=0)
                    std::cout<<"("<<name_<<") "<<"error event"<<std::endl;
                if((ev.events()&Event::Out)!=0)
                    std::cout<<"("<<name_<<") "<<"write event"<<std::endl;
                Event e = ev.events();
                {
                    ConnectionState* conn_stat=nullptr;
                    conn_stat = connection_state_by_id(ev.get_as_32());
                    
                    if(!conn_stat){
                        err = std::make_error_code(std::errc::no_such_device);
                        continue;
                    }
                    if(!handle_events(connection_handle(ev.get_as_32()),*conn_stat,e,err))
                        continue;
                    //delete
                    if(name_=="server 0"){
                        if(!conn_stat->proc_)
                            assert(conn_stat->events_handled_&(Event::EdgeTrigger|Event::In));
                        else assert(conn_stat->events_handled_&Event::In ||
                            conn_stat->events_handled_&(Event::Out|Event::In));
                    }
                    if(conn_stat->conn_->state()==
                        Connection::State::Connecting &&
                        ev.events() & Event::Out)
                    {
                        if (auto sock_error = conn_stat->socket_->error(err);
                            sock_error==std::error_code()) 
                        {
                            set_connection_state(conn_stat->conn_.get(),
                            Connection::State::Active);
                            EventHandle ev_tmp = ev;
                            ev_tmp.set_events(
                                Event::In|Event::EdgeTrigger|Event::HangUp|Event::Error);
                            if(!modify_tracking_event(conn_stat->socket_->native(),
                                            ev_tmp,
                                            err)){
                                std::cout<<"("<<name_<<")"<<"Connection add failed: \n";
                                std::cout<<"("<<name_<<")"<<err.message()<<std::endl;
                                push_command(std::make_shared<Command<
                                    CommandType::RemoveConnection>>(
                                        connection_handle(ev.get_as_32())));
                            }
                            else{
                                conn_stat->events_handled_ = 
                                    Event::In|Event::EdgeTrigger|Event::HangUp|Event::Error;
                                std::cout<<"("<<name_<<")"<<"Connection established: \n";
                                print_ip_port(std::cout,conn_stat->conn_->address());
                            }
                        } else {
                            std::vector<std::shared_ptr<BaseCommand>> cmds;
                            cmds.emplace_back(std::make_shared<Command<
                                CommandType::RemoveConnection>>(
                                    connection_handle(ev.get_as_32())));
                            push_commands(std::move(cmds));
                            std::cout<<"("<<name_<<")"<<"Connection add failed: \n";
                            std::cout<<"("<<name_<<")"<<err.message()<<std::endl;
                            continue;
                        }
                    }
                    if (conn_stat->proc_)
                    {
                        std::error_code err;
                        conn_stat->proc_->handle_event(e,err);
                        if(err!=std::error_code())
                        switch(static_cast<std::errc>(err.value())){
                            case std::errc::operation_in_progress:
                            case std::errc::no_buffer_space:
                            case std::errc::resource_unavailable_try_again:
                                err.clear();
                                break;
                            default:
                                conn_stat->socket_->close();
                                conn_stat->proc_.reset();
                                conn_stat->connIO_.reset();
                                push_command(std::make_shared<Command<
                                CommandType::RemoveConnection>>(
                                    connection_handle(ev.get_as_32())));
                        }
                    }
                }
            }
        }
    }

    void Worker::handle_pending(std::error_code& err) noexcept{
        while(auto cmd = extract_command())
            cmd->execute(this);
    }

    ThreadPool::ThreadPool(size_t num_threads,std::error_code& err) {
        for (size_t i = 0; i < num_threads; ++i) {
            using namespace std::string_literals;
            workers_.emplace_back(std::make_unique<ServerWorker>(
                "server "s+std::to_string(i),24,err));
            if(err!=std::error_code()){
                workers_.clear();
                break;
            }
            workers_.back()->start();
        }
    }

    ThreadPool::~ThreadPool() {
        for (auto& w : workers_) w->command_worker(Worker::WorkerCommand::Stop);
    }

    ConnectionHandle ThreadPool::attach_connection(
        const Address& addr,
        const server::Settings& settings,
        Socket&& socket,
        std::error_code& err) noexcept
    {
        size_t index = next_worker_++ % workers_.size();
        ConnectionHandle hconn(workers_[index].get());
        hconn.execute_command(
                std::make_shared<Command<CommandType::AttachConnection>>(
                    hconn,
                    std::make_unique<Connection>(addr),
                    settings,
                    std::move(socket)),
                    err);
        return hconn;
    }
    ConnectionHandle ThreadPool::attach_connection(
        const Address& addr,
        server::Settings&& settings,
        Socket&& socket,
        std::error_code& err) noexcept
    {
        size_t index = next_worker_++ % workers_.size();
        ConnectionHandle hconn(workers_[index].get());
        hconn.execute_command(
                std::make_shared<Command<CommandType::AttachConnection>>(
                    hconn,
                    std::make_unique<Connection>(addr),
                    std::move(settings),
                    std::move(socket)),
                    err);
        return hconn;
    }

    bool ThreadPool::modifyConnection(
                ConnectionHandle hconn,
                std::vector<std::shared_ptr<Socket::BaseOption>>&& options,
                std::error_code& err) noexcept
    {
        return hconn.execute_command(
                std::make_shared<Command<CommandType::ModifyConnection>>(
                    hconn,std::move(options)),err);
    }
    
    void ThreadPool::stopConnections(
                bool wait_for_end_connections,
                uint16_t timeout_sec,
                std::error_code& err) noexcept
    {
        for (auto& w : workers_) 
            w->stop_all(
                timeout_sec,
                wait_for_end_connections,
                err);
    }
    void ThreadPool::stop(
                bool wait_for_end_connections,
                uint16_t timeout_sec) noexcept
    {
        for (auto& w : workers_) 
            w->command_worker(Worker::WorkerCommand::Stop);
    }
}