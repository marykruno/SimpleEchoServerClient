#include "stdafx.h"
#include "UDPServer.h"

//function that represent operation performed by thread pool workers
//this is only one type of job for our thread pool in case of udp echo server
static void MakeEcho(socket_utils::socket_fd s, std::string echo_message, sockaddr_in to, int to_len)
{
	//we have reseived whole message, it's time to send back
	Logger::Log("Worker thread ", std::this_thread::get_id(), " received ", echo_message.size(), " bytes, message is:\n", echo_message);
	Logger::Log("Worker thread ", std::this_thread::get_id(), " sent echo message: ", echo_message);
	int send_res = socket_utils::send_data_to(s, echo_message.c_str(), (int)((echo_message).size()), (sockaddr*)&to, to_len);//sendto(s, echo_message.c_str(), (int)echo_message.size(), 0, (struct sockaddr*)&clientaddr, len);
	
	if (send_res == SOCKET_ERROR)
	{
		Logger::Log("Worker thread ", std::this_thread::get_id(), " send failed with error: ", WSAGetLastError());
		//		cout << "send failed: " << WSAGetLastError() << endl;
		return;
	}
}

void UDPServer::RunInternal()
{
	while (m_stop == false)
	{
		fd_set fd_read;
		FD_ZERO(&fd_read); //set to zero
		FD_SET(m_socket, &fd_read); //fdset
		struct timeval tv = { 1, 0 };
		fd_set fd_error = fd_read;

		int ret = select(0, &fd_read, NULL, &fd_error, &tv);

		if (ret == SOCKET_ERROR)
		{
			Logger::Log("UDPServer thread: ", "select method returned error: ", WSAGetLastError());
			break;
		}
		else if (FD_ISSET(m_socket, &fd_error))
		{
			Logger::Log("UDPServer thread: ", "main socket has error: ", WSAGetLastError());
		}

		if (ret != 0 && FD_ISSET(m_socket, &fd_read) )
		{
			////
			sockaddr_in client_addr;
			int client_addr_len = sizeof(client_addr);
			ZeroMemory(&client_addr, sizeof(sockaddr_in));
			std::string echo_message;
			int recv_res = socket_utils::receive_full_data_from(m_socket, echo_message, (sockaddr*)&client_addr, client_addr_len);
			if (recv_res == 0)
			{
				Logger::Log("Can not receive data, connection socket");
				continue;
			}
			else if (recv_res < 0)
			{
				Logger::Log("Can not receive data, recv failed, error code: ", WSAGetLastError());
			}
			else
			{
				m_pool->AddJob(MakeEcho, m_socket, echo_message, client_addr, client_addr_len);
			}

		}
	}

}