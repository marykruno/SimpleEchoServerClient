#pragma once
#include <memory>
#include <thread>
#include <atomic>
#include <iostream>
#include "socket_utils.h"
#include "ThreadPool.h"
#include "Logger.h"
#include "ServerBase.h"



class UDPServer : public ServerProcessorBase
{
public:
	UDPServer() {}
    virtual ~UDPServer() {}
    bool Init( const char* ListenPort, ThreadWorkerPool* pool )
    {
        return ServerProcessorBase::Init( ListenPort, pool, socket_utils::TransportType::UDP );
    }
protected:
    //this is main udp server function executed by m_executionThread
	virtual void RunInternal() override;
};
