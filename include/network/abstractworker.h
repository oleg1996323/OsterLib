#pragma once
#include "multiplexor.h"
#include "abstractprocess.h"
#include "connection.h"
#include <queue>
#include <span>
#include "worker/command.h"
#include "connectionIO.h"

namespace network{
class AbstractProcess;
class AbstractWorker{
	public:
	enum class WorkerCommand{
		None,
		Stop
	};
	template<CommandType T,typename... ARGS>
	friend struct Command;
	struct ConnectionState{
		std::unique_ptr<Connection> conn_;
		std::unique_ptr<AbstractConnectionProcess> proc_;
		std::unique_ptr<ConnectionIO> connIO_;
		std::shared_ptr<Socket> socket_;
		Event events_handled_;
	};
    AbstractWorker(uint32_t order_lenght,std::error_code& err):
    		multiplexor_(order_lenght,err)
	{
		if(err!=std::error_code())
			return;
	}
	virtual ~AbstractWorker(){
		if(stop_possible())
			thread().request_stop();
		std::cout<<"Stop requested"<<std::endl;
		wake_event();
		if (thread().joinable()) thread().join();
		std::cout<<"thread joined"<<std::endl;
	}
	void start(){
        thread() = std::jthread([this](std::stop_token st) { 
            std::error_code err;
            run(st,err); });
		stop_ = thread().get_stop_source().get_token();
    }
	void push_command(std::shared_ptr<BaseCommand> cmd) noexcept;
	void push_commands(std::vector<std::shared_ptr<BaseCommand>>&& cmds) noexcept;
	bool enable_writing(
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
			std::cout<<"enable writable"<<std::endl;
		}
		else{
			new_events = new_events&~Event::Out;
			std::cout<<"disable writable"<<std::endl;
		}
		EventHandle ev(hconn.id(), new_events);
		bool res = modify_tracking_event(found->second.socket_->native(), ev, err);
		if (err == std::error_code()) {
			found->second.events_handled_ = new_events;
		}
		return res;
	}
	bool enable_readable(
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
			std::cout<<"enable readable"<<std::endl;
		}
		else{
			new_events = new_events&~Event::In;
			std::cout<<"disable readable"<<std::endl;
		}
		EventHandle ev(hconn.id(), new_events);
		bool res = modify_tracking_event(found->second.socket_->native(), ev, err);
		if (err == std::error_code()) {
			found->second.events_handled_ = new_events;
		}
		return res;
	}

	bool contains_connection(const ConnectionHandle& hconn) noexcept;
	bool stop_process(const ConnectionHandle& hconn,bool wait, uint16_t timeout_sec,std::error_code& err) noexcept;
    bool shutdown_connection(std::error_code& err,
			const ConnectionHandle& hconn) noexcept;
    bool close_connection(std::error_code& err,
			const ConnectionHandle& hconn) noexcept;
	bool set_options(
			std::error_code& err,
			const ConnectionHandle& hconn,
			std::span<std::shared_ptr<Socket::BaseOption>> options) noexcept;
	bool set_option(
			std::error_code& err,
			const ConnectionHandle& hconn,
			std::shared_ptr<Socket::BaseOption> option) noexcept;
	void stop_all(uint16_t timeout_sec,bool wait,std::error_code& err) noexcept;
	void shutdown_all(std::error_code& err) noexcept;
	void remove_all(std::error_code& err) noexcept;
	virtual void run(std::stop_token st,std::error_code& err) = 0;
	void command_worker(WorkerCommand cmd) noexcept{
		{
			std::lock_guard lk(mutex());
			w_cmds_.push(cmd);
		}
		wake_event();
	}
	Connection::Properties connection_properties(ConnectionHandle hconn) const noexcept;
protected:
	void stop(bool wait_for_end_connections,
            uint16_t timeout_sec){
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
	virtual bool connectInternal(
			const ConnectionHandle& hconn,
			std::unique_ptr<Connection> conn,
            Socket&& socket,std::error_code& err) noexcept = 0;
	virtual bool attachConnectionInternal(
			ConnectionHandle hconn,
			std::unique_ptr<Connection> addr,
			Socket&& socket,
			std::error_code& err
			) noexcept = 0;
	virtual bool removeConnectionInternal(
			const ConnectionHandle& hconn,
            bool wait_for_end_connections,
            uint16_t timeout_sec,
			std::error_code& err) noexcept = 0;
	virtual bool modifyConnectionInternal(
			const ConnectionHandle& hconn,
            std::span<std::shared_ptr<Socket::BaseOption>> options,
			std::error_code& err) noexcept = 0;
    virtual bool addConnectionProcessInternal(
			const ConnectionHandle& hconn,
            std::unique_ptr<AbstractConnectionProcess> proc,
			std::error_code& err) noexcept = 0;
    virtual bool removeConnectionProcessInternal(
			const ConnectionHandle& hconn,
            bool wait_for_end_connections,
            uint16_t timeout_sec,
			std::error_code& err) noexcept = 0;
	std::unique_ptr<ConnectionIO> make_connectionIO(
				std::shared_ptr<Socket> sock_ptr,
				ConnectionHandle hconn,
				const Event& events,
				std::error_code& err) noexcept;
	std::jthread& thread() const noexcept;
	std::mutex& mutex() const noexcept;
	void wake_event() noexcept;
	std::span<network::EventHandle> wait(std::error_code& err,
        	int32_t timeout) noexcept;
	const std::shared_ptr<Socket> socket_by_id(ConnectionId id) const noexcept;
	const ConnectionState* connection_state_by_id(ConnectionId id) const noexcept;
	std::shared_ptr<Socket> socket_by_id(ConnectionId id) noexcept;
	ConnectionState* connection_state_by_id(ConnectionId id) noexcept;
	const std::unordered_map<ConnectionId,
        ConnectionState>& connections() const noexcept;
	std::unordered_map<ConnectionId,
        ConnectionState>& connections() noexcept;
	std::shared_ptr<BaseCommand> extract_command() noexcept;
	static void set_connection_state(Connection* conn,Connection::State state) noexcept;
	ConnectionHandle connection_handle(ConnectionId id) noexcept;
	bool remove_tracking_event(FileDescriptor fd,std::error_code& err) noexcept;
	bool add_tracking_event(
				FileDescriptor fd,
				EventHandle ev,
				std::error_code& err) noexcept;
	bool modify_tracking_event(
				FileDescriptor fd,
				EventHandle ev,
				std::error_code& err) noexcept;
	virtual void handle_pending(std::error_code& err) noexcept = 0;
    bool handle_events(
				ConnectionHandle hconn,
				ConnectionState& state,
				Event events,
				std::error_code& err) noexcept;
	void notify_ready_command(std::shared_ptr<BaseCommand> cmd) noexcept;
	bool stop_requested() noexcept{
		return stop_.stop_requested();
	}
	bool stop_possible() noexcept{
		return stop_.stop_possible();
	}
	void handle_worker_commands() noexcept{
		WorkerCommand cmd;
		{
			std::lock_guard lk(mutex());
			if(w_cmds_.empty())
				return;
			cmd = w_cmds_.front();
			w_cmds_.pop();
		}
		if(cmd==WorkerCommand::Stop)
			stop(false,0);
	}
private:
	mutable std::jthread thread_;
	mutable std::mutex m_;
    std::unordered_map<ConnectionId,
        ConnectionState> connections_;
	std::queue<std::shared_ptr<BaseCommand>> cmds_;
	std::queue<WorkerCommand> w_cmds_;
	Multiplexor multiplexor_;
	std::stop_token stop_;
};
}