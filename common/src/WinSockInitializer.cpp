#include "stdafx.h"
#include "WinSockInitializer.h"

WinSockInitializer::~WinSockInitializer()
{
    if (m_bInit)
        WSACleanup();
}
bool WinSockInitializer::Init()
{
    bool ret_val = false;
    WSADATA wsaData;
    int result = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
    if (result == 0)
    {
        if (LOBYTE( wsaData.wVersion ) != 2 ||
            HIBYTE( wsaData.wVersion ) != 2)
        {
            std::cout << "WSAStartup: could not find a usable WinSock dll" << std::endl;
            WSACleanup();
			m_bInit = false;
        }
        else
            ret_val = true;
    }
    else
        std::cout << "WSAStartup failed, error code: " << result << std::endl;

    return ret_val;
}



