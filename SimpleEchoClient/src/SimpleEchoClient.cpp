// SimpleSocketClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "WinSockInitializer.h"
#include "socket_utils.h"
#include "command_line_helper.h"
#include "client_echo_helper.h"

#pragma comment(lib, "Ws2_32.lib")

//helpers to deal with command line arguments
static const char* s_help_str = 
"\nCommands:\n \
optional:\n\
-h - print this string help information\n\
mandatory parameters:\n\
1. server name or ip address\n\
2. port number\n\
optional parameter:\n\
3. -udp to run client in udp mode\n\
if this key is not specified tcp mode will be used\n\
to shutdown client type quit";

class CEchoClientCommandLineHelper
{
public:
    CEchoClientCommandLineHelper() :
        parser( CEchoClientCommandLineHelper::rules, arg_constrain( 2, 2 ) )
        {}
public:
    bool ParseArgs( size_t arg_num, const char* arguments[] )
    {
        return parser.parse( arg_num, arguments );
    }
    bool IsHelpRequested() const
    {
        return parser.is_key_presented( "-h" );
    }
    socket_utils::TransportType GetMode() const
    {
        if (parser.is_key_presented( "-udp" ))
            return socket_utils::TransportType::UDP;
        return socket_utils::TransportType::TCPIP;
    }
    const std::string& GetServerName() const
    {
        return parser.get_free_parameters()[0];
    };
    const std::string& GetPortStr() const
    {
        return parser.get_free_parameters()[1];
    };
private:
    CCommandLineParser parser;
private:
    static const std::map<std::string, arg_constrain> rules;
};
const std::map<std::string, arg_constrain> CEchoClientCommandLineHelper::rules =
{
    { "-h", { 0, 0 } },
    { "-udp", { 0, 0 } }
};


int _tmain( int argc, const char* argv[] )
{

    std::cout << "Simple echo tcp/udp client" << std::endl;
    CEchoClientCommandLineHelper command_hlp;
    if (!command_hlp.ParseArgs( size_t( argc - 1 ), argv + 1 ))
    {
        std::cout << "Invalid command arguments!" << std::endl;
        std::cout << s_help_str << std::endl;
        return -1;
    }
    if (command_hlp.IsHelpRequested())
    {
        std::cout << s_help_str << std::endl;
    }
    std::cout << "Trying to connect with:" << std::endl;
    std::cout << "Server: " << command_hlp.GetServerName() << std::endl;
    std::cout << "Port: " << command_hlp.GetPortStr() << std::endl;
    const char* server_name = command_hlp.GetServerName().c_str();
    const char* port_number = command_hlp.GetPortStr().c_str();
    socket_utils::TransportType type_mode = command_hlp.GetMode();
    if (type_mode == socket_utils::TransportType::TCPIP)
    {
        std::cout << "Requested TCP IP mode for client" << std::endl;
    }
    else
    {
        std::cout << "Requested UDP mode for client" << std::endl;
    }

    // Initialize Winsock
    WinSockInitializer wsInit;
    if (!wsInit.Init())
    {
        std::cout << "WinSock initialization: Failed" << std::endl;
        std::cout << "Shutdown server" << std::endl;
        return 1;
    }

    ClientEchoHelper echo_helper;

    socket_utils::ECodeType connect_code = echo_helper.ConnectToServer( type_mode, server_name, port_number );

    if (connect_code != socket_utils::ECodeType::ALL_OK)
    {

        switch (connect_code)
        {
        case socket_utils::ECodeType::GETADDRINFO_FAILED:
            std::cout << "getaddrinfo failed: " << WSAGetLastError() << std::endl;
            break;
        case socket_utils::ECodeType::CONNECTION_SERVER_FAILED:
            std::cout << "Unable to connect to server! " << WSAGetLastError() << std::endl;
            break;

        default:
            ;
        };
        return  1;
    }

    std::cout << "Connection to server sucseeds!" << std::endl;
    std::cout << "Please, type your message:" << std::endl;
    while (true)
    {
        std::string message;
        int byte_sended = 0;
        std::getline( std::cin, message, '\n' );
        if (message == "quit")
            break;
        // Send an initial buffer
        int is_sended = echo_helper.SendToServer( message, byte_sended );
        if (!is_sended)
        {
            std::cout << "send failed: " << WSAGetLastError() << std::endl;
            break;
        }
        std::cout << "Bytes Sent: " << byte_sended << std::endl;
        
        std::string echo_message;
        size_t timeout = 60;
        ClientEchoHelper::EReceiveErrorType recv_err_code = echo_helper.ReceiveResponseFromServer( echo_message, timeout ); //we are going to wait not more than 1 minute
        if (recv_err_code == ClientEchoHelper::EReceiveErrorType::TIME_EXP )
        {
            std::cout << "no response from server, time out = 1 minute is expired" << std::endl;
            continue;
        }
        else if (recv_err_code == ClientEchoHelper::EReceiveErrorType::OP_ERROR)
        {
            std::cout << "recieving opretation failed, error code: "<<WSAGetLastError() << std::endl;
            continue;
        }
        else if (recv_err_code == ClientEchoHelper::EReceiveErrorType::CRITICAL_ERROR 
            || recv_err_code == ClientEchoHelper::EReceiveErrorType::SOCKET_CLOSED)
        {
            std::cout << "Error, code: " << WSAGetLastError() << std::endl;
            break;
        }
        std::cout << "Bytes received: " << echo_message.size() << std::endl;
        std::cout << "Echo: " << echo_message << std::endl;
 
	}
	return 0;
}

