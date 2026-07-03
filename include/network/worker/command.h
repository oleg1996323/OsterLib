#pragma once
#include <system_error>
#include "address.h"
#include <string>
#include <cstdint>
#include "definitions/protocol.h"
#include "definitions/family.h"
#include "definitions.h"
#include "connectionhandle.h"
#include "commonsocket.h"
#include <expected>
#include "command_types.h"
#include "clientsettings.h"
#include "serversettings.h"
#include <future>

namespace network
{	class Connection;
	class AbstractWorker;
	class AbstractConnectionProcess;
	class BaseCommand:public std::enable_shared_from_this<BaseCommand>
	{	
		private:
		std::promise<std::error_code> prom_;
		std::shared_future<std::error_code> err_=prom_.get_future().share();
		friend class AbstractWorker;
		std::string attribute_;
		public:
		void set_attribute(const std::string& attribute) noexcept{
			attribute_=attribute;
		}
		const std::string& attribute() const noexcept{
			return attribute_;
		}
		virtual ~BaseCommand() = default;
		virtual void execute(AbstractWorker* worker){
			execute_internal(worker);
		}
		bool ready() const noexcept{
			return wait_ready(0);
		}
		bool wait_ready(Timeout timeout_sec) const noexcept;
		std::optional<bool> successed() const noexcept{
			if(ready()){
				if(err_.get())
					return false;
				else return true;
			}
			else return std::nullopt;
		}
		std::optional<std::error_code> error() const noexcept{
			if(ready())
				return err_.get();
			else return std::nullopt;
		}
		protected:
		virtual void execute_internal(
				AbstractWorker* worker) noexcept = 0;
		void set_error(std::error_code err) noexcept{
			prom_.set_value(err);
		}
	};

	template <CommandType T, typename... ARGS>
	struct Command;

	template <>
	struct Command<CommandType::AddConnection> :public BaseCommand
	{
		ConnectionHandle hconn_;
		const client::Settings& settings_;
		std::string host_;
		Port port_;
		Socket::Type type_;
		Protocol proto_;
		Command(ConnectionHandle hconn,
				std::string host,
				Port port,
				Socket::Type type,
				Protocol proto,
				const client::Settings& settings);
		Command(ConnectionHandle hconn,
				std::string host,
				Port port,
				Socket::Type type,
				Protocol proto,
				client::Settings&& settings);
		virtual void execute_internal(
				AbstractWorker* w) noexcept override;
	};

	template <>
	struct Command<CommandType::AttachConnection> :public BaseCommand
	{
		ConnectionHandle hconn_;
		const network::server::Settings& settings_;
		std::unique_ptr<Connection> conn_;
		Socket socket_;
		Command(ConnectionHandle hconn,
				std::unique_ptr<Connection>&& conn,
				const server::Settings& settings,
				Socket&& sock);
		Command(ConnectionHandle hconn,
				std::unique_ptr<Connection>&& conn,
				server::Settings&& settings,
				Socket&& sock);
		virtual void execute_internal(
				AbstractWorker* w) noexcept override;
	};

	template<>
	struct Command<CommandType::RemoveConnection> :public BaseCommand
	{
		ConnectionHandle hconn_;
		Timeout timeout_sec_{0};
		Command(ConnectionHandle hconn,
				Timeout timeout_sec=0);
		virtual ~Command() = default;
		virtual void execute_internal(
				AbstractWorker* w) noexcept override;
	};

	template<>
	struct Command<CommandType::ModifyConnection> :public BaseCommand
	{
		ConnectionHandle hconn_;
		std::vector<std::shared_ptr<Socket::BaseOption>> options_;
		Command(ConnectionHandle hconn,
				std::vector<std::shared_ptr<Socket::BaseOption>>&& options);
		virtual ~Command() = default;
		virtual void execute_internal(
				AbstractWorker* w) noexcept override;
	};

	template<>
	struct Command<CommandType::AttachProcess> :public BaseCommand
	{
		ConnectionHandle hconn_;
		std::unique_ptr<AbstractConnectionProcess> proc_;
		Command(ConnectionHandle hconn,
				std::unique_ptr<AbstractConnectionProcess> proc);
		virtual ~Command() = default;
		virtual void execute_internal(
				AbstractWorker* w) noexcept override;
	};

	template<>
	struct Command<CommandType::RemoveProcess> :public BaseCommand
	{
		ConnectionHandle hconn_;
		Timeout timeout_sec_{0};
		Command(ConnectionHandle hconn,
				Timeout timeout_sec=0);
		virtual ~Command() = default;
		virtual void execute_internal(
				AbstractWorker* w) noexcept override;
	};

	template<>
	struct Command<CommandType::RequestStop> :public BaseCommand
	{
		ConnectionHandle hconn_;
		Timeout timeout_sec_{0};
		Command(ConnectionHandle hconn,
				Timeout timeout_sec=0);
		virtual ~Command() = default;
		virtual void execute_internal(
				AbstractWorker* w) noexcept override;
	};

	/**
	 *
	 */
	template<>
	struct Command<CommandType::ShutDownConnection> :public BaseCommand
	{
		ConnectionHandle hconn_;
		Command(ConnectionHandle hconn);
		virtual ~Command() = default;
		virtual void execute_internal(AbstractWorker* w) noexcept override;
	};
}
#include "frame/frames.h"
#include "abstractprocess.h"
namespace network{
	class AbstractRequestableConnectionProcess;
	template<>
	struct Command<CommandType::RequestData>:public BaseCommand
	{
		friend class AbstractRequestableConnectionProcess;
		protected:
		std::error_code emplace_request_to_process(
			ConnectionHandle hconn,
			AbstractWorker* w
			) noexcept;
		std::shared_ptr<AbstractFrame> to_send_;
		std::shared_ptr<AbstractFrame> to_receive_;
		size_t bytes_sent_ = 0;
		size_t bytes_recv_ = 0;
		public:
		ConnectionHandle hconn_;
		Command(ConnectionHandle hconn);
		virtual ~Command() = default;
		template<typename START_t,typename DATA_t, typename END_t>
        std::shared_ptr<Frame<START_t,DATA_t,END_t>> received(Frame<START_t,DATA_t,END_t>* in) noexcept{
			if (auto dyn_cast = std::dynamic_pointer_cast<Frame<START_t,DATA_t,END_t>>(to_receive_))
				return dyn_cast;
			else return {};
		}
		size_t reset_sent(bool reset) noexcept{
			if(reset){
				auto result = bytes_sent_;
				return result;
			}
			else return bytes_sent_;
		}
		size_t reset_received(bool reset) noexcept{
			if(reset){
				auto result = bytes_recv_;
				return result;
			}
			else return bytes_recv_;
		}
		size_t bytes_sent(size_t sent) noexcept{
			bytes_sent_+=sent;
			return bytes_sent_;
		}
		size_t bytes_received(size_t received) noexcept{
			bytes_recv_+=received;
			return bytes_recv_;
		}
		bool all_sent(bool reset) noexcept{
			if(bytes_sent_>=serialization::serial_size(*to_send_)){
				if(reset)
					bytes_sent_=0;
				return true;
			}
			else return false;
		}
		bool all_received(bool reset) noexcept{
			if(bytes_recv_>=serialization::min_serial_size(*to_send_) &&
				bytes_recv_==serialization::serial_size(*to_send_))
			{
				if(reset)
					bytes_recv_ = 0;
				return true;
			}
			else return false;
		}
		template<typename START_t,typename DATA_t, typename END_t>
        std::shared_ptr<Frame<START_t,DATA_t,END_t>> sent(Frame<START_t,DATA_t,END_t>* in) noexcept{
			if (auto dyn_cast = std::dynamic_pointer_cast<Frame<START_t,DATA_t,END_t>>(to_send_))
				return dyn_cast;
			else return {};
		}
		std::shared_ptr<AbstractFrame> received() noexcept{
			return to_receive_;
		}
		std::shared_ptr<AbstractFrame> sent() noexcept{
			return to_send_;
		}
	};

	template<>
	struct Command<CommandType::TaskDone>:public BaseCommand
	{
		ConnectionHandle hconn_;
		Command(ConnectionHandle hconn);
		virtual void execute_internal(
				AbstractWorker* w) noexcept override;
	};

	template<typename RESULT_EXPECTED,
		typename START_FRAME,
		typename DATA_FRAME_SEND,
		typename END_FRAME>
	struct RequestCommandSpec:public Command<CommandType::RequestData>
	{
		friend class AbstractRequestableConnectionProcess;
		public:
		RequestCommandSpec(
			ConnectionHandle hconn,
			START_FRAME&& start,
			DATA_FRAME_SEND&& to_send,
			END_FRAME&& end):
				Command(hconn)
					{
						to_send_ = 
						std::make_shared<Frame<
							START_FRAME,
							DATA_FRAME_SEND,
							END_FRAME>>(
								std::forward<START_FRAME>(start),
								std::forward<DATA_FRAME_SEND>(to_send),
								std::forward<END_FRAME>(end));
						to_receive_ = std::make_shared<Frame<
								START_FRAME,
								RESULT_EXPECTED,
								END_FRAME>>();
					}
		virtual ~RequestCommandSpec() = default;
		std::shared_ptr<Frame<
				START_FRAME,
				RESULT_EXPECTED,
				END_FRAME>> received() noexcept{
			if(BaseCommand::ready())
				return std::dynamic_pointer_cast<Frame<
				START_FRAME,
				RESULT_EXPECTED,
				END_FRAME>>(to_receive_);
			else return {};
		}
		std::shared_ptr<Frame<
				START_FRAME,
				DATA_FRAME_SEND,
				END_FRAME>> sent() noexcept{
			if(BaseCommand::ready())
				return std::dynamic_pointer_cast<Frame<
				START_FRAME,
				DATA_FRAME_SEND,
				END_FRAME>>(to_send_);
			else return {};
		}
		virtual void execute_internal(
				AbstractWorker* w) noexcept override
		{
			if(auto err = emplace_request_to_process(
				hconn_,
				w);
				err)
			{
				std::cout<<"Command request: "<<err.message()<<std::endl;
				set_error(err);
			}
		}
		template<typename START_t,typename DATA_t, typename END_t>
        std::shared_ptr<Frame<START_t,DATA_t,END_t>> received(Frame<START_t,DATA_t,END_t>* in) noexcept{
			if constexpr(
				std::is_same_v<START_t,START_FRAME> &&
				std::is_same_v<DATA_t,DATA_FRAME_SEND> && 
				std::is_same_v<END_t,END_FRAME>)
				return Command::received(in);
			else return {};
		}
		template<typename START_t,typename DATA_t, typename END_t>
        std::shared_ptr<Frame<START_t,DATA_t,END_t>> sent(Frame<START_t,DATA_t,END_t>* out) noexcept{
			if constexpr(
				std::is_same_v<START_t,START_FRAME> &&
				std::is_same_v<DATA_t,DATA_FRAME_SEND> && 
				std::is_same_v<END_t,END_FRAME>)
				return Command::received(out);
			else return {};
		}
	};
}