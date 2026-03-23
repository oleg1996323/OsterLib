#include "multiplexor.h"
#include "commonsocket.h"
#include "connection.h"

void network::Multiplexor::__epoll_ctl_throw__(std::error_code& err){
    err = 
        std::make_error_code(static_cast<std::errc>(errno));
    errno = 0;
}

void network::Multiplexor::__epoll_wait_throw__(std::error_code& err){
    err = 
        std::make_error_code(static_cast<std::errc>(errno));
    errno = 0;
}
bool network::Multiplexor::__set_interruptor__(std::error_code& err){
    if(!interruptor){
        if(int ev = eventfd(0,EFD_NONBLOCK);ev==-1){
            std::error_code err = 
                std::make_error_code(
                static_cast<std::errc>(errno));
            errno = 0;
            return false;
        }
        else interruptor = std::move(std::unique_ptr<Interruptor>(new Interruptor(ev)));
        epoll_event ev{.events = Event::In};
        ev.data.u64 = si_ctrl;
        if(epoll_ctl(epollfd,EPOLL_CTL_ADD,interruptor->fd_,&ev)==-1){
            __epoll_ctl_throw__(err);
            return false;
        }
        else err.clear();
    }
    return true;
}
network::Multiplexor::Multiplexor(size_t mp_controled,std::error_code& err) noexcept:
events_([mp_controled](){
    std::vector<EventHandle> res;
    res.resize(mp_controled);
    return res;
}()),
epollfd(epoll_create(mp_controled)){
    if(errno!=0){
        err = std::make_error_code(static_cast<std::errc>(errno));
        errno = 0;
        std::cout<<err.message()<<std::endl;
        return;
    }
    __set_interruptor__(err);
    if(epollfd==-1){
        err=std::make_error_code(std::errc::bad_file_descriptor);
        return;
    }
}
network::Multiplexor::~Multiplexor(){
    int old = std::exchange(epollfd,-1);
    ::close(old);
}

bool network::Multiplexor::add(
        FileDescriptor fd,
        Event event,
        std::error_code& err) noexcept{
    if(epollfd==-1){
        err=std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }
    epoll_event ev;
    if(fd<0){
        err=std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }
    ev.events = event;
    ev.data.fd = fd;
    if(epoll_ctl(
                epollfd,
                EPOLL_CTL_ADD,
                fd,
                &ev)==-1)
    {
        __epoll_ctl_throw__(err);
        remove(fd,err);
    }
    return true;
}
bool network::Multiplexor::add(
        FileDescriptor fd,
        EventHandle ev,
        std::error_code& err) noexcept{
    if(epollfd==-1){
        err=std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }
    if(fd<0){
        err = std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }
    
    if(epoll_ctl(
                epollfd,
                EPOLL_CTL_ADD,
                fd,
                ev.native())==-1)
    {
        remove(fd,err);
        __epoll_ctl_throw__(err);
        return false;
    }
    err.clear();
    return true;
}
bool network::Multiplexor::add(
        FileDescriptor fd,
        uint32_t val,
        Event event,
        std::error_code& err) noexcept{
    if(epollfd==-1){
        err=std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }
    epoll_event ev;
    if(fd<0){
        err=std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }
    ev.events = event;
    ev.data.u32 = val;
    if(epoll_ctl(
                epollfd,
                EPOLL_CTL_ADD,
                fd,
                &ev)==-1)
    {
        remove(fd,err);
        __epoll_ctl_throw__(err);
    }
    return true;
}
bool network::Multiplexor::add(
        FileDescriptor fd,
        uint64_t val,
        Event event,
        std::error_code& err) noexcept{
    if(epollfd==-1){
        err=std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }
    epoll_event ev;
    if(fd<0)
        return false;
    
    ev.events = event;
    ev.data.u64 = val;
    if(epoll_ctl(
                epollfd,
                EPOLL_CTL_ADD,
                fd,
                &ev)==-1)
    {
        remove(fd,err);
        __epoll_ctl_throw__(err);
    }
    return true;
}
bool network::Multiplexor::add(
        FileDescriptor fd,
        void* ptr,
        Event event,
        std::error_code& err) noexcept{
    if(epollfd==-1){
        err=std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }
    epoll_event ev;
    if(fd<0)
        return false;
    
    ev.events = event;
    ev.data.ptr = ptr;
    if(epoll_ctl(
                epollfd,
                EPOLL_CTL_ADD,
                fd,
                &ev)==-1)
    {
        remove(fd,err);
        __epoll_ctl_throw__(err);
    }
    return true;
}
bool network::Multiplexor::remove(
        int fd,
        std::error_code& err) noexcept
{
    if(epollfd==-1){
        err=std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }
    if(epoll_ctl(
                epollfd,
                EPOLL_CTL_DEL,
                fd,
                nullptr)==-1)
    {
        __epoll_ctl_throw__(err);
        return false;
    }
    return true;
}
bool network::Multiplexor::modify(
        FileDescriptor fd,
        EventHandle hevent,
        std::error_code& err) noexcept
{
    if(epollfd==-1){
        err=std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }
    if(fd<0){
        err=std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }
    if(epoll_ctl(epollfd,
        EPOLL_CTL_MOD,fd,
        hevent.native())==-1){
        __epoll_ctl_throw__(err);
        if(err.value()==
            static_cast<int>(std::errc::bad_file_descriptor))
            remove(fd,err);
        return false;
    }
    return true;
}

bool network::Multiplexor::modify(
        int fd,
        Event event,
        std::error_code& err) noexcept
{
    if(epollfd==-1){
        err=std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }
    if(fd<0)
        return false;
    epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    if(epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev)==-1){
        __epoll_ctl_throw__(err);
        if(err.value()==
            static_cast<int>(std::errc::bad_file_descriptor))
            remove(fd,err);
        return false;
    }
    return true;
}
void network::Multiplexor::interrupt() noexcept{
    uint64_t one = 1;
    if(interruptor.get()!=nullptr)
        write(interruptor->fd_, &one, sizeof(one));
}
/**
 * @param timeout - in milliseconds
 * If timeout == -1 than infinite timeout is set.
 */
std::span<network::EventHandle> 
    network::Multiplexor::wait(
        std::error_code& err,
        int32_t timeout) noexcept
{
    if(epollfd==-1){
        err=std::make_error_code(std::errc::bad_file_descriptor);
        return{};
    }
    int event_sz;
    do {
        event_sz = epoll_wait(
            epollfd,
            reinterpret_cast<epoll_event*>(events_.data()),
            events_.size(),
            timeout);
    }
    while(event_sz == -1 && errno == EINTR);
    if (event_sz == -1) {
        if(fcntl(epollfd,F_GETFD)==-1)
            std::terminate();
        __epoll_wait_throw__(err);
        return {};
    }

    if (event_sz == 0) {
        err.clear();
        return {};
    }
    int valid_events = 0;
    for(int i = 0; i < event_sz; ++i) {
        if(events_[i].get_as_64() == si_ctrl)
        {
            uint64_t dummy;
            while (read(interruptor->fd_, &dummy, sizeof(dummy))==sizeof(dummy)){}
            if (errno != EAGAIN) {
                err = std::make_error_code(static_cast<std::errc>(errno));
                return {};
        }
            continue;
        }
        events_[valid_events++] = events_[i];
    }
    return std::span(events_).subspan(0, valid_events);
}