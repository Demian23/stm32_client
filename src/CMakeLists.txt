add_executable(stm32_client main.cpp
        SerialPort.h
        SerialPort.cpp
        Channel.h
        Channel.cpp
        Protocol.h
        ErrnoException.h
        CommandProcesser.cpp
        CommandProcesser.h
        ProtocolMsg.h
        LocalStatusCode.h
        BinMsg.cpp
        BinMsg.h
        LoadMsg.h
)
