#include "worker/command.h"
#include "abstractworker.h"

namespace network{

	bool BaseCommand::wait_ready(Timeout timeout_sec) const noexcept{
		if(timeout_sec>0)
			return err_.wait_for(std::chrono::seconds(timeout_sec))
				==std::future_status::ready;
		else return err_.wait_for(std::chrono::seconds())
				==std::future_status::ready;
	}

	Command<CommandType::AddConnection>::Command(ConnectionHandle hconn,
				std::string host,
				Port port,
				Socket::Type type,
				Protocol proto,
				const client::Settings& settings):
				hconn_(hconn),
				settings_(settings),
				host_(host),
				port_(port),
				type_(type),
				proto_(proto){}

	Command<CommandType::AddConnection>::Command(ConnectionHandle hconn,
				std::string host,
				Port port,
				Socket::Type type,
				Protocol proto,
				client::Settings&& settings):
				hconn_(hconn),
				settings_(std::move(settings)),
				host_(host),
				port_(port),
				type_(type),
				proto_(proto){}

	Command<CommandType::AttachConnection>::Command(ConnectionHandle hconn,
			std::unique_ptr<Connection>&& conn,
			const server::Settings& settings,
			Socket&& sock):
			hconn_(hconn),
			conn_(std::move(conn)),
			settings_(settings),
			socket_(std::move(sock)){}

	Command<CommandType::AttachConnection>::Command(ConnectionHandle hconn,
			std::unique_ptr<Connection>&& conn,
			server::Settings&& settings,
			Socket&& sock):
			hconn_(hconn),
			conn_(std::move(conn)),
			settings_(std::move(settings)),
			socket_(std::move(sock)){}

	Command<CommandType::RemoveConnection>::Command(ConnectionHandle hconn,
			Timeout timeout_sec):
			hconn_(hconn),
			timeout_sec_(timeout_sec){}

	Command<CommandType::ModifyConnection>::Command(ConnectionHandle hconn,
			std::vector<std::shared_ptr<Socket::BaseOption>>&& options):
			hconn_(hconn),
			options_(std::move(options)){}

	Command<CommandType::AttachProcess>::Command(ConnectionHandle hconn,
			std::unique_ptr<AbstractConnectionProcess> proc):
			hconn_(hconn),
			proc_(std::move(proc)){}

	Command<CommandType::RequestStop>::Command(ConnectionHandle hconn,
			Timeout timeout_sec):
			hconn_(hconn),
			timeout_sec_(timeout_sec){}

	Command<CommandType::ShutDownConnection>::Command(ConnectionHandle hconn):
			hconn_(hconn){}
	Command<CommandType::RequestData>::Command(ConnectionHandle hconn):
			hconn_(hconn){}





	void Command<CommandType::AddConnection>::execute_internal(AbstractWorker* w) noexcept
	{
		std::error_code err;
		if(!hconn_.is_valid_handler() || 
			!hconn_.same_owner(w)){
			err = std::make_error_code(std::errc::invalid_argument);
			//r1std::cout<<err.message()<<std::endl;
			set_error(err);
			return;
		}
		Address addr = make_address(host_,port_,err);
		if(err==std::error_code()){
			Socket sock = ::network::socket(addr,type_,proto_,err);
			if(!err){
				auto conn = make_connection(
						w,
						host_,
						port_,
						type_,
						proto_,
						err);
				if(conn && !err)
				{
					w->connectInternal(
						hconn_,
						std::move(conn),
						settings_,
						std::move(sock),
						err);
					//if(err)
						//r1std::cout<<"Command AddConnection: "<<err.message()<<std::endl;
					set_error(err);
				}
				return;
			}
			else{
				//r1std::cout<<"Command AddConnection: "<<err.message()<<std::endl;
				set_error(err);
				return;
			}
		}
		else{
			//r1std::cout<<"Command AddConnection: "<<err.message()<<std::endl;
			set_error(err);
		}
	}

	void Command<
			CommandType::AttachConnection>::execute_internal(AbstractWorker* w) noexcept{
		std::error_code err;
		w->attachConnectionInternal(
			hconn_,
			std::move(conn_),
			settings_,
			std::move(socket_),
			err);
		set_error(err);
		////r1std::cout<<"Attach Connection command done"<<std::endl;
		return;
	}

	void Command<
			CommandType::RemoveConnection>::execute_internal(AbstractWorker* w) noexcept{
		std::error_code err;
		w->removeConnectionInternal(hconn_,timeout_sec_,err);
		set_error(err);
		////r1std::cout<<"Remove Connection command done"<<std::endl;
		return;
	}

	void Command<
			CommandType::ModifyConnection>::execute_internal(AbstractWorker* w) noexcept{
		std::error_code err;
		w->modifyConnectionInternal(hconn_,options_,err);
		set_error(err);
		////r1std::cout<<"Modify Connection command done"<<std::endl;
		return;
	}

	void Command<
			CommandType::AttachProcess>::execute_internal(AbstractWorker* w) noexcept{
		std::error_code err;
		w->addConnectionProcessInternal(hconn_,std::move(proc_),err);
		set_error(err);
		////r1std::cout<<"Attach Process command done"<<std::endl;
		return;
	}

	void Command<
			CommandType::RemoveProcess>::execute_internal(AbstractWorker* w) noexcept{
		std::error_code err;
		w->removeConnectionProcessInternal(hconn_,timeout_sec_,err);
		set_error(err);
		////r1std::cout<<"Remove Process command done"<<std::endl;
		return;
	}
	
	void Command<
			CommandType::RequestStop>::execute_internal(AbstractWorker* w) noexcept{
		std::error_code err;
		w->stop_process(hconn_,timeout_sec_,err);
		set_error(err);
		//r1std::cout<<"Request Stop command done"<<std::endl;
		return;
	}

	void Command<
			CommandType::ShutDownConnection>::execute_internal(AbstractWorker* w) noexcept{
		std::error_code err;
		w->shutdown_connection(err,hconn_);
		set_error(err);
		//r1std::cout<<"Shutdown command done"<<std::endl;
		return;
	}

	std::error_code Command<
			CommandType::RequestData>::emplace_request_to_process(
		ConnectionHandle hconn,
		AbstractWorker* w
	) noexcept
	{
		if(auto found = w->connections().find(hconn_.id());
			found==w->connections().end())
			return std::make_error_code(std::errc::no_such_device);
		else{
			
			if(!found->second.proc_)
				return std::make_error_code(std::errc::no_such_process);
			else{
				if(found->second.proc_->requestable()){
					std::error_code err;
					reinterpret_cast<AbstractRequestableConnectionProcess*>(
						found->second.proc_.get())->push_request(
							std::dynamic_pointer_cast<std::decay_t<decltype(*this)>>(shared_from_this()),
							err);
					return err;
				}
				else return std::make_error_code(std::errc::operation_not_supported);
			}
		}
	}
	Command<CommandType::TaskDone>::Command(ConnectionHandle hconn):
		hconn_(hconn){}
	void Command<CommandType::TaskDone>::execute_internal(
		AbstractWorker* w) noexcept
	{
		std::error_code err;
		if(auto conn_stat = w->connection_state_by_id(hconn_.id());
			conn_stat!=nullptr &&
			conn_stat->proc_!=nullptr &&
			conn_stat->proc_->has_task()){
			if(conn_stat->proc_->is_ready(err) && !err){
				conn_stat->proc_->on_task_done(err);
				set_error(err);
			}
			else set_error(std::make_error_code(std::errc::operation_in_progress));			
		}
		else set_error(std::make_error_code(std::errc::no_such_process));
	}
}