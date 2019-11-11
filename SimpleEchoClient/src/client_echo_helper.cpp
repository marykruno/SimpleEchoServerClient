#include "stdafx.h"
#include "client_echo_helper.h"
using namespace socket_utils;

socket_utils::ECodeType ClientEchoHelper::ConnectToServer(
    socket_utils::TransportType type,
    const char* server_name,
    const char* port_num )
{
    reset();
	m_ConnectionSocket = create_socket( type );
    return connect_socket(m_ConnectionSocket, type, server_name, port_num );
}
bool ClientEchoHelper::SendToServer( const std::string& message, int& byte_sended )
{
    if (m_ConnectionSocket == INVALID_SOCKET)
        return false;
    int send_result = send_data(m_ConnectionSocket, message.c_str(), (int)(message.size()) );
    byte_sended = send_result;
    return send_result != SOCKET_ERROR;
}

ClientEchoHelper::EReceiveErrorType ClientEchoHelper::ReceiveResponseFromServer( 
                                                      std::string& message, 
                                                      size_t timeout_seconds )
{
    if (m_ConnectionSocket == INVALID_SOCKET)
        return ClientEchoHelper::EReceiveErrorType::CRITICAL_ERROR;
    struct timeval timeout = { (long)timeout_seconds, 0 };
    ESelectErrorCode select_code = socket_utils::socket_check(m_ConnectionSocket, &timeout );
    if (select_code == ESelectErrorCode::TIME_EXPIRED)
    {
        return ClientEchoHelper::EReceiveErrorType::TIME_EXP;
    } 
    else if (select_code == ESelectErrorCode::ERROR_PRESENT)
    {
        return ClientEchoHelper::EReceiveErrorType::SOCKET_CLOSED;
    }
    int recv_result = receive_full_data(m_ConnectionSocket, message );
    if (recv_result == 0)
    {
        return ClientEchoHelper::EReceiveErrorType::SOCKET_CLOSED;
    }
    else if (recv_result < 0)
    {
        return ClientEchoHelper::EReceiveErrorType::OP_ERROR;
    }
    return ClientEchoHelper::EReceiveErrorType::OK;
}


