#include "abstractclient.h"
#include "abstractserver.h"
#include <gtest/gtest.h>
#include "send.h"
#include "receive.h"
#include "abstractprocess.h"
#include <linux/tls.h>

using namespace network;

std::mutex m_;
std::atomic<int> client_ping_val = 1;
std::atomic<int> server_ping_val = 2;

class ClientPingProcess:public AbstractRequestableConnectionProcess{
    public:
    static std::atomic<int> count_sent;
    static std::atomic<int> count_recv;

    virtual void on_read(std::error_code& err) noexcept override{
        //std::cout<<"Client: receive ping"<<std::endl;
        using result_t = Frame<size_t,size_t,std::monostate>;
        result_t* result_ptr;
        auto& result = *active_request_->received(result_ptr);
        auto recv = io_context().receive(err,
                result.start_frame(),
                result.data_frame());
        if(err) handle_receive_error(err);
        else{
            active_request_->bytes_received(recv);
            if(active_request_->all_received(false)){
                std::cout<<"Client received: start="<<
                    result.start_frame()<<" data="<<result.data_frame()<<std::endl;
                std::cout<<"Client expect receive: start="<<8<<
                    " data="<<server_ping_val.load()<<std::endl;
                if(server_ping_val.load()==static_cast<int>(result.data_frame()))
                {
                    server_ping_val.fetch_add(2,std::memory_order::relaxed);
                    client_ping_val.fetch_add(2,std::memory_order::relaxed);
                    count_recv.fetch_add(1,std::memory_order::relaxed);
                    active_request_->reset_sent(true);
                }
                if(!active_request(err)){
                    if(err) handle_receive_error(err);
                    return;
                }
            }
            complete_current_request(err);
            err.clear();
        }
    }
    virtual void on_write(std::error_code& err) noexcept override{
        
        using send_t = Frame<size_t,size_t,std::monostate>;
        send_t* ping;
        auto sending_frame = this->active_request_->sent(ping);
        sending_frame->data_frame()=client_ping_val.load(std::memory_order::relaxed);
        std::cout<<"Client send: start="<<sending_frame->start_frame()<<
            " data="<<sending_frame->data_frame()<<std::endl;
        auto sent = io_context().send(err,*active_request_->sent());
        //if(err) return;
        if(err) handle_sending_error(err);
        else {
            active_request_->bytes_sent(sent);
            if(active_request_->all_sent(true));
                count_sent.fetch_add(1,std::memory_order::relaxed);
            err.clear();
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
        //std::cout<<"Server: receive ping"<<std::endl;
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
        //std::cout<<"(server) Ping received"<<std::endl;
        std::cout<<"Server received: start="<<
                ping.start_<<" data="<<ping.data_<<std::endl;
            std::cout<<"Server expect receive: start="<<8<<
                " data="<<client_ping_val.load()<<std::endl;
        if(!io_context().has_to_read() && 
            client_ping_val.load()==static_cast<int>(ping.data_) && 
            ping.start_==8){
            count_recv.fetch_add(1,std::memory_order::relaxed);
            on_write(err);
        }
        else{
            on_write(err);
        }
        return;
    }
    virtual void on_write(std::error_code& err) noexcept override{
        SizeFramedData<size_t> ping;
        ping.start_=serialization::serial_size(ping.data_);
            ping.data_=server_ping_val.load();
        std::cout<<"Server send: start="<<ping.start_<<" data="<<server_ping_val.load()<<std::endl;
        io_context().send(err,ping.start_,static_cast<size_t>(server_ping_val.load()));
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
    for(int i = 0;i<100;++i){
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
            //std::this_thread::sleep_for(std::chrono::milliseconds(5));
            for(int i=0;i<5;++i){
                std::cout<<"Client send ping="<<client_ping_val.load()<<std::endl;
                auto cmd = client.request<size_t>(hconn,
                        serialization::serial_size(size_t(1)),
                        static_cast<size_t>(client_ping_val.load()),std::monostate());
                cmd->wait_ready(10);
                //std::cout<<"command "<<i<<" error: "<<cmd->error()->message()<<std::endl;
            }
        }
        EXPECT_EQ(ServerPingProcess::count_recv.load(std::memory_order::relaxed),5);
        EXPECT_EQ(ServerPingProcess::count_sent.load(std::memory_order::relaxed),5);
        EXPECT_EQ(ClientPingProcess::count_recv.load(std::memory_order::relaxed),5);
        EXPECT_EQ(ClientPingProcess::count_sent.load(std::memory_order::relaxed),5);
        ServerPingProcess::count_recv.store(0,std::memory_order::relaxed);
        ServerPingProcess::count_sent.store(0,std::memory_order::relaxed);
        ClientPingProcess::count_recv.store(0,std::memory_order::relaxed);
        ClientPingProcess::count_sent.store(0,std::memory_order::relaxed);
    }
}

class ClientProcess:public AbstractRequestableConnectionProcess{
    public:
    virtual void on_read(std::error_code& err) noexcept override{
            //std::cout<<"Client: receive ping"<<std::endl;
            using result_t = Frame<std::monostate,std::string,std::monostate>;
            result_t* result;
            auto recv = io_context().receive(err,*active_request_->received(result));
            if(err) handle_receive_error(err);
            else{
                active_request_->bytes_received(recv);
                if(active_request_->all_received(true)){
                    complete_current_request(err);
                    err.clear();
                }
            }
    }
    virtual void on_write(std::error_code& err) noexcept override{
            using sending_t = Frame<std::monostate,std::string,std::monostate>;
            auto sent = io_context().send(err,*active_request_->sent());
            if(err) handle_sending_error(err);
            else {
                active_request_->bytes_sent(sent);
                if(active_request_->all_sent(true))
                    err.clear();
            }
            // if(err!=std::error_code())
            //     //std::cout<<err.message()<<std::endl;
            // else {
            
            // }
    }
    virtual void on_task_done(std::error_code& err) noexcept override{
    }
    virtual void on_stop_requested(std::error_code& err) noexcept override{
        reset_requests(err);
        //std::cout<<"Client: stop requests"<<std::endl;
        //if(err!=std::error_code())
            //std::cout<<err.message()<<std::endl;
    }

    ClientProcess(ConnectionHandle hconn,std::error_code& err):
        AbstractRequestableConnectionProcess(hconn,err){}
    ~ClientProcess() = default;
    virtual void handle_event(
                Event event,
                std::error_code& err) noexcept
    {
        if(event&Event::In) on_read(err);
        //if(event&Event::Out)on_write(err);
        
    }
};

class ServerProcess:public AbstractConnectionProcess{
    public:
    static std::string received;
    virtual void on_read(std::error_code& err) noexcept override{
        std::string msg;
        io_context().receive(err,msg);
        if(auto err_val = static_cast<std::errc>(err.value());
            err_val!=std::errc::operation_in_progress &&
            err!=std::error_code() &&
            err_val!=std::errc::resource_unavailable_try_again)
        {
            // if (err == std::errc::connection_reset)
            //     //std::cout << "(server) connection closed by peer" << std::endl;
            return;
        }
        //std::cout<<"(server) Ping received"<<std::endl;
        received=msg;
        on_write(err);
        return;
    }
    virtual void on_write(std::error_code& err) noexcept override{
        std::string msg = "Hello client";
        io_context().send(err,msg);
        if(err!=std::error_code() &&
            static_cast<std::errc>(err.value())!=
            std::errc::operation_in_progress)
        {
            //std::cout<<err.message()<<std::endl;
            //std::cout<<"(server) Error at sending"<<std::endl;
            io_context().clear_send_buffer();
            return;
        }
    }
    virtual void on_task_done(std::error_code& err) noexcept override{
    }
    virtual void on_stop_requested(std::error_code& err) noexcept override{
    }

    ServerProcess(ConnectionHandle hconn,std::error_code& err):
        AbstractConnectionProcess(hconn,err){}
    ~ServerProcess() = default;
    virtual void handle_event(
                Event event,
                std::error_code& err) noexcept
    {
        if(event&Event::In) on_read(err);
        if(event&Event::Out)on_write(err);
    }
};

std::string ServerProcess::received{};

TEST(Client_server,BufferHelloExchange){
    server::Settings settings;
    settings.host_ = "127.0.0.1";
    settings.port_ = 32397;
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
        server.set_processes_at_connections<ServerProcess>();
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
        std::unique_ptr<ClientProcess> proc = std::make_unique<ClientProcess>(hconn,err);
        hconn.add_process(std::move(proc),err);
        //std::this_thread::sleep_for(std::chrono::milliseconds(5));
        auto cmd = client.request<std::string>(hconn,
                    std::monostate(),
                    std::string("Hello server"),
                    std::monostate());
            cmd->wait_ready(10);
        EXPECT_EQ(ServerProcess::received,"Hello server");
        EXPECT_EQ(cmd->received()->data_frame(),"Hello client");
}


class ClientVectorProcess:public AbstractRequestableConnectionProcess{
    public:
    virtual void on_read(std::error_code& err) noexcept override{
            //std::cout<<"Client: receive ping"<<std::endl;
            using result_t = Frame<std::monostate,std::vector<int>,std::monostate>;
            result_t* result;
            auto recv = io_context().receive(err,*active_request_->received(result));
            if(err) handle_receive_error(err);
            else{
                active_request_->bytes_received(recv);
                if(active_request_->all_received(true)){
                    complete_current_request(err);
                    err.clear();
                }
            }
    }
    virtual void on_write(std::error_code& err) noexcept override{
            using sending_t = Frame<std::monostate,std::vector<int>,std::monostate>;
            auto sent = io_context().send(err,*active_request_->sent());
            if(err) handle_sending_error(err);
            else {
                active_request_->bytes_sent(sent);
                if(active_request_->all_sent(true))
                    err.clear();
            }
            // if(err!=std::error_code())
            //     //std::cout<<err.message()<<std::endl;
            // else {
            
            // }
    }
    virtual void on_task_done(std::error_code& err) noexcept override{
    }
    virtual void on_stop_requested(std::error_code& err) noexcept override{
        reset_requests(err);
        //std::cout<<"Client: stop requests"<<std::endl;
        //if(err!=std::error_code())
            //std::cout<<err.message()<<std::endl;
    }

    ClientVectorProcess(ConnectionHandle hconn,std::error_code& err):
        AbstractRequestableConnectionProcess(hconn,err){}
    ~ClientVectorProcess() = default;
    virtual void handle_event(
                Event event,
                std::error_code& err) noexcept
    {
        if(event&Event::In) on_read(err);
        //if(event&Event::Out)on_write(err);
        
    }
};

class ServerVectorProcess:public AbstractConnectionProcess{
    public:
    static std::vector<int> received;
    std::vector<int> msg;
    virtual void on_read(std::error_code& err) noexcept override{
        err.clear();
        io_context().receive(err,msg);
        if(err) {
            handle_receive_error(err);
            return;
        }
        received=msg;
        on_write(err);
        return;
    }
    virtual void on_write(std::error_code& err) noexcept override{
        std::vector<int> msg;
        for(int i=100;i<200;++i)
            msg.push_back(i);
        io_context().send(err,msg);
        if(err) handle_sending_error(err);
    }
    virtual void on_task_done(std::error_code& err) noexcept override{
    }
    virtual void on_stop_requested(std::error_code& err) noexcept override{
    }

    ServerVectorProcess(ConnectionHandle hconn,std::error_code& err):
        AbstractConnectionProcess(hconn,err){}
    ~ServerVectorProcess() = default;
    virtual void handle_event(
                Event event,
                std::error_code& err) noexcept
    {
        if(event&Event::In) on_read(err);
        if(event&Event::Out)on_write(err);
    }
};

std::vector<int> ServerVectorProcess::received{};

TEST(Client_server,BufferOverflowCase){
    server::Settings settings;
    settings.host_ = "127.0.0.1";
    settings.port_ = 32397;
    settings.protocol_ = Protocol::TCP;
    settings.num_threads_pool_ = 1;
    settings.timeout_seconds_processes_ = 3;
    settings.options_=ConnectionOptions{
        .reuse_address_{true,{}},
        .reuse_port_={true,{}},
        .keep_alive_={true,{}},
        .buffer_size_in_={20,{}},
        .buffer_size_out_={20,{}}};
    //for(int i = 0;i<5;++i){
        Server server;
        std::error_code err;
        server.configure(settings,
                            err);
        server.launch(err);
        server.set_processes_at_connections<ServerVectorProcess>();
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
        std::unique_ptr<ClientVectorProcess> proc = std::make_unique<ClientVectorProcess>(hconn,err);
        hconn.add_process(std::move(proc),err);
        //std::this_thread::sleep_for(std::chrono::milliseconds(5));
        std::vector<int> numbers;
        for(int i=0;i<100;++i)
            numbers.push_back(i);
        auto cmd = client.request<std::vector<int>>(hconn,
                    std::monostate(),
                    numbers,
                    std::monostate());
            cmd->wait_ready(10);
        std::vector<int> client_numbers;
        for(int i=100;i<200;++i)
            client_numbers.push_back(i);
        EXPECT_EQ(ServerVectorProcess::received,numbers);
        EXPECT_EQ(cmd->received()->data_frame(),client_numbers);
}

class A{
    public:
    void method(int val1,int val2){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
};

static std::atomic_bool caught_stop = false;
static std::atomic_bool not_caught_stop = true;
int foo(std::stop_token stop,double d, std::string str){
    std::this_thread::sleep_for(std::chrono::seconds(2));
    if(stop.stop_requested()){
        caught_stop.store(true,std::memory_order::release);
        not_caught_stop.store(false,std::memory_order::release);
        return 1;
    }
    return 1;
}

TEST(Process,LaunchProcess){
    Process proc;
    std::error_code err;
    A a;
    proc.emplace_binded_task<TaskMode::Thread>(err,&A::method,a,1,1);
    proc.emplace_task<TaskMode::Thread>(err,foo,1,"string");
    static_assert(std::is_invocable_v<decltype(foo),std::stop_token,int,const char[9]>);
    using foo_t = decltype(foo);
    // std::invoke_result_t<decltype(foo),std::stop_token,int,const char (&)[9]>;
    err.clear();
    EXPECT_FALSE(proc.is_ready(err));
    EXPECT_FALSE(err);
    EXPECT_TRUE(proc.is_busy(err));
    EXPECT_FALSE(err);
    EXPECT_TRUE(proc.has_task());
    EXPECT_FALSE(caught_stop.load(std::memory_order::relaxed));
    EXPECT_TRUE(not_caught_stop.load(std::memory_order::relaxed));
    proc.request_stop(0,err);
    EXPECT_FALSE(err);
    std::cout<<err.message()<<std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_TRUE(caught_stop.load(std::memory_order::relaxed));
    EXPECT_FALSE(not_caught_stop.load(std::memory_order::relaxed));
    EXPECT_FALSE(proc.has_task());
    EXPECT_FALSE(proc.is_busy(err));
    std::cout<<err.message()<<std::endl;
    EXPECT_FALSE(err);
    EXPECT_TRUE(proc.is_ready(err));
    EXPECT_FALSE(err);
    
}

int main(int argc, char* argv[]){
    signal(SIGPIPE,SIG_IGN);
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}