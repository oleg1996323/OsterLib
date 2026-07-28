#include "OsterLib/boost_functional/json.h"

std::expected<boost::json::value,std::error_code> parse_json_from_file(const fs::path& path) noexcept{
    using namespace boost;
    if(fs::exists(path)){
        std::ifstream file(path,std::ifstream::in);
        if(!file.is_open())
            return std::unexpected(std::make_error_code(std::errc::file_exists));
        json::stream_parser parser;
        json::error_code err_code;
        std::vector<char> buffer;
        std::error_code err;
        size_t buf_sz = fs::file_size(path,err);
        if(err.value()!=0)
            return std::unexpected(err);
        buffer.resize(buf_sz);
        while(file.good()){
            file.read(buffer.data(),buffer.size());
            parser.write(buffer.data(),file.gcount(),err_code);
            if(err_code)
                return std::unexpected(std::make_error_code(std::errc::bad_message));
        }
        if(!parser.done())
            return std::unexpected(std::make_error_code(std::errc::bad_message));
        else
            parser.finish();
        return parser.release();
    }
    else return std::unexpected(std::make_error_code(std::errc::no_such_file_or_directory));
}

std::expected<boost::json::value,std::error_code> parse_json_from_buffer(std::string_view input) noexcept{
    using namespace boost;
        json::stream_parser parser;
        json::error_code err_code;
        parser.write(input.data(),input.size(),err_code);
        if(err_code)
            return std::unexpected(std::make_error_code(std::errc::bad_message));
        if(!parser.done())
            return std::unexpected(std::make_error_code(std::errc::bad_message));
        else
            parser.finish();
        return parser.release();
}

std::expected<boost::json::value,std::error_code> parse_json_from_buffer(const std::string& input) noexcept{
    return parse_json_from_buffer(std::string_view(input));
}

template<>
boost::json::value to_json(const linger& val){
    boost::json::object result;
    result["active"]=static_cast<bool>(val.l_onoff);
    result["duration(sec)"]=static_cast<int32_t>(val.l_linger);
    return result;
}
template<>
std::expected<linger,std::exception> from_json(const boost::json::value& val){
    if(!val.is_object() && !val.is_null())
        return std::unexpected(std::invalid_argument("not object"));
    auto& c = val.as_object();
    linger result;
    if(c.contains("active")){
        if(auto tmp = from_json<bool>(c.at("active"));tmp.has_value())
            result.l_onoff = tmp.value();
    }
    if(c.contains("duration(sec)")){
        if(auto tmp = from_json<int32_t>(c.at("duration(sec)"));tmp.has_value())
            result.l_linger = tmp.value();
    }
    return result;
}

template<>
boost::json::value to_json(const timeval& val){
    boost::json::object tv;
    tv["duration(sec)"] = val.tv_sec;
    tv["duration(usec)"] = val.tv_usec;
    return tv;
}

template<>
std::expected<timeval,std::exception> from_json(const boost::json::value& val){
    if(!val.is_object() && !val.is_null())
        return std::unexpected(std::invalid_argument("not object"));
    auto& c = val.as_object();
    timeval result;
    if(c.contains("duration(sec)")){
        if(auto tmp = from_json<decltype(result.tv_sec)>(c.at("duration(sec)"));tmp.has_value())
            result.tv_sec = tmp.value();
    }
    if(c.contains("duration(usec)")){
        if(auto tmp = from_json<decltype(result.tv_usec)>(c.at("duration(usec)"));tmp.has_value())
            result.tv_usec = tmp.value();
    }
    return result;
}