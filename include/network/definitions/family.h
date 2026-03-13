#pragma once
#include <sys/socket.h>
#include <cstdint>

enum class Family : sa_family_t{
    UNDEF = AF_UNSPEC,
    IPv4 = AF_INET,
    IPv6 = AF_INET6
};