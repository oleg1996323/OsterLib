#pragma once
#include "typed_msg.h"
#include <functional>
#include <memory>

namespace network{
    class MessageFactory{
        using Creator = std::function<std::unique_ptr<Message>()>;
        static MessageFactory& instance() {
            static MessageFactory inst;
            return inst;
        }

        void register_type(MessageType type, Creator creator) {
            creators_[type] = std::move(creator);
        }

        std::unique_ptr<Message> create(MessageType type) const {
            auto it = creators_.find(type);
            if (it != creators_.end()) {
                return it->second();
            }
            return nullptr;
        }

    private:
        std::unordered_map<MessageType, Creator> creators_;
    };

    #define REGISTER_MESSAGE_TYPE(MT, PAYLOAD_TYPE) \
    namespace { \
        struct Register_##MT { \
            Register_##MT() { \
                MessageFactory::instance().register_type(MT, []() { \
                    return std::make_unique<TypedMessage<MT, PAYLOAD_TYPE>>(); \
                }); \
            } \
        }; \
        static Register_##MT g_register_##MT; \
    }
}