#pragma once

namespace network{
    enum CommandType
    {
        AddConnection,
        RemoveConnection,
        ModifyConnection,
        AttachProcess,
        RemoveProcess,
        ShutDownConnection,
        AttachConnection,
        RequestStop,
        RequestData
    };
}