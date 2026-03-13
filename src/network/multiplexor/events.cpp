#include "multiplexor/events.h"

network::Event operator|(
        network::Event lhs,
        network::Event rhs) noexcept{
    return static_cast<network::Event>(
        static_cast<std::underlying_type_t<network::Event>>(lhs)|
        static_cast<std::underlying_type_t<network::Event>>(rhs));
}
network::Event operator&(
        network::Event lhs,
        network::Event rhs) noexcept{
    return static_cast<network::Event>(
        static_cast<std::underlying_type_t<network::Event>>(lhs)&
        static_cast<std::underlying_type_t<network::Event>>(rhs));
}
network::Event operator^(
        network::Event lhs,
        network::Event rhs) noexcept{
    return static_cast<network::Event>(
        static_cast<std::underlying_type_t<network::Event>>(lhs)^
        static_cast<std::underlying_type_t<network::Event>>(rhs));
}
network::Event operator~(
        network::Event val) noexcept{
    return static_cast<network::Event>(
        ~static_cast<std::underlying_type_t<
            network::Event>>(val));
}
network::Event operator&(
        network::Event lhs,int rhs) noexcept{
    return static_cast<network::Event>(
        static_cast<std::underlying_type_t<network::Event>>(lhs)&
            rhs);
}
network::Event operator&(
        std::underlying_type_t<network::Event> lhs,
        network::Event rhs) noexcept{
    return static_cast<network::Event>(lhs &
            static_cast<std::underlying_type_t<
                network::Event>>(rhs));
}
network::Event operator|(
        network::Event lhs,
        std::underlying_type_t<network::Event> rhs) noexcept{
    return static_cast<network::Event>(
            static_cast<std::underlying_type_t<
            network::Event>>(lhs)|
            rhs);
}
network::Event operator|(
        std::underlying_type_t<network::Event> lhs,
        network::Event rhs) noexcept{
    return static_cast<network::Event>(lhs |
            static_cast<std::underlying_type_t<
            network::Event>>(rhs));
}