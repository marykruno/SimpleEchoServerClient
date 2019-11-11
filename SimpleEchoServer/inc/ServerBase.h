#pragma once
#include <memory>
#include <thread>
#include <atomic>
#include "socket_utils.h"
#include "ThreadPool.h"


//base class for TCP/UDPServers
class ServerProcessorBase
{
public:
    ServerProcessorBase() : m_socket( INVALID_SOCKET ) {};
    virtual ~ServerProcessorBase()
    {
        if (socket_utils::is_valid( m_socket ))
            closesocket( m_socket );
    }
    bool Init( const char* ListenPort, ThreadWorkerPool* pool, socket_utils::TransportType server_type );
    bool IsRunning() const;
    void Stop();
    void Run();
protected:
    //main server thread function that is executed by m_executionThread
    //the implementation is different for tcp and udp
    //overrided by tcp and udp servers
    virtual void RunInternal() = 0;

protected:
    std::unique_ptr< std::thread > m_executionThread;
    std::atomic<bool> m_stop;
    socket_utils::socket_fd m_socket;
    ThreadWorkerPool* m_pool;
};