#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include "boost_functional/json.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
namespace fs = std::filesystem;
bool change_directory(const char* dir);
bool change_directory(const fs::path& dir);
bool change_directory(const std::string& dir);
bool change_directory(std::string_view dir);

inline bool directory_accessible(const fs::path& path) noexcept{
    if(!fs::exists(path)){
        if(!fs::create_directories(path))
            return false;
        else return true;
    }
    else return true;
}

namespace std::filesystem{
    inline std::string separator(){
        #ifdef _WIN32
            return "\\";
        #else 
            return "/";
        #endif
    }
}

template<typename PREDICATE>
std::error_code safe_write_to_file(
    const fs::path& directory,
    const std::string& filename,
    PREDICATE pred)
{
    auto gen_fn_num = boost::uuids::random_generator()();
    std::string gen_fn_str = std::string(gen_fn_num.begin(),gen_fn_num.end());
    std::ofstream fd(directory/gen_fn_str,
        std::ios::out|std::ios::trunc);
    if(!fd.is_open()){
        return std::make_error_code(std::errc::inappropriate_io_control_operation); //cannot open
    }
    else{
        
        try{
            static_assert(std::is_invocable_r_v<bool, PREDICATE, std::ofstream&>,
        "PREDICATE must be callable with std::ofstream& and return bool (or convertible to bool)");
            if(pred(fd)){
                fd.close();
                fs::remove(directory/gen_fn_str);
                return std::make_error_code(std::errc::operation_canceled); //PREDICATE error
            }
        }
        catch(...){
            fd.close();
            fs::remove(directory/gen_fn_str);
            return std::make_error_code(std::errc::operation_canceled); //PREDICATE error
        }
        fd.close();
        fs::remove(directory/filename);
        fs::rename(directory/gen_fn_str,
            directory/filename);
        return std::error_code();
    }
}