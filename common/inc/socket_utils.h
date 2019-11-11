#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <string>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

namespace socket_utils
{
    typedef SOCKET socket_fd;
    inline bool is_valid( socket_fd  s ) { return s != INVALID_SOCKET;  }
	enum  ECodeType
	{
		ALL_OK = 0,
		GETADDRINFO_FAILED = 1,
		CONNECTION_SERVER_FAILED = 2,
		BIND_SOCKET_FAILED = 3

	};
    enum TransportType
    {
        TCPIP = 0,
        UDP = 1
    };
    socket_fd create_socket( TransportType  type );
    ECodeType bind_socket( socket_fd s, TransportType type, const char* port );
    ECodeType connect_socket( socket_fd s, TransportType type, const char* server_name, const char* port );
    
    //wrapper for send (TCP socket)
    int send_data( socket_fd s, const char* buf, int len );
    
    //simple wrappers under socket functions recv and recvfrom
    //the problem is that we don't know what is data size
    //and may receive into buffer only part of data
    
    //for tcp socket
    int receive_data( socket_fd s, char* buf, int len);
	
    //for udp socket
    int receive_data_from(socket_fd s, char* buf, int len, sockaddr* from, int from_len);
    
    //these helper functions are intended for reading full packet
    //for tcp socket
    int receive_full_data( socket_fd s, std::string&  full_message );
	
    //for udp socket
    int receive_full_data_from(socket_fd s, std::string&  full_message, sockaddr* from, int from_len);
	
    //for udp socket
    int send_data_to(socket_fd s, const char* buf, int len, const sockaddr* to, int to_len);
    
    enum ESelectErrorCode
    {
        READ_READY = 0,
        TIME_EXPIRED = 1,
        ERROR_PRESENT = 2
    };
    //wrapper for select in case if we are waiting for read signal (or error signal) for just one socket
    ESelectErrorCode socket_check( socket_fd s, struct timeval* p_time_out );
};