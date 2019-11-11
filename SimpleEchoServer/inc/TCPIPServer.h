#pragma once
#include <set>
#include <map>
#include <memory>
#include <thread>
#include <atomic>
#include <iostream>
#include "socket_utils.h"
#include "ThreadPool.h"
#include "Logger.h"
#include "ServerBase.h"


//helper class that stores the current tcp connections
//also for each tcp connection we support the state
//bool in_use that indicates that job related to this socket is 
//already added to thread pool, when job is completed worker thread 
//set this flag to false
//this is need to serialize all echo server operations performed 
//for concrete tcp connection
class LiveTCPConnections
{
public:
	~LiveTCPConnections()
	{
		RemoveAllSockets();
	}
	void FillWSAPollData(std::vector<WSAPOLLFD>& info) const;
	bool LockUnlockSocket(socket_utils::socket_fd s, bool in_use);
	bool AddSocket(socket_utils::socket_fd id);
	bool RemoveSocket(socket_utils::socket_fd id);
	void RemoveAllSockets();
	size_t size() const;
private:
	mutable std::mutex m_mutex;
	std::map<socket_utils::socket_fd, bool> m_liveConnections;
};


class TCPIPServer : public ServerProcessorBase
{
public:
	TCPIPServer() {};
    virtual ~TCPIPServer() {}
    bool Init( const char* ListenPort, ThreadWorkerPool* pool )
    {
        return ServerProcessorBase::Init( ListenPort, pool, socket_utils::TransportType::TCPIP );
    }
protected:
    //main server thread function that is executed by m_executionThread
	virtual void RunInternal() override;
private:
	LiveTCPConnections m_liveConnections;
};

