#pragma once
#include <string_view>
#include <vector>
#include <span>
#include "serialization.h"

namespace network{
    enum class FrameType{
        NotFramed,
        SizeBefore
    };
    struct AbstractDataFrame{
        virtual serialization::SerializationEC serialize(
                std::vector<char>& buf) const noexcept = 0;
        virtual serialization::SerializationEC deserialize(
                std::span<const char> buf) noexcept = 0;
        virtual size_t serial_size() const noexcept = 0;
        virtual size_t min_initial_size() const noexcept = 0;
    };

    template<typename START,typename DATA, typename END>
    struct DataFrame:public AbstractDataFrame{
        START start_;
        DATA data_;
        END end_;
        virtual serialization::SerializationEC serialize(
            std::vector<char>& buf) const noexcept override{
            return serialization::serialize<true>(*this,buf,start_,data_,end_);
        }
        virtual serialization::SerializationEC deserialize(
                std::span<const char> buf) noexcept override{
            return serialization::deserialize<true>(*this,buf,start_,data_,end_);
        }
        virtual size_t serial_size() const noexcept{
            return serialization::serial_size(*this);
        }
        virtual size_t min_initial_size() const noexcept{
            return serialization::min_serial_size(start_);
        }
    };

    template<typename DATA>
    using NotFramedData = DataFrame<std::monostate,DATA,std::monostate>;
    template<typename DATA>
    using SizeFramedData = DataFrame<size_t,DATA,std::monostate>;
}

namespace serialization{
    template<bool NETWORK_ORDER,typename START,typename DATA,typename END>
    struct Serialize<NETWORK_ORDER,network::DataFrame<START,DATA,END>>{
        using type = network::DataFrame<START,DATA,END>;
        auto operator()(const type& val,
                    std::vector<char>& buf) const noexcept{
            static_assert(NETWORK_ORDER==true,"Frames support only network-endianess serialization");
            return serialize<NETWORK_ORDER>(val,buf,val.start_,val.data_,val.end_);
        }
    };

    template<bool NETWORK_ORDER,typename START,typename DATA,typename END>
    struct Deserialize<NETWORK_ORDER,network::DataFrame<START,DATA,END>>{
        using type = network::DataFrame<START,DATA,END>;
        auto operator()(type& val,
                    std::span<const char> buf) const noexcept{
            static_assert(NETWORK_ORDER==true,"Frames support only network-endianess serialization");
            return deserialize<NETWORK_ORDER>(val,buf,val.start_,val.data_,val.end_);
        }
    };

    template<typename START,typename DATA,typename END>
    struct Serial_size<network::DataFrame<START,DATA,END>>{
        using type = network::DataFrame<START,DATA,END>;
        size_t operator()(const type& val) const noexcept{
            return serial_size(val.start_,val.data_,val.end_);
        }
    };

    template<typename START,typename DATA,typename END>
    struct Min_serial_size<network::DataFrame<START,DATA,END>>{
        using type = network::DataFrame<START,DATA,END>;
        static constexpr size_t value = []()
        {
            return min_serial_size<
                    decltype(type::start_),
                    decltype(type::data_),
                    decltype(type::end_)>();
        }();
    };
     
    template<typename START,typename DATA,typename END>
    struct Max_serial_size<network::DataFrame<START,DATA,END>>{
        using type = network::DataFrame<START,DATA,END>;
        static constexpr size_t value = []()
        {
            return max_serial_size<
                    decltype(type::start_),
                    decltype(type::data_),
                    decltype(type::end_)>();
        }();
    };
}