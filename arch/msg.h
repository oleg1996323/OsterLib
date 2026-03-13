#pragma once
#include <vector>
#include <span>
#include <memory>
#include <cstdint>
#include <optional>

namespace network{
    enum class MessageType : uint8_t {
        None,
        SizeBefore,
        HTTP1_0,
        HTTP1_1
    };
    class Message {
        public:
        virtual ~Message() = default;
        virtual MessageType type() const noexcept = 0;
        virtual std::vector<char> serialize() const = 0;
        virtual bool deserialize(std::span<const char> data) = 0;
    };

    static std::string_view msg_type_to_text(MessageType msg,std::error_code& err) noexcept;
    static MessageType text_to_msg_type(const std::string& text,std::error_code& err) noexcept;
    static std::optional<MessageType> parse_msg_type(const std::span<const char> buffer) noexcept;
}