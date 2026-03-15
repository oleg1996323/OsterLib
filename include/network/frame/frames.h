#pragma once
#include <vector>
#include "serialization.h"
#include "dataframe.h"

namespace network{

    class AbstractFrame{
        protected:
        template<bool NETWORK_ORDER>
        friend struct serialization::Serialize;
        template<bool NETWORK_ORDER>
        friend struct serialization::Deserialize;
        friend struct serialization::Serial_size<AbstractFrame>;
        friend struct serialization::Max_serial_size<AbstractFrame>;
        friend struct serialization::Min_serial_size<AbstractFrame>;
        AbstractFrame(){}

        virtual serialization::SerializationEC deserialize(
                serialization::StreamSerializer& buffer) noexcept = 0;
        virtual serialization::SerializationEC serialize(
                std::vector<char>& buffer) const noexcept = 0;
        public:
        virtual size_t serial_size() const noexcept = 0;
        virtual size_t min_initial_size() const noexcept = 0;
    };

    template<typename START,typename DATA,typename END>
    class SenderFrame;

    template<typename START,typename DATA,typename END>
    class SenderFrame final:public AbstractFrame{
        template<bool,typename,typename,typename>
        friend struct serialization::Serialize;
        template<bool,typename,typename,typename>
        friend struct serialization::Deserialize;
        template<typename,typename,typename>
        friend struct serialization::Serial_size;
        template<typename,typename,typename>
        friend struct serialization::Max_serial_size;
        template<typename,typename,typename>
        friend struct serialization::Min_serial_size;
        DataFrame<START,DATA,END> frame_val_;
        public:
        SenderFrame(
            START&& start,
            DATA&& data,
            END&& end)
        {
            static_assert(!std::is_empty_v<decltype(frame_val_)>,"frame cannot be empty");
            if constexpr(!std::is_empty_v<START>)
                frame_val_.start_ = std::forward<START>(start);
            frame_val_.data_ = std::forward<DATA>(data);
            if constexpr(!std::is_empty_v<END>)
                frame_val_.end_ = std::forward<END>(end);
            
        }
        private:
        virtual serialization::SerializationEC serialize(
                std::vector<char>& buffer) const noexcept override;
        virtual serialization::SerializationEC deserialize(
                serialization::StreamSerializer& buffer) noexcept override;
        virtual size_t serial_size() const noexcept override;
        virtual size_t min_initial_size() const noexcept override;
    };

    template<typename START,typename DATA,typename END>
    class ReceiverFrame final:public AbstractFrame{
        template<bool,typename,typename,typename>
        friend struct serialization::Serialize;
        template<bool,typename,typename,typename>
        friend struct serialization::Deserialize;
        template<typename,typename,typename>
        friend struct serialization::Serial_size;
        template<typename,typename,typename>
        friend struct serialization::Max_serial_size;
        template<typename,typename,typename>
        friend struct serialization::Min_serial_size;
        DataFrame<START,DATA,END> frame_val_;
        public:
        ReceiverFrame(){}
        private:
        virtual serialization::SerializationEC serialize(
                std::vector<char>& buffer) const noexcept override;

        virtual serialization::SerializationEC deserialize(
                serialization::StreamSerializer& buffer) noexcept override;
        virtual size_t serial_size() const noexcept override;
        virtual size_t min_initial_size() const noexcept override;
    };
}

namespace serialization{
    template<bool NETWORK_ORDER,typename START,typename DATA,typename END>
    struct Serialize<NETWORK_ORDER,network::SenderFrame<DATA,START,END>>{
        using type = network::SenderFrame<DATA,START,END>;
        auto operator()(const type& val,
                    std::vector<char>& buf) const noexcept{
            static_assert(NETWORK_ORDER==true,"Frames support only network-endianess serialization");
            return serialize<NETWORK_ORDER>(val,buf,val.frame_val_);
        }
    };

    template<bool NETWORK_ORDER,typename START,typename DATA,typename END>
    struct Deserialize<NETWORK_ORDER,network::SenderFrame<DATA,START,END>>{
        using type = network::SenderFrame<DATA,START,END>;
        auto operator()(type& val,
                    StreamSerializer& buf) const noexcept{
            static_assert(NETWORK_ORDER==true,"Frames support only network-endianess serialization");
            return deserialize<NETWORK_ORDER>(val,buf,val.frame_val_);
        }
    };

    template<typename START,typename DATA,typename END>
    struct Serial_size<network::SenderFrame<DATA,START,END>>{
        using type = network::SenderFrame<DATA,START,END>;
        size_t operator()(const type& val) const noexcept{
            return serial_size(val.frame_val_);
        }
    };

    template<typename START,typename DATA,typename END>
    struct Min_serial_size<network::SenderFrame<DATA,START,END>>{
        using type = network::SenderFrame<DATA,START,END>;
        static constexpr size_t value = []()
        {
            return min_serial_size<
                    decltype(type::frame_val_)>();
        }();
    };
     
    template<typename START,typename DATA,typename END>
    struct Max_serial_size<network::SenderFrame<DATA,START,END>>{
        using type = network::SenderFrame<DATA,START,END>;
        static constexpr size_t value = []()
        {
            return max_serial_size<
                    decltype(type::frame_val_)>();
        }();
    };

    template<bool NETWORK_ORDER,typename START,typename DATA,typename END>
    struct Serialize<NETWORK_ORDER,network::ReceiverFrame<DATA,START,END>>{
        using type = network::ReceiverFrame<DATA,START,END>;
        auto operator()(const type& val,
                    std::vector<char>& buf) const noexcept{
            static_assert(NETWORK_ORDER==true,"Frames support only network-endianess serialization");
            return serialize<NETWORK_ORDER>(val,buf,val.frame_val_);
        }
    };

    template<bool NETWORK_ORDER,typename START,typename DATA,typename END>
    struct Deserialize<NETWORK_ORDER,network::ReceiverFrame<DATA,START,END>>{
        using type = network::ReceiverFrame<DATA,START,END>;
        auto operator()(type& val,
                    StreamSerializer& buf) const noexcept{
            static_assert(NETWORK_ORDER==true,"Frames support only network-endianess serialization");
            return deserialize<NETWORK_ORDER>(val,buf,val.frame_val_);
        }
    };

    template<typename START,typename DATA,typename END>
    struct Serial_size<network::ReceiverFrame<DATA,START,END>>{
        using type = network::ReceiverFrame<DATA,START,END>;
        size_t operator()(const type& val) const noexcept{
            return serial_size(val.frame_val_);
        }
    };

    template<typename START,typename DATA,typename END>
    struct Min_serial_size<network::ReceiverFrame<DATA,START,END>>{
        using type = network::ReceiverFrame<DATA,START,END>;
        static constexpr size_t value = []()
        {
            return min_serial_size<
                    decltype(type::frame_val_)>();
        }();
    };
     
    template<typename START,typename DATA,typename END>
    struct Max_serial_size<network::ReceiverFrame<DATA,START,END>>{
        using type = network::ReceiverFrame<DATA,START,END>;
        static constexpr size_t value = []()
        {
            return max_serial_size<
                    decltype(type::frame_val_)>();
        }();
    };
    template<bool NETWORK_ORDER>
    struct Serialize<NETWORK_ORDER,network::AbstractFrame>{
        using type = network::AbstractFrame;
        auto operator()(const type& val,
                    std::vector<char>& buf) const noexcept{
            static_assert(NETWORK_ORDER==true,"Frames support only network-endianess serialization");
            return val.serialize(buf);
        }
    };

    template<bool NETWORK_ORDER>
    struct Deserialize<NETWORK_ORDER,network::AbstractFrame>{
        using type = network::AbstractFrame;
        auto operator()(type& val,
                    StreamSerializer& buf) const noexcept{
            static_assert(NETWORK_ORDER==true,"Frames support only network-endianess serialization");
            return val.deserialize(buf);
        }
    };

    template<>
    struct Serial_size<network::AbstractFrame>{
        using type = network::AbstractFrame;
        size_t operator()(const type& val) const noexcept{
            return val.serial_size();
        }
    };

    template<>
    struct Min_serial_size<network::AbstractFrame>{
        using type = network::AbstractFrame;
        static constexpr size_t value = []()
        {
            return 0;
        }();
    };
     
    template<>
    struct Max_serial_size<network::AbstractFrame>{
        using type = network::AbstractFrame;
        static constexpr size_t value = []()
        {
            return std::numeric_limits<uint64_t>::max();
        }();
    };
}

namespace network{
    template<typename START,typename DATA,typename END>
    serialization::SerializationEC SenderFrame<START,DATA,END>::serialize(
            std::vector<char>& buffer) const noexcept
    {
        return serialization::serialize_network(
            frame_val_,
            buffer);
    }
    template<typename START,typename DATA,typename END>
    serialization::SerializationEC SenderFrame<START,DATA,END>::deserialize(
            serialization::StreamSerializer& buffer) noexcept
    {
        return serialization::deserialize_network(
            frame_val_,
            buffer);
    }
    template<typename START,typename DATA,typename END>
    size_t SenderFrame<START,DATA,END>::serial_size() const noexcept{
        return serialization::serial_size(frame_val_);
    }

    template<typename START,typename DATA,typename END>
    size_t SenderFrame<START,DATA,END>::min_initial_size() const noexcept
    {
        return frame_val_.min_initial_size();
    }

    template<typename START,typename DATA,typename END>
    serialization::SerializationEC ReceiverFrame<START,DATA,END>::serialize(
            std::vector<char>& buffer) const noexcept
    {
        return serialization::serialize_network(
            frame_val_,
            buffer);
    }
    template<typename START,typename DATA,typename END>
    serialization::SerializationEC ReceiverFrame<START,DATA,END>::deserialize(
            serialization::StreamSerializer& buffer) noexcept
    {
        return serialization::deserialize_network(
            frame_val_,
            buffer);
    }
    template<typename START,typename DATA,typename END>
    size_t ReceiverFrame<START,DATA,END>::serial_size() const noexcept{
        return serialization::serial_size(frame_val_);
    }

    template<typename START,typename DATA,typename END>
    size_t ReceiverFrame<START,DATA,END>::min_initial_size() const noexcept
    {
        return frame_val_.min_initial_size();
    }
}