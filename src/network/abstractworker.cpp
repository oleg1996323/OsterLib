#include "abstractworker.h"

namespace network{
	AbstractWorker::AbstractWorker(uint32_t order_lenght,std::error_code& err):
    		multiplexor_(order_lenght,err)
	{
		if(err!=std::error_code())
			return;
	}
	void AbstractWorker::stop(bool wait_for_end_connections,
            uint16_t timeout_sec){
		std::lock_guard lock(mutex());
        std::error_code err;
        for(auto& [conn,conn_state]:connections()){
            if(conn_state.proc_.get()!=nullptr)
                conn_state.proc_->request_stop(
                    wait_for_end_connections,
                    timeout_sec,err);
        }
        thread().request_stop();
		wake_event();
    }
	void AbstractWorker::start(){
		std::lock_guard lock(mutex());
        thread() = std::jthread(std::bind_front(&AbstractWorker::__thread_launch__,this));
    }
	void AbstractWorker::__thread_launch__(std::stop_token st) noexcept{
		std::error_code err;
		while (!st.stop_requested()) {
		auto events = wait(err,3000);
		handle_worker_commands();
		if(st.stop_requested())
			break;
		this->handle_pending(err);
		for (const auto& ev : events) {
			//if((ev.events()&Event::In)!=0)
				//std::cout<<"("<<name_<<") "<<"read event"<<std::endl;
			//if((ev.events()&Event::HangUp)!=0)
				//std::cout<<"("<<name_<<") "<<"hangup event"<<std::endl;
			//if((ev.events()&Event::Error)!=0)
				//std::cout<<"("<<name_<<") "<<"error event"<<std::endl;
			//if((ev.events()&Event::Out)!=0)
				//std::cout<<"("<<name_<<") "<<"write event"<<std::endl;
		run(ev,st,err); }}
	}
    bool AbstractWorker::set_options(
			std::error_code& err,
			const ConnectionHandle& hconn,
			std::span<std::shared_ptr<Socket::BaseOption>> options) noexcept
	{
		if(auto found = connections_.find(hconn.id());found!=connections_.end()){
			if(auto loaded = found->second.conn_->
					state_.load(std::memory_order_acquire);
				loaded != Connection::State::Active &&
				loaded != Connection::State::Connecting)
			{
				err = std::make_error_code(std::errc::not_connected);
				return false;
			}
			found->second.socket_->set_options(err,options);
			return true;
		}
		else{
			err = std::make_error_code(std::errc::no_such_device);	
			return false;
		}
    }
	bool AbstractWorker::enable_writing(
			ConnectionHandle hconn,
			bool enable,
			std::error_code& err) noexcept{
		auto found = connections().find(hconn.id());
		if (found == connections().end()) {
			err = std::make_error_code(std::errc::no_such_device);
			return false;
		}
		Event new_events = found->second.events_handled_;
		if (enable){
			new_events = new_events|Event::Out;
			assert((new_events&Event::Out)==Event::Out);
			//r1std::cout<<"enable writable: id="<<hconn.id()<<std::endl;
		}
		else{
			new_events = new_events&~Event::Out;
			assert((new_events&Event::Out)==0);
			//r1std::cout<<"disable writable: id="<<hconn.id()<<std::endl;
		}
		EventHandle ev(hconn.id(), new_events);
		bool res = modify_tracking_event(found->second.socket_->native(), ev, err);
		if (err == std::error_code()) {
			found->second.events_handled_ = new_events;
		}
		return res;
	}
	bool AbstractWorker::enable_readable(
			ConnectionHandle hconn,
			bool enable,
			std::error_code& err) noexcept{
		auto found = connections().find(hconn.id());
		if (found == connections().end()) {
			err = std::make_error_code(std::errc::no_such_device);
			return false;
		}
		Event new_events = found->second.events_handled_;
		if (enable){
			new_events = new_events|Event::In;
			//r1std::cout<<"enable readable"<<std::endl;
		}
		else{
			new_events = new_events&~Event::In;
			//r1std::cout<<"disable readable"<<std::endl;
		}
		EventHandle ev(hconn.id(), new_events);
		bool res = modify_tracking_event(found->second.socket_->native(), ev, err);
		if (err == std::error_code()) {
			found->second.events_handled_ = new_events;
		}
		return res;
	}
	bool AbstractWorker::set_option(
			std::error_code& err,
			const ConnectionHandle& hconn,
			std::shared_ptr<Socket::BaseOption> option) noexcept
	{
		if(auto found = connections_.find(hconn.id());found!=connections_.end()){
			if(auto loaded = found->second.conn_->
					state_.load(std::memory_order_acquire);
				loaded != Connection::State::Active &&
				loaded != Connection::State::Connecting)
			{
				err = std::make_error_code(std::errc::not_connected);
				return false;
			}
			found->second.socket_->set_option(option,err);
			return true;
		}
		else{
			err = std::make_error_code(std::errc::no_such_device);	
			return false;
		}
    }

	bool AbstractWorker::contains_connection(const ConnectionHandle& hconn) noexcept{
		return connections_.contains(hconn.id());
	}
	bool AbstractWorker::stop_process(const ConnectionHandle& hconn,bool wait, uint16_t timeout_sec,std::error_code& err) noexcept{
		if(auto found = connections_.find(hconn.id());found!=connections_.end()){
			if(found->second.proc_){
				found->second.proc_->request_stop(wait,timeout_sec,err);
				if(err==std::error_code())
					return true;
				else return false;
			}
			else{
				err = std::make_error_code(std::errc::no_such_process);
				return false;
			}
		}
		else{
			err = std::make_error_code(std::errc::no_such_device);
			return false;
		}
	}
    bool AbstractWorker::shutdown_connection(std::error_code& err,
			const ConnectionHandle& hconn) noexcept
	{
        if(auto found = connections_.find(hconn.id());found!=connections_.end()){
			found->second.conn_->state_.store(Connection::State::Shutdowned,
						std::memory_order_acq_rel);
			return found->second.socket_->shutdown_all(err);
		}
		else{
			err = std::make_error_code(std::errc::no_such_device);
			return false;
		}
    }
    bool AbstractWorker::close_connection(std::error_code& err,
			const ConnectionHandle& hconn) noexcept
	{
        if(auto found = connections_.find(hconn.id());found!=connections_.end()){
			found->second.conn_->state_.exchange(Connection::State::Closed,
						std::memory_order_acq_rel);
			multiplexor_.remove(found->second.socket_->native(),err);
			found->second.socket_->close();
			//r1std::cout<<"Erasing id="<<found->first<<std::endl;
			connections_.erase(found);
			return true;
        }
		else{
			err = std::make_error_code(std::errc::no_such_device);
			return false;
		}
    }

	void AbstractWorker::stop_all(uint16_t timeout_sec,bool wait,std::error_code& err) noexcept{
		std::for_each(connections().begin(),connections().end(),[&](
			const std::pair<const ConnectionId,ConnectionState>& id_stat){
			ConnectionHandle hconn(this,id_stat.first);
			hconn.execute_command(
				std::move(std::make_shared<Command<CommandType::RequestStop>>(
					hconn,timeout_sec,wait)),err);			
		});
	}
	void AbstractWorker::shutdown_all(std::error_code& err) noexcept{
		std::for_each(connections().begin(),connections().end(),[&](
			const std::pair<const ConnectionId,ConnectionState>& id_stat){
			ConnectionHandle hconn(this,id_stat.first);
			hconn.execute_command(
				std::move(std::make_shared<Command<CommandType::ShutDownConnection>>(
					hconn)),err);
		});
	}
	void AbstractWorker::remove_all(std::error_code& err) noexcept{
		std::for_each(connections().begin(),connections().end(),[&](
			const std::pair<const ConnectionId,ConnectionState>& id_stat){
			ConnectionHandle hconn(this,id_stat.first);
			hconn.execute_command(
				std::move(std::make_shared<Command<CommandType::RemoveConnection>>(
					hconn)),err);
		});
	}
	std::unique_ptr<ConnectionIO> AbstractWorker::make_connectionIO(
				Socket& sock_ptr,
				ConnectionHandle hconn,
				size_t recv_buf_sz,
				const Event& events,
				std::error_code& err) noexcept
	{
		err.clear();
		//@todo make configurable buffer size
		return std::make_unique<ConnectionIO>(sock_ptr,hconn,&events,recv_buf_sz,err);
	}
	std::jthread& AbstractWorker::thread() const noexcept{
		return thread_;
	}
	std::mutex& AbstractWorker::mutex() const noexcept{
		return m_;
	};
	void AbstractWorker::wake_event() noexcept{
		multiplexor_.interrupt();
	}
	std::span<network::EventHandle> AbstractWorker::wait(std::error_code& err,
        	int32_t timeout) noexcept{
		return multiplexor_.wait(err,timeout);
	}
	Socket* AbstractWorker::socket_by_id(ConnectionId id) noexcept{
		return const_cast<AbstractWorker*>(this)->socket_by_id(id);
	}
	const Socket* AbstractWorker::socket_by_id(ConnectionId id) const noexcept{
		if(auto found = connections().find(id);found!=connections().end())
			return found->second.socket_.get();
		else return nullptr;
	}
	const AbstractWorker::ConnectionState* AbstractWorker::connection_state_by_id(ConnectionId id) const noexcept{
		if(auto found = connections().find(id);found!=connections().end())
			return &found->second;
		else return nullptr;
	}
	AbstractWorker::ConnectionState* AbstractWorker::connection_state_by_id(ConnectionId id) noexcept{
		if(auto found = connections().find(id);found!=connections().end())
			return &found->second;
		else return nullptr;
	}
	const std::unordered_map<ConnectionId,
        AbstractWorker::ConnectionState>& AbstractWorker::connections() const noexcept{
		return this->connections_;
	}
	std::unordered_map<ConnectionId,
        AbstractWorker::ConnectionState>& AbstractWorker::connections() noexcept{
		return this->connections_;
	}
	Connection::Properties AbstractWorker::connection_properties(ConnectionHandle hconn) const noexcept{
		std::lock_guard lock(mutex());
		auto connstat = connection_state_by_id(hconn.id());
		if(connstat && connstat->conn_){
			return Connection::Properties{
					.address = connstat->conn_->address(),
					.state = connstat->conn_->state()};
		}
		else return Connection::Properties{};
	}
	void AbstractWorker::set_connection_state(Connection* conn,Connection::State state) noexcept{
		if(conn){
			conn->state_.store(
				state,std::memory_order::release);
			return;
		}
		assert(false);
	}
	ConnectionHandle AbstractWorker::connection_handle(ConnectionId id) noexcept{
		return ConnectionHandle(this,id);
	}
	bool AbstractWorker::remove_tracking_event(FileDescriptor fd,std::error_code& err) noexcept{
		return multiplexor_.remove(fd,err);
	}
	bool AbstractWorker::add_tracking_event(
				FileDescriptor fd,
				EventHandle ev,
				std::error_code& err) noexcept{
		return multiplexor_.add(fd,ev,err);
	}
	bool AbstractWorker::modify_tracking_event(
				FileDescriptor fd,
				EventHandle ev,
				std::error_code& err) noexcept{
		return multiplexor_.modify(fd,ev,err);
	}
    bool AbstractWorker::handle_events(
				ConnectionHandle hconn,
				ConnectionState& state,
				Event events,
				std::error_code& err) noexcept
    {
		if(events&Event::Error || events&Event::HangUp){
			//r1std::cout<<"Handling error: id="<<hconn.id()<<std::endl;
			state.conn_->state_ = Connection::State::Closed;
			state.socket_->close();
			state.proc_.reset();
			state.connIO_.reset();
			push_command(
				std::make_shared<Command<CommandType::RemoveConnection>>(
					hconn,0,false));
			if(err!=std::error_code()){
				std::lock_guard lk(mutex());
				connections().erase(hconn.id());
			}
			return false;
		}
		else return true;
    }
	std::shared_ptr<BaseCommand> AbstractWorker::extract_command() noexcept{
		std::lock_guard lk(m_);
		if(cmds_.empty()){
			return std::shared_ptr<BaseCommand>();
		}
		else {
			auto result = std::move(cmds_.front());
			cmds_.pop();
			return result;
		}
	}
	void AbstractWorker::push_command(std::shared_ptr<BaseCommand> cmd) noexcept{
		{
			std::lock_guard lk(m_);
			cmds_.push(std::move(cmd));
		}
		wake_event();
	}
	void AbstractWorker::push_commands(std::vector<std::shared_ptr<BaseCommand>>&& cmds) noexcept{
		if(cmds.empty())
			return;
		{
			std::lock_guard lk(m_);
			for(auto&& cmd:cmds){
				cmds_.push(cmd);
			}
		}
		wake_event();
	}
}