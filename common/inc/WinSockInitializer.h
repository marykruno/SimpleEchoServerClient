#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include "WinSockInitializer.h"


class WinSockInitializer
{
public:
    WinSockInitializer() : m_bInit( false ) {}
    ~WinSockInitializer();
    bool Init();
private:
    bool m_bInit;
};

