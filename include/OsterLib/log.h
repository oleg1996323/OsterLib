#pragma once
#include <fstream>
#include <chrono>
#include <filesystem>
#include <format>
#include <mutex>
#include "error.h"
#include <format>
#include <iostream>

namespace osterlib{
    class Log:public std::ofstream{
        std::mutex m_;
        static std::streambuf* cout_buffer_;
        static std::streambuf* clog_buffer_;
        public:
        Log(const std::filesystem::path& log_dir,
                std::error_code& err);
        Log();
        ~Log();
        void attach_cout(bool val) noexcept;
        void attach_clog(bool val) noexcept;
        template<typename... Args>
        Log& log(std::error_code err){
            *this<<get_log_time()<<": "<<err.message()<<std::endl;
            return *this;
        }
        template<typename... Args>
        Log& log(const ContextedError& err){
            *this<<get_log_time()<<": "<<err.what()<<std::endl;
            return *this;
        }

        static std::string get_log_time();
    };
}