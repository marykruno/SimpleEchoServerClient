#include "stdafx.h"
#include "ServerBase.h"
#include <iostream>

bool ServerProcessorBase::IsRunning() const
{
    if (!m_executionThread.get())
        return false;
    return true;
}

void ServerProcessorBase::Stop()
{
    if (IsRunning())
    {
        m_stop = true;
        m_executionThread->join();
        m_executionThread.reset();
        closesocket( m_socket );
    }
}

void ServerProcessorBase::Run()
{
    m_stop = false;
    if (IsRunning())
        return;
    auto th_func = [this]( void )->void {this->RunInternal(); };
    m_executionThread = std::make_unique< std::thread >( std::thread( th_func ) );
}

bool ServerProcessorBase::Init( const char* ListenPort, ThreadWorkerPool* pool, socket_utils::TransportType server_type )
{
    m_pool = pool;
    //creation of main_socket;
    m_socket = socket_utils::create_socket( server_type );
    socket_utils::ECodeType bind_code = socket_utils::bind_socket( m_socket, server_type, ListenPort );

    if (bind_code != socket_utils::ECodeType::ALL_OK)
    {

        switch (bind_code)
        {
        case socket_utils::ECodeType::GETADDRINFO_FAILED:
            std::cout << "getaddrinfo failed: " << WSAGetLastError() << std::endl;
            break;
        case socket_utils::ECodeType::BIND_SOCKET_FAILED:
            std::cout << "Unable to bind listen socket! " << WSAGetLastError() << std::endl;
            break;

        default:
            ;
        };
        return  false;
    }
    if (server_type == socket_utils::TransportType::TCPIP)
    {
        int ret = listen( m_socket, 5 );
        if (ret == SOCKET_ERROR)
        {
            std::cout << "Listen was terminated with error: " << WSAGetLastError() << std::endl;
            return false;
        }
    }
    return true;
}
