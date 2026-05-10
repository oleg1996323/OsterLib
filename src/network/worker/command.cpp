#include "worker/command.h"
#include "abstractworker.h"

namespace network{

	Command<CommandType::AddConnection>::Command(ConnectionHandle hconn,
				std::string host,
				Port port,
				Socket::Type type,
				Protocol proto):
				hconn_(hconn),
				host_(host),
				port_(port),
				type_(type),
				proto_(proto){}

	Command<CommandType::AttachConnection>::Command(ConnectionHandle hconn,
			std::unique_ptr<Connection>&& conn,
			Socket&& sock):
			hconn_(hconn),
			conn_(std::move(conn)),
			socket_(std::move(sock)){}

	Command<CommandType::RemoveConnection>::Command(ConnectionHandle hconn,
			uint32_t timeout_sec,
			bool wait):
			hconn_(hconn),
			timeout_sec_(timeout_sec),
			wait_(wait){}

	Command<CommandType::ModifyConnection>::Command(ConnectionHandle hconn,
			std::vector<std::shared_ptr<Socket::BaseOption>>&& options):
			hconn_(hconn),
			options_(std::move(options)){}

	Command<CommandType::AttachProcess>::Command(ConnectionHandle hconn,
			std::unique_ptr<AbstractConnectionProcess> proc):
			hconn_(hconn),
			proc_(std::move(proc)){}

	Command<CommandType::RequestStop>::Command(ConnectionHandle hconn,
			uint32_t timeout_sec,
			bool wait):
			hconn_(hconn),
			timeout_sec_(timeout_sec),
			wait_(wait){}

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
			std::cout<<err.message()<<std::endl;
			set_error(err);
			set_ready();
		}
		Address addr = make_address(host_,port_,err);
		if(err==std::error_code()){
			Socket sock = ::network::socket(addr,type_,proto_,err);
			if(err==std::error_code()){
				auto conn = make_connection(
						w,
						host_,
						port_,
						type_,
						proto_,
						err);
				if(conn && err==std::error_code())
				{
					w->connectInternal(
						hconn_,
						std::move(conn),
						std::move(sock),
						err);
					if(err!=std::error_code())
						std::cout<<"Command AddConnection: "<<err.message()<<std::endl;
					set_error(err);
					set_ready();
				}
				return;
			}
			else{
				std::cout<<"Command AddConnection: "<<err.message()<<std::endl;
				set_error(err);
				set_ready();
			}
		}
		else{
			std::cout<<"Command AddConnection: "<<err.message()<<std::endl;
			set_error(err);
			set_ready();
		}
	}

	void Command<
			CommandType::AttachConnection>::execute_internal(AbstractWorker* w) noexcept{
		std::error_code err;
		w->attachConnectionInternal(
			hconn_,
			std::move(conn_),
			std::move(socket_),
			err);
		set_error(err);
		set_ready();
		//std::cout<<"Attach Connection command done"<<std::endl;
		return;
	}

	void Command<
			CommandType::RemoveConnection>::execute_internal(AbstractWorker* w) noexcept{
		std::error_code err;
		w->removeConnectionInternal(hconn_,wait_,timeout_sec_,err);
		set_error(err);
		set_ready();
		//std::cout<<"Remove Connection command done"<<std::endl;
		return;
	}

	void Command<
			CommandType::ModifyConnection>::execute_internal(AbstractWorker* w) noexcept{
		std::error_code err;
		w->modifyConnectionInternal(hconn_,options_,err);
		set_error(err);
		set_ready();
		//std::cout<<"Modify Connection command done"<<std::endl;
		return;
	}

	void Command<
			CommandType::AttachProcess>::execute_internal(AbstractWorker* w) noexcept{
		std::error_code err;
		w->addConnectionProcessInternal(hconn_,std::move(proc_),err);
		set_error(err);
		set_ready();
		//std::cout<<"Attach Process command done"<<std::endl;
		return;
	}

	void Command<
			CommandType::RemoveProcess>::execute_internal(AbstractWorker* w) noexcept{
		std::error_code err;
		w->removeConnectionProcessInternal(hconn_,wait_,timeout_sec_,err);
		set_error(err);
		set_ready();
		//std::cout<<"Remove Process command done"<<std::endl;
		return;
	}
	
	void Command<
			CommandType::RequestStop>::execute_internal(AbstractWorker* w) noexcept{
		std::error_code err;
		w->stop_process(hconn_,wait_,timeout_sec_,err);
		set_error(err);
		set_ready();
		std::cout<<"Request Stop command done"<<std::endl;
		return;
	}

	void Command<
			CommandType::ShutDownConnection>::execute_internal(AbstractWorker* w) noexcept{
		std::error_code err;
		w->shutdown_connection(err,hconn_);
		set_error(err);
		set_ready();
		std::cout<<"Shutdown command done"<<std::endl;
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
}