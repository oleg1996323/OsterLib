#pragma once
#include <cstdint>
#include <system_error>
#include <atomic>
#include <memory>
#include <vector>
#include "OsterLib/network/commonsocket.h"

namespace network{
class AbstractWorker;
class AbstractRequestableConnectionProcess;
class BaseCommand;

using ConnectionId = uint32_t;

class ConnectionHandle{
    static std::atomic<uint32_t> id_counter;
    ConnectionId conn_id_{0};
    AbstractWorker* owner_{nullptr};
    friend class AbstractWorker;
    friend ConnectionHandle make_connection_handle(AbstractWorker*) noexcept;
    friend class ConnectionIO;
    ConnectionHandle(AbstractWorker* owner,ConnectionId conn_id):
    conn_id_(conn_id),owner_(owner){}
    AbstractWorker* owner() noexcept{
        return owner_;
    }
    public:
    const AbstractWorker* owner() const noexcept{
        return owner_;
    }
    ConnectionHandle(AbstractWorker* owner):
    conn_id_(id_counter.fetch_add(1, std::memory_order_relaxed)),
        owner_(owner)
    {
        conn_id_=(conn_id_==0?1:conn_id_);
    }
    ConnectionHandle(const ConnectionHandle& other):
    conn_id_(other.conn_id_),owner_(other.owner_){}
    ConnectionHandle(ConnectionHandle&& other) noexcept:
        conn_id_(std::exchange(other.conn_id_, 0)),
        owner_(std::exchange(other.owner_, nullptr)){}
    ConnectionHandle& operator=(const ConnectionHandle& other){
        if(this!=&other){
            conn_id_ = other.conn_id_;
            owner_ = other.owner_;
        }
        return *this;
    }
    ConnectionHandle& operator=(ConnectionHandle&& other){
        if(this!=&other){
            conn_id_ = std::exchange(other.conn_id_,0);
            owner_ = std::exchange(other.owner_,nullptr);
        }
        return *this;
    }
    ConnectionId id() const noexcept{
        return conn_id_;
    }
    bool execute_command(std::shared_ptr<BaseCommand> cmd, std::error_code& err) noexcept;
	bool execute_commands(std::vector<std::shared_ptr<BaseCommand>>&& cmds,
                std::error_code& err) noexcept;
    bool is_valid_handler() const noexcept{
        return owner_ != nullptr && conn_id_ != 0;
    }
    bool shutdown(std::error_code& err) noexcept;
    bool close(std::error_code& err) noexcept;
    bool add_process(std::unique_ptr<AbstractRequestableConnectionProcess> proc,
                    std::error_code& err) noexcept;
    bool stop_process(std::error_code& err) noexcept;
    bool set_option(std::error_code& err,Socket::BaseOption& option) noexcept;
    bool set_options(std::error_code& err,
                    std::span<Socket::BaseOption> options) noexcept;
    bool same_owner(const AbstractWorker* owner) const noexcept{
        return owner==owner_;
    }
};

ConnectionHandle make_connection_handle(AbstractWorker* owner) noexcept;
}