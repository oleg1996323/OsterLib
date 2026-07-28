#pragma once
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/name_generator_sha1.hpp>
#include <boost/uuid/name_generator_md5.hpp>

namespace crypto{
    namespace detail{
        constexpr size_t sha1_sz = [](){
            boost::uuids::detail::sha1::digest_type arr;
            return sizeof(arr)/sizeof(arr[0]);
        }();
        constexpr size_t md_sz = [](){
            boost::uuids::detail::md5::digest_type arr;
            return sizeof(arr)/sizeof(arr[0]);
        }();
    }

    using SHA1 = boost::uuids::detail::sha1::digest_type;
    using MD5 = boost::uuids::detail::md5::digest_type;

    namespace detail{
        std::string to_string(const MD5 &digest) noexcept;
        std::string to_string(const SHA1 &digest) noexcept;
    }

    template<typename DIGEST>
    std::string digest_to_string(const DIGEST& digest) noexcept{
        static_assert(std::is_same_v<SHA1,DIGEST> ||
                    std::is_same_v<MD5,DIGEST>);
        return detail::to_string(digest);
    }
}