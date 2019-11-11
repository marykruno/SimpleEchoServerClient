#include "stdafx.h"
#include "TCPIPServer.h"

//helper functions
inline bool IsErrorPresented( const WSAPOLLFD& wpoll )
{
    return (wpoll.revents & POLLERR) || (wpoll.revents & POLLHUP) || (wpoll.revents & POLLNVAL);
}
inline bool IsReadyForRead( const WSAPOLLFD& wpoll )
{
    return (wpoll.revents & (POLLIN | POLLPRI)) != 0;
}


//class LiveTCPConnections implementation
void LiveTCPConnections::FillWSAPollData(std::vector<WSAPOLLFD>& info) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	for (const auto& s : m_liveConnections)
	{
		if (s.second == false)
		{
			WSAPOLLFD add_item;
			add_item.fd = s.first;
			add_item.events = POLLRDBAND | POLLRDNORM;
			add_item.revents = 0;
			info.push_back(add_item);
		}
	}

}

bool LiveTCPConnections::LockUnlockSocket(socket_utils::socket_fd s, bool in_use)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_liveConnections.find(s);
	if (it == m_liveConnections.end())
		return false;
	it->second = in_use;
	return true;
}

bool LiveTCPConnections::AddSocket(socket_utils::socket_fd id)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	//deal with set
	if (m_liveConnections.find(id) == m_liveConnections.end())
	{
		m_liveConnections.insert(std::make_pair(id, false));
		return true;
	}
	return false;
}

bool LiveTCPConnections::RemoveSocket(socket_utils::socket_fd id)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_liveConnections.find(id);
	if (it != m_liveConnections.end())
	{
		m_liveConnections.erase(it);
		return true;
	}
	return false;
}

void LiveTCPConnections::RemoveAllSockets()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto& s : m_liveConnections)
		closesocket(s.first);
	m_liveConnections.clear();
}

size_t LiveTCPConnections::size() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_liveConnections.size();
}

//Connection processing function
//this is only one type of job for our thread pool in case of tcp echo server
static void ConnectionProcessing(LiveTCPConnections& tcp_connections, SOCKET ClientSocket)
{
    std::string received_message;
    int recv_res = socket_utils::receive_full_data(ClientSocket, received_message);
    bool need_to_close_connection = true;
    if (recv_res == 0)
    {
        Logger::Log( "Worker thread ", std::this_thread::get_id(), " detects connection ", ClientSocket, "is  closed" );
    }
    else if (recv_res < 0)
    {
        Logger::Log( "Worker thread ", std::this_thread::get_id(), " recv faled for connection ", ClientSocket, "error code: ", WSAGetLastError() );
    }
    else
        need_to_close_connection = false;
    
    if (need_to_close_connection)
	{
		tcp_connections.RemoveSocket(ClientSocket);
		closesocket(ClientSocket);
        return;
	}
	//we have reseived whole message, it's time to send back
	Logger::Log("Worker thread ", std::this_thread::get_id(), " received ", received_message.size(), " bytes from connection: ", ClientSocket, " message is:\n", received_message.c_str());

	const char* response_msg = received_message.c_str();
	Logger::Log("Worker thread ", std::this_thread::get_id(), " sent echo message: ", response_msg);
	int send_res = socket_utils::send_data(ClientSocket, response_msg, (int)(received_message.size()));
		//send(ClientSocket, response_msg, (int)(received_message.size()), 0);
	if (send_res == SOCKET_ERROR)
	{
		//std::cout << "send failed: " << WSAGetLastError() << std::endl;
		Logger::Log("Worker thread ", std::this_thread::get_id(), " send falied to connection ", ClientSocket, "error code: ", WSAGetLastError());
		tcp_connections.RemoveSocket(ClientSocket);
		closesocket(ClientSocket);
		return;
	}
	tcp_connections.LockUnlockSocket(ClientSocket, false);
}

///TCPIPServer implementation

static const int s_wsapoll_timeout = 10;

void TCPIPServer::RunInternal()
{
    std::vector<WSAPOLLFD> wsa_poll_info;
    WSAPOLLFD main_item;
    main_item.fd = m_socket;
    main_item.events = POLLRDBAND | POLLRDNORM;
    main_item.revents = 0;
    wsa_poll_info.push_back( main_item );
    while (m_stop == false)
	{
        //content of info must be filled from zero each cycle iteration!
        //except of the fisrt element that represent the main (listen) socket
        //however, revents field of this fisrt element must be set to zero
        //at the start of each cycle iteration
        wsa_poll_info.resize( 1 );
        wsa_poll_info[0].revents = 0;
        
        //to avoid multiple reallocation when FillWSAPollData adds new element
        //call reserve to allocate requered capacity at once
        wsa_poll_info.reserve( m_liveConnections.size() + 1 );
        m_liveConnections.FillWSAPollData( wsa_poll_info );
        
        int result = WSAPoll( &wsa_poll_info[0], (ULONG)(wsa_poll_info.size()), s_wsapoll_timeout );
		
        if (result == SOCKET_ERROR)
		{
			Logger::Log("TCPServer thread: ", "WSAPoll failed, error code: ", WSAGetLastError());
			continue;
		}
		if (result == 0)
			continue;
		
        //deal with first element that is related to main socket
        const WSAPOLLFD& main_socket_info = wsa_poll_info[0];
        if (main_socket_info.revents != 0)
            --result;
        if (IsErrorPresented( main_socket_info ))
        {
            Logger::Log( "TCPServer thread: ", "main socket has error: ", WSAGetLastError() );
        }
        else if (IsReadyForRead( main_socket_info )) //indicates that it's time to call accept
        {
            //we don't put accept task to ThreadPool
            //instead accept action is performed in the main server loop, here:
            SOCKET ClientSocket = accept( m_socket, NULL, NULL );
            if (ClientSocket == INVALID_SOCKET)
            {
                Logger::Log( "TCPServer thread: ", "accept method failed,  error: ", WSAGetLastError() );
            }
            else
            {
                m_liveConnections.AddSocket( ClientSocket );
                Logger::Log( "TCPServer thread: ", "new connection accepted: ", ClientSocket );
            }

        }
        
        //check live connections
		auto itr = wsa_poll_info.begin() + 1;
		auto end = wsa_poll_info.end();
		for (; itr != end && result != 0; ++itr)
		{
            if ((*itr).revents != 0)
                --result;
            if (IsErrorPresented(*itr))
			{
				m_liveConnections.RemoveSocket((*itr).fd);
				closesocket((*itr).fd);
			}
			else  if (IsReadyForRead(*itr))
			{
				m_liveConnections.LockUnlockSocket((*itr).fd, true);
				m_pool->AddJob(ConnectionProcessing, std::ref(m_liveConnections), (*itr).fd);
			}
		}

	}
}