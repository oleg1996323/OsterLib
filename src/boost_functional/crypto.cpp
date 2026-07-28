#include "OsterLib/boost_functional/crypto.h"
#include <boost/algorithm/hex.hpp>

namespace crypto{
    namespace detail{
        std::string to_string(const boost::uuids::detail::md5::digest_type &digest) noexcept{
            std::string result;
            boost::algorithm::hex(digest, digest + (detail::md_sz-1), std::back_inserter(result));
            return result;
        }
        std::string to_string(const boost::uuids::detail::sha1::digest_type &digest) noexcept{
            std::string result;
            boost::algorithm::hex(digest, digest + (detail::sha1_sz-1), std::back_inserter(result));
            return result;
        }
    }
}