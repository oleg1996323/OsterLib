#pragma once
#include "definitions/family.h"
#include "definitions/protocol.h"
#include <cstdint>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>

namespace network{
    using Port = uint16_t;
    using FileDescriptor = int;
}