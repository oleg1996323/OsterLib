#include "connectionhandle.h"
#include "abstractworker.h"
#include "commonsocket.h"
#include "abstractprocess.h"

namespace network{
std::atomic<ConnectionId> ConnectionHandle::id_counter{1};

bool ConnectionHandle::shutdown(std::error_code& err) noexcept{
    if(owner_)
        return owner_->shutdown_connection(err,*this);
    else{
        err = std::make_error_code(std::errc::no_such_device);
        return false;
    }
}
bool ConnectionHandle::close(std::error_code& err) noexcept{
    if(owner_)
        return owner_->close_connection(err,*this);
    else{
        err = std::make_error_code(std::errc::no_such_device);
        return false;
    }
}
bool ConnectionHandle::execute_command(
            std::shared_ptr<BaseCommand> cmd,
            std::error_code& err) const noexcept
{
    if(is_valid_handler()){
        owner()->push_command(std::move(cmd));
        return true;
    }
    else{
        err = std::make_error_code(std::errc::no_such_device);
        return false;
    }
}
bool ConnectionHandle::execute_commands(std::vector<std::shared_ptr<BaseCommand>>&& cmds,
            std::error_code& err) const noexcept{
    if(is_valid_handler()){
        owner()->push_commands(std::move(cmds));
        return true;
    }
    else{
        err = std::make_error_code(std::errc::no_such_device);
        return false;
    }
}
ConnectionHandle make_connection_handle(AbstractWorker* owner) noexcept{
    if(owner)
        return ConnectionHandle(owner);
    else
        return ConnectionHandle(nullptr,
            0);
}
bool ConnectionHandle::add_process(
        std::unique_ptr<AbstractRequestableConnectionProcess> proc,
        std::error_code& err) noexcept
{
    if(is_valid_handler()){
        owner()->push_command(std::make_shared<
                Command<CommandType::AttachProcess>>(
                *this,std::move(proc)));
        return true;
    }
    else{
        err = std::make_error_code(std::errc::no_such_device);
        return false;
    }
}
}