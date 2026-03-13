#include "abstractworker.h"

namespace network{
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
			std::cout<<"Erasing id="<<found->first<<std::endl;
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
				std::shared_ptr<Socket> sock_ptr,
				ConnectionHandle hconn,
				const Event& events,
				std::error_code& err) noexcept
	{
		if(sock_ptr){
			err.clear();
			//@todo make configurable buffer size
			return std::make_unique<ConnectionIO>(sock_ptr,hconn,&events,4096,err);
		}
		else{
			err = std::make_error_code(std::errc::invalid_argument);
			return {};
		}
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
	std::shared_ptr<Socket> AbstractWorker::socket_by_id(ConnectionId id) noexcept{
		if(auto found = connections().find(id);found!=connections().end())
			return found->second.socket_;
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
	void AbstractWorker::set_connection_state(Connection* conn,Connection::State state) noexcept{
		if(conn)
			conn->state_.store(
				state,std::memory_order::release);
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
			std::cout<<"Handling error: id="<<hconn.id()<<std::endl;
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
			return true;
		}
		else return false;
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