#include "OsterLib/log.h"

namespace osterlib{
    std::streambuf* Log::cout_buffer_ = std::cout.rdbuf();
    std::streambuf* Log::clog_buffer_ = std::clog.rdbuf();

    Log::Log(const std::filesystem::path& log_dir,
            std::error_code& err){
        if(!std::filesystem::exists(log_dir))
        {
            if(!std::filesystem::create_directories(log_dir))
                err = std::make_error_code(osterlib::errc::create_directory_denied);
        }
        if(!std::filesystem::is_directory(log_dir))
            err = std::make_error_code(osterlib::errc::not_directory);
        std::filesystem::path filename = std::filesystem::path(log_dir)/(std::format("{:%Y_%m_%d_%H_%M_%S}",std::chrono::system_clock::now())+".txt");
        open(filename,std::ios::trunc);
        if(!is_open())
            err = std::make_error_code(osterlib::errc::file_permission_denied);
    }

    Log::Log(){
        set_rdbuf(std::clog.rdbuf());
    }

    Log::~Log(){
        flush();
        close();
    }

    void Log::attach_cout(bool val) noexcept{
        if(val)
            std::cout.rdbuf(rdbuf());
        else std::cout.rdbuf(cout_buffer_);
    }
    void Log::attach_clog(bool val) noexcept{
        if(val)
            std::clog.rdbuf(rdbuf());
        else std::clog.rdbuf(clog_buffer_);
    }

    std::string Log::get_log_time(){
        return std::format("{:%Y/%m/%d %H:%M:%S}",std::chrono::system_clock::now());
    }
}