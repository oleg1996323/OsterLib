#include "abstractclient.h"
#include "abstractserver.h"
#include <gtest/gtest.h>
#include "send.h"
#include "receive.h"
#include "abstractprocess.h"

using namespace network;

std::mutex m_;

class ClientPingProcess:public AbstractRequestableConnectionProcess{
    public:
    static std::atomic<int> count_sent;
    static std::atomic<int> count_recv;
    virtual void on_read(std::error_code& err) noexcept override{
            //std::cout<<"Client: receive ping"<<std::endl;
            try_receive(err);
            if(err)
            {   
                switch(static_cast<std::errc>(err.value())){
                    case std::errc::resource_unavailable_try_again:
                    case std::errc::operation_in_progress:
                    case std::errc::no_buffer_space:
                        //std::cout<<"(client) on read error: "<<err.message()<<std::endl;
                        err.clear();
                        return;
                    default:
                        complete_current_request(err);
                        err.clear();
                        make_active_request();
                        return;
                }
            }
            else{
                if(!io_context().has_to_read()){
                    count_recv.fetch_add(1,std::memory_order::relaxed);
                }
                complete_current_request(err);
                err.clear();
            }
    }
    virtual void on_write(std::error_code& err) noexcept override{
        if(count_sent.load(std::memory_order::relaxed)<6){
            //std::cout<<"Client: send ping"<<std::endl;
            bool all_sent = try_send(err);
            if(err) return;
            // if(err!=std::error_code())
            //     //std::cout<<err.message()<<std::endl;
            // else {
                if(all_sent)
                    count_sent.fetch_add(1,std::memory_order::relaxed);
                err.clear();
            // }
        }
    }
    virtual void on_task_done(std::error_code& err) noexcept override{
    }
    virtual void on_stop_requested(std::error_code& err) noexcept override{
        reset_requests(err);
        //std::cout<<"Client: stop requests"<<std::endl;
        //if(err!=std::error_code())
            //std::cout<<err.message()<<std::endl;
    }

    ClientPingProcess(ConnectionHandle hconn,std::error_code& err):
        AbstractRequestableConnectionProcess(hconn,err){}
    ~ClientPingProcess() = default;
    virtual void handle_event(
                Event event,
                std::error_code& err) noexcept
    {
        if(event&Event::In) on_read(err);
        //if(event&Event::Out)on_write(err);
        
    }
};

std::atomic<int> ClientPingProcess::count_sent = 0;
std::atomic<int> ClientPingProcess::count_recv = 0;

// должен быть известен фрейм, который десериализуется
class ServerPingProcess:public AbstractConnectionProcess{
    public:
    static std::atomic<int> count_sent;
    static std::atomic<int> count_recv;
    virtual void on_read(std::error_code& err) noexcept override{
        SizeFramedData<size_t> ping;
        io_context().receive(err,ping.start_,ping.data_);
        if(auto err_val = static_cast<std::errc>(err.value());
            err_val!=std::errc::operation_in_progress &&
            err!=std::error_code() &&
            err_val!=std::errc::resource_unavailable_try_again)
        {
            // if (err == std::errc::connection_reset)
            //     //std::cout << "(server) connection closed by peer" << std::endl;
            return;
        }
        if(ping.data_!=1 || ping.start_!=8){
            err = std::make_error_code(std::errc::bad_message);
            // //std::cout<<"(server) Not 1 for ping"<<std::endl;
            // //std::cout<<"start="<<ping.start_<<";data="<<ping.data_<<std::endl;
        }
        else{
            //std::cout<<"(server) Ping received"<<std::endl;
            if(!io_context().has_to_read()){
                count_recv.fetch_add(1,std::memory_order::relaxed);
                on_write(err);
            }
        }
        return;
    }
    virtual void on_write(std::error_code& err) noexcept override{
        SizeFramedData<size_t> ping;
        ping.start_=1;ping.data_=1;
        io_context().send(err,ping.start_,ping.data_);
        if(err!=std::error_code() &&
            static_cast<std::errc>(err.value())!=
            std::errc::operation_in_progress)
        {
            //std::cout<<err.message()<<std::endl;
            //std::cout<<"(server) Error at sending"<<std::endl;
            io_context().clear_send_buffer();
            return;
        }
        else{
            //std::cout<<"(server) Ping sent"<<std::endl;
            count_sent.fetch_add(1,std::memory_order::relaxed);
        }
    }
    virtual void on_task_done(std::error_code& err) noexcept override{
    }
    virtual void on_stop_requested(std::error_code& err) noexcept override{
    }

    ServerPingProcess(ConnectionHandle hconn,std::error_code& err):
        AbstractConnectionProcess(hconn,err){}
    ~ServerPingProcess() = default;
    virtual void handle_event(
                Event event,
                std::error_code& err) noexcept
    {
        if(event&Event::In) on_read(err);
        if(event&Event::Out)on_write(err);
    }
};

std::atomic<int> ServerPingProcess::count_sent = 0;
std::atomic<int> ServerPingProcess::count_recv = 0;

class Server:public AbstractServer{
    FRIEND_TEST(Client_server,ping);
};

class Client:public AbstractClient{
    public:
    Client(std::error_code& err,uint16_t ev_order):
        AbstractClient(err,ev_order){}
};

// void broken_pipe(int sig){
//     //std::cout<<"pipe broken"<<std::endl;
// }

TEST(Client_server,ping){
    server::Settings settings;
    settings.host_ = "127.0.0.1";
    settings.port_ = 32396;
    settings.protocol_ = Protocol::TCP;
    settings.num_threads_pool_ = 1;
    settings.timeout_seconds_processes_ = 3;
    settings.options_=ConnectionOptions{
        .reuse_address_{true,{}},
        .reuse_port_={true,{}},
        .keep_alive_={true,{}}};
    //for(int i = 0;i<5;++i){
        Server server;
        std::error_code err;
        server.configure(settings,
                            err);
        server.launch(err);
        server.set_processes_at_connections<ServerPingProcess>();
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
        Client client(err,10);
        auto hconn = client.connect(
                settings.host_,
                settings.port_,
                Socket::Type::Stream,
                Protocol::TCP,
                client::Settings(),
                err);
        ASSERT_EQ(err,std::error_code());
        {
            std::unique_ptr<ClientPingProcess> proc = std::make_unique<ClientPingProcess>(hconn,err);
            hconn.add_process(std::move(proc),err);
            for(int i=0;i<5;++i){
                auto cmd = client.request<size_t>(hconn,
                        serialization::serial_size(size_t(1)),
                        size_t(1),std::monostate());
                cmd->wait_ready();
                //std::cout<<"command "<<i<<" error: "<<cmd->error()->message()<<std::endl;
            }
        }
        std::lock_guard lk(m_);
        EXPECT_EQ(ServerPingProcess::count_recv.load(std::memory_order::relaxed),5);
        EXPECT_EQ(ServerPingProcess::count_sent.load(std::memory_order::relaxed),5);
        EXPECT_EQ(ClientPingProcess::count_recv.load(std::memory_order::relaxed),5);
        EXPECT_EQ(ClientPingProcess::count_sent.load(std::memory_order::relaxed),5);
        ServerPingProcess::count_recv.store(0,std::memory_order::relaxed);
        ServerPingProcess::count_sent.store(0,std::memory_order::relaxed);
        ClientPingProcess::count_recv.store(0,std::memory_order::relaxed);
        ClientPingProcess::count_sent.store(0,std::memory_order::relaxed);
    //}
}

TEST(Client_server,BufferOverflowExchange){
    
}

int main(int argc, char* argv[]){
    signal(SIGPIPE,SIG_IGN);
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}