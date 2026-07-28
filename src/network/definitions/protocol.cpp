#include "OsterLib/network/definitions/protocol.h"
#include <unordered_map>

namespace network{
    const std::unordered_map<Protocol,std::string_view> num_to_proto={
        {Protocol::IP,"ip"},
        {Protocol::ICMP,"icmp"},
        {Protocol::IGMP,"igmp"},
        {Protocol::IPIP,"ipip"},
        {Protocol::TCP,"tcp"},
        {Protocol::EGP,"egp"},
        {Protocol::PUP,"pup"},
        {Protocol::UDP,"udp"},
        {Protocol::IDP,"idp"},
        {Protocol::TP,"tp"},
        {Protocol::DCCP,"dccp"},
        {Protocol::IPV6,"ipv6"},
        {Protocol::RSVP,"rsvp"},
        {Protocol::GRE,"gre"},
        {Protocol::ESP,"esp"},
        {Protocol::AH,"ah"},
        {Protocol::MTP,"mtp"},
        {Protocol::BEETPH,"beetph"},
        {Protocol::ENCAP,"encap"},
        {Protocol::PIM,"pim"},
        {Protocol::COMP,"comp"},
        {Protocol::SCTP,"sctp"},
        {Protocol::UDPLITE,"udplite"},
        {Protocol::MPLS,"mpls"},
        {Protocol::ETHERNET,"eth"},
        {Protocol::RAW,"raw"},
        {Protocol::MPTCP,"mptcp"}
    };
    const std::unordered_map<std::string_view,Protocol> proto_to_num=[]
        (const std::unordered_map<Protocol,std::string_view>& to_proto)
    {
        std::unordered_map<std::string_view,Protocol> result;
        for(auto& [num,proto]:to_proto)
            result[proto]=num;
        return result;
    }(num_to_proto);
    namespace protocol{
        std::optional<Protocol> to_number(std::string_view proto) noexcept{
            if(auto res = proto_to_num.find(proto);res!=proto_to_num.end())
                return res->second;
            else return std::nullopt;
        }
        std::string_view to_text(Protocol number) noexcept{
            if(auto res = num_to_proto.find(number);res!=num_to_proto.end())
                return res->second;
            else return std::string_view();
        }
    }
}