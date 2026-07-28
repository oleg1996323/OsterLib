#pragma once
#include "OsterLib/network/definitions/family.h"
#include "OsterLib/network/definitions/protocol.h"
#include <cstdint>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>

namespace network{
    using Port = uint16_t;
    using FileDescriptor = int32_t;
    using Timeout = int32_t; //second
}