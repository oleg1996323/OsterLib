#pragma once
#include "frame/msg.h"
#include <flat_map>

namespace network{
    const static std::flat_map<MessageType,std::string> t_to_text
    {
        {MessageType::None,std::string("")},
        {MessageType::SizeBefore,std::string("SZ/")},
        {MessageType::HTTP1_0,std::string("HTTP/1.0")},
        {MessageType::HTTP1_1,std::string("HTTP/1.1")}
    };

    const static std::flat_map<std::string_view,MessageType> text_to_t = 
    [](const std::flat_map<MessageType,std::string>& to_text) noexcept{
        std::flat_map<std::string_view,MessageType> result;
        for(const auto& [msg,str]:to_text)
            result[str]=msg;
        return result;
    }(t_to_text);

    std::string_view type_to_text(MessageType msg,std::error_code& err) noexcept{
        if(auto found = t_to_text.find(msg);found==t_to_text.end()){
            err = std::make_error_code(std::errc::bad_message);
            return {};
        }
        else{
            err.clear();
            return found->second;
        }
    }
    std::optional<MessageType> parse_msg_type(const std::span<const char> buffer) noexcept{
        auto text = std::string_view(buffer);
        auto found = text_to_t.lower_bound(text);
        if(found!=text_to_t.end() && text.starts_with(found->first))
            return found->second;
        else return std::nullopt;
    }
}