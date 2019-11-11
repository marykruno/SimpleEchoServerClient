#include "stdafx.h"
#include "socket_utils.h"

namespace socket_utils
{
	socket_fd create_socket(TransportType type)
	{
		socket_fd ret_socket = INVALID_SOCKET;
		int af = AF_UNSPEC;
		int sock_type = 0, proto = 0;
		if (type == TCPIP)
		{
			sock_type = SOCK_STREAM;
			proto = IPPROTO_TCP;
		}
		else if (type == UDP)
		{
			sock_type = SOCK_DGRAM;
			proto = IPPROTO_UDP;
		}
		ret_socket = socket(af, sock_type, proto);
		return ret_socket;
	}

	ECodeType bind_socket(socket_fd s, TransportType type, const char* port)
	{
		struct addrinfo addr;
		ZeroMemory(&addr, sizeof(addr));
		addr.ai_family = AF_INET;
		if (type == TCPIP)
		{
			addr.ai_socktype = SOCK_STREAM;
			addr.ai_protocol = IPPROTO_TCP;
		}
		else if (type == UDP)
		{
			addr.ai_socktype = SOCK_DGRAM;
			addr.ai_protocol = IPPROTO_UDP;
		}
		addr.ai_flags = AI_PASSIVE;

		struct addrinfo* result = NULL;
		int iResult = getaddrinfo(NULL, port, &addr, &result);
		if (iResult != 0)
		{
            freeaddrinfo( result );
			return ECodeType(GETADDRINFO_FAILED);
		}
		iResult = bind(s, result->ai_addr, (int)(result->ai_addrlen));
		freeaddrinfo(result);
		if (iResult == SOCKET_ERROR)
		{
			return  ECodeType(CONNECTION_SERVER_FAILED);
		}
		return ECodeType(ALL_OK);
	}

    ECodeType connect_socket( socket_fd s, TransportType type, const char* server_name, const char* port )
    {
        struct addrinfo addr;
        ZeroMemory( &addr, sizeof( addr ) );
        addr.ai_family = AF_INET;
		if (type == TCPIP)
		{
			addr.ai_socktype = SOCK_STREAM;
			addr.ai_protocol = IPPROTO_TCP;
		}
		else if (type == UDP)
		{
			addr.ai_socktype = SOCK_DGRAM;
			addr.ai_protocol = IPPROTO_UDP;
		}
		struct addrinfo* result = NULL;
        int iResult = getaddrinfo( server_name, port, &addr, &result );
        if (iResult != 0)
        {
            freeaddrinfo( result );
            return ECodeType(GETADDRINFO_FAILED);
        }
        
        iResult = connect( s, result->ai_addr, (int)(result->ai_addrlen) );
        freeaddrinfo( result );
        if (iResult == SOCKET_ERROR)
        {
            return  ECodeType( CONNECTION_SERVER_FAILED );
        }
        return ECodeType(ALL_OK);
    }
	int send_data(socket_fd s, const char* buf, int len)
	{
		return send(s, buf, len, 0);
	}

	int send_data_to(socket_fd s, const char* buf, int len, const sockaddr* to, int to_len)
	{
		return sendto(s, buf, len, 0, to, to_len);
	}
	int receive_data(socket_fd s, char* buf, int len)
	{
		return recv(s, buf, len, 0);
	}
	int receive_data_from(socket_fd s, char* buf, int len, sockaddr* from, int from_len)
	{
		return recvfrom(s, buf, len, 0, from, &from_len);
	}
    ESelectErrorCode socket_check( socket_fd s, struct timeval* p_time_out )
    {
        ESelectErrorCode ret = TIME_EXPIRED;
        struct timeval immediate;
        immediate.tv_sec = 0;
        immediate.tv_usec = 0;
        struct timeval local = (p_time_out) ? (*p_time_out) : (immediate);
        fd_set fd_read, fd_error;
        FD_ZERO( &fd_read ); 
        FD_ZERO( &fd_error ); 
        FD_SET( s, &fd_read ); 
        FD_SET( s, &fd_error ); 
        int result = select( 0, &fd_read, NULL, &fd_error, &local );
        //
        if (ret == 0) //it means that we have read all message, exit from loop
            ret = TIME_EXPIRED;
        else if (ret == SOCKET_ERROR || FD_ISSET( s, &fd_error ))
            ret = ERROR_PRESENT;
        else if (FD_ISSET( s, &fd_read ))
            ret = READ_READY;
        return ret;
    }

    int receive_full_data( socket_fd ClientSocket, std::string&  full_message )
    {
        const size_t buffer_capacity = 512;
        char r_buffer[buffer_capacity + 1];
        int recv_res = socket_utils::receive_data( ClientSocket, r_buffer, buffer_capacity);
        while (true)
        {
            if (recv_res <= 0 )
            {
                break;
            }
            r_buffer[recv_res] = 0;
            full_message += r_buffer;
            if (recv_res < buffer_capacity) //we just received all message, exit from loop
                break;

            //we need to know is anything else to read?
            fd_set fd_read;
            FD_ZERO( &fd_read ); //set to zero
            FD_SET( ClientSocket, &fd_read ); // bind socket to fd_set structure
            struct timeval tv = { 0, 0 };
            int ret = select( 0, &fd_read, NULL, NULL, &tv );
            if (ret == 0) //it means that we have read all message, exit from loop
                break;
            if (ret == SOCKET_ERROR) //fatal error, close this socket
            {
				recv_res = -1;
                break;
            }
            //if we are here it means that some bytes are not read yet, so try to read
			recv_res = socket_utils::receive_data( ClientSocket, r_buffer, buffer_capacity);
            //recv(ClientSocket, r_buffer, capacity_buf, 0);
        }
        return recv_res;
    }

	int receive_full_data_from(socket_fd ClientSocket, std::string&  full_message, sockaddr* from, int from_len)
	{
		const size_t capacity_buf = 1024;
		char r_buffer[capacity_buf + 1];
		int recv_res = socket_utils::receive_data_from(ClientSocket, r_buffer, capacity_buf, from, from_len);
		while (true)
		{
			if (recv_res <= 0)
			{
				break;
			}
			r_buffer[recv_res] = 0;
			full_message += r_buffer;
			if (recv_res < capacity_buf) //we just received all message, exit from loop
				break;

			//we need to know is anything else to read?
			fd_set fd_read;
			FD_ZERO(&fd_read); //set to zero
			FD_SET(ClientSocket, &fd_read); // bind socket to fd_set structure
			struct timeval tv = { 0, 0 };
			int ret = select(0, &fd_read, NULL, NULL, &tv);
			if (ret == 0) //it means that we have read all message, exit from loop
				break;
			if (ret == SOCKET_ERROR) //fatal error, close this socket
			{
				recv_res = -1;
				break;
			}
			//if we are here it means that some bytes are not read yet, so try to read
			recv_res = socket_utils::receive_data_from(ClientSocket, r_buffer, capacity_buf, from, from_len);
			//recv(ClientSocket, r_buffer, capacity_buf, 0);
		}
		return recv_res;
	}
};