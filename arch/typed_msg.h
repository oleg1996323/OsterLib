#pragma once
#include "msg.h"
#include "serialization.h"

namespace network{
    template<MessageType MT, typename Payload>
    class TypedMessage : public Message {
    public:
        TypedMessage() = default;
        explicit TypedMessage(const Payload& p) : payload_(p) {}

        MessageType type() const noexcept override { return MT; }

        std::vector<char> serialize(std::error_code& err) const override {
            std::vector<char> buf;
            // Пишем тип сообщения первым байтом (чтобы при десериализации можно было проверить)
            buf.push_back(static_cast<uint8_t>(MT));
            // Сериализуем полезную нагрузку
            serialization::serialize_network(type_to_text(MT,err), buf);
            serialization::serialize_network(payload_, buf);  // предположим, что такая функция есть
            return buf;
        }

        bool deserialize(std::span<const char> data) override {
            if (data.empty()) return false;
            MessageType received = static_cast<MessageType>(data[0]);
            if (received != MT) return false;  // проверка соответствия типа
            auto payload_span = data.subspan(1);
            auto ec = serialization::deserialize_network(payload_, payload_span);
            return ec == serialization::SerializationEC::NONE;
        }

        Payload& payload() { return payload_; }
        const Payload& payload() const { return payload_; }

    private:
        Payload payload_;
    };
}