#pragma once
#include "socket_utils.h"

class ClientEchoHelper
{
public:
    ClientEchoHelper() : m_ConnectionSocket( INVALID_SOCKET ) {}
    ~ClientEchoHelper()
    {
        reset();
    }
public:
    void reset()
    {
        if (m_ConnectionSocket != INVALID_SOCKET)
            closesocket(m_ConnectionSocket);
		m_ConnectionSocket = INVALID_SOCKET;
    }
    bool good() const
    {
        return m_ConnectionSocket != INVALID_SOCKET;
    }
    socket_utils::ECodeType ConnectToServer( socket_utils::TransportType, const char* server_name, const char* port_num );
    bool SendToServer( const std::string& messsage, int& byte_sended);
    enum EReceiveErrorType
    {
        OK = 0,
        TIME_EXP = 1,
        CRITICAL_ERROR = 2,
        OP_ERROR = 3,
        SOCKET_CLOSED = 4
    };
    EReceiveErrorType ReceiveResponseFromServer( std::string& message, size_t timeout_seconds );
private:
    socket_utils::socket_fd m_ConnectionSocket;
};
