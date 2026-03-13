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

namespace network
{	class Connection;
	class AbstractWorker;
	class AbstractConnectionProcess;
	class BaseCommand:public std::enable_shared_from_this<BaseCommand>
	{
		friend class AbstractWorker;
		public:
		virtual ~BaseCommand() = default;
		virtual void execute(AbstractWorker* worker){
			execute_internal(worker);
		}
		bool ready() const noexcept{
			return ready_.load(
				std::memory_order::relaxed);
		}
		void wait_ready(){
			while (!ready_.load(std::memory_order_acquire)) {
				ready_.wait(false, std::memory_order_relaxed);
			}
			return;
		}
		std::optional<bool> successed() const noexcept{
			if(ready())
				return err_c_.load(std::memory_order::relaxed)==0;
			else return std::nullopt;
		}
		std::optional<std::error_code> error() const noexcept{
			if(ready())
				return std::make_error_code(
					static_cast<std::errc>(
					err_c_.load(std::memory_order::relaxed)));
			else return std::nullopt;
		}
		protected:
		virtual void execute_internal(
				AbstractWorker* worker) noexcept = 0;
		void set_error(std::error_code err) noexcept{
			err_c_.store(err.value(),std::memory_order::release);
		}
		void set_error(int err) noexcept{
			err_c_.store(err,std::memory_order::release);
		}
		void set_ready() noexcept{
			ready_.store(true,std::memory_order::release);
			ready_.notify_all();
		}
		private:
		std::atomic<bool> ready_{false};
		std::atomic<uint32_t> err_c_{0};
	};

	template <CommandType T, typename... ARGS>
	struct Command;

	template <>
	struct Command<CommandType::AddConnection> :public BaseCommand
	{
		ConnectionHandle hconn_;
		std::string host_;
		Port port_;
		Socket::Type type_;
		Protocol proto_;
		Command(ConnectionHandle hconn,
				std::string host,
				Port port,
				Socket::Type type,
				Protocol proto);
		virtual void execute_internal(
				AbstractWorker* w) noexcept override;
	};

	template <>
	struct Command<CommandType::AttachConnection> :public BaseCommand
	{
		ConnectionHandle hconn_;
		std::unique_ptr<Connection> conn_;
		std::unique_ptr<AbstractConnectionProcess> proc_;
		Socket socket_;
		Command(ConnectionHandle hconn,
				std::unique_ptr<Connection>&& conn,
				std::unique_ptr<AbstractConnectionProcess> proc,
				Socket&& sock);
		virtual void execute_internal(
				AbstractWorker* w) noexcept override;
	};

	template<>
	struct Command<CommandType::RemoveConnection> :public BaseCommand
	{
		ConnectionHandle hconn_;
		uint32_t timeout_sec_{0};
		bool wait_{false};
		Command(ConnectionHandle hconn,
				uint32_t timeout_sec=0,
				bool wait = false);
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
		uint32_t timeout_sec_{0};
		bool wait_{false};
		Command(ConnectionHandle hconn,
				uint32_t timeout_sec=0,
				bool wait = false);
		virtual ~Command() = default;
		virtual void execute_internal(
				AbstractWorker* w) noexcept override;
	};

	template<>
	struct Command<CommandType::RequestStop> :public BaseCommand
	{
		ConnectionHandle hconn_;
		uint32_t timeout_sec_{0};
		bool wait_{false};
		Command(ConnectionHandle hconn,
				uint32_t timeout_sec=0,
				bool wait = false);
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
		public:
		AbstractFrame* weak_to_send_;
		AbstractFrame* weak_to_receive_;
		ConnectionHandle hconn_;
		Command(ConnectionHandle hconn);
		virtual ~Command() = default;
	};

	template<typename RESULT_EXPECTED,
		typename START_FRAME,
		typename DATA_FRAME_SEND,
		typename END_FRAME>
	struct RequestCommandSpec:public Command<CommandType::RequestData>
	{
		std::shared_ptr<SenderFrame<
				START_FRAME,
				DATA_FRAME_SEND,
				END_FRAME>> to_send_;
		std::shared_ptr<ReceiverFrame<
				START_FRAME,
				RESULT_EXPECTED,
				END_FRAME>> to_receive_;
		friend class AbstractRequestableConnectionProcess;
		public:
		RequestCommandSpec(
			ConnectionHandle hconn,
			START_FRAME&& start,
			DATA_FRAME_SEND&& to_send,
			END_FRAME&& end):
				Command(hconn),
					to_send_(
					std::make_shared<SenderFrame<
						START_FRAME,
						DATA_FRAME_SEND,
						END_FRAME>>(
							std::forward<START_FRAME>(start),
							std::forward<DATA_FRAME_SEND>(to_send),
							std::forward<END_FRAME>(end))),
					to_receive_(
					std::make_shared<ReceiverFrame<
						START_FRAME,
						RESULT_EXPECTED,
						END_FRAME>>())
					{
						weak_to_send_ = to_send_.get();
						weak_to_receive_ = to_receive_.get();
					}
		virtual ~RequestCommandSpec() = default;
		std::shared_ptr<ReceiverFrame<
				START_FRAME,
				RESULT_EXPECTED,
				END_FRAME>> get_result_frame() noexcept{
			if(BaseCommand::ready())
				return to_receive_;
			else return {};
		}
		virtual void execute_internal(
				AbstractWorker* w) noexcept override
		{
			if(auto err = emplace_request_to_process(
				hconn_,
				w);
				err!=std::error_code())
			{
				std::cout<<"Command request: "<<err.message()<<std::endl;
				set_error(err);
				set_ready();
			}
		}
	};
}