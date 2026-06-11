#pragma once
#include "multiplexor.h"
#include "abstractprocess.h"
#include "connection.h"
#include <queue>
#include <span>
#include "worker/command.h"
#include "connectionIO.h"
#include "serversettings.h"
#include "clientsettings.h"

namespace network{
class AbstractProcess;
class AbstractWorker{
	void __thread_launch__(std::stop_token st) noexcept;

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
		std::unique_ptr<Socket> socket_;
		Event events_handled_;
	};
    AbstractWorker(uint32_t order_lenght,std::error_code& err);
	virtual ~AbstractWorker() = default;
	void start();
	void push_command(std::shared_ptr<BaseCommand> cmd) noexcept;
	void push_commands(std::vector<std::shared_ptr<BaseCommand>>&& cmds) noexcept;
	bool enable_writing(
			ConnectionHandle hconn,
			bool enable,
			std::error_code& err) noexcept;
	bool enable_readable(
			ConnectionHandle hconn,
			bool enable,
			std::error_code& err) noexcept;

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
	virtual void run(EventHandle ev,std::stop_token st,std::error_code& err) = 0;
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
            uint16_t timeout_sec);
	virtual bool connectInternal(
			const ConnectionHandle& hconn,
			std::unique_ptr<Connection> conn,
			const client::Settings& settings,
            Socket&& socket,std::error_code& err) noexcept = 0;
	virtual bool attachConnectionInternal(
			ConnectionHandle hconn,
			std::unique_ptr<Connection> addr,
			const server::Settings& settings,
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
				Socket& sock_ptr,
				ConnectionHandle hconn,
				size_t recv_buf_sz,
				const Event& events,
				std::error_code& err) noexcept;
	std::jthread& thread() const noexcept;
	std::mutex& mutex() const noexcept;
	void wake_event() noexcept;
	std::span<network::EventHandle> wait(std::error_code& err,
        	int32_t timeout) noexcept;
	const Socket* socket_by_id(ConnectionId id) const noexcept;
	const ConnectionState* connection_state_by_id(ConnectionId id) const noexcept;
	Socket* socket_by_id(ConnectionId id) noexcept;
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
    std::unordered_map<ConnectionId,
        ConnectionState> connections_;
	std::queue<std::shared_ptr<BaseCommand>> cmds_;
	std::queue<WorkerCommand> w_cmds_;
	Multiplexor multiplexor_;
	mutable std::mutex m_;
	mutable std::jthread thread_;
};
}