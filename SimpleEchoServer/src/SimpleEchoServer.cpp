// SimpleEchoServer.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include "WinSockInitializer.h"
#include "TCPIPServer.h"
#include "UDPServer.h"
#include "Logger.h"
#include "command_line_helper.h"

#pragma comment(lib, "Ws2_32.lib")

//helpers to deal with command line arguments
static const char* s_help_str = 
"\nCommands (all are optional):\n \
-h - print this string help information\n\
-tcp port_number - to setup tcp/ip port number server is listening on\n\
default value is 12321\n\
-udp port_number - to setup udp port number server is listening on\n\
default value is 12321\n\
-log file_name - to specify log file\n\
if not specified the standard console output is used\n\
to shutdown server type quit in console";

class CServerCommandLineHelper
{
public:
    CServerCommandLineHelper() : parser( CServerCommandLineHelper::rules, 
                                        arg_constrain(0,0)
                                        )
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
    std::string GetTCPPortStr() const
    {
        const auto& values = parser.get_values_by_key( "-tcp" );
        if (values.size() == 0)
            return CServerCommandLineHelper::tcp_def_port;
        return values[0];
    };
    std::string GetUDPPortStr() const
    {
        const auto& values = parser.get_values_by_key( "-udp" );
        if (values.size() == 0)
            return CServerCommandLineHelper::udp_def_port;
        return values[0];
    };
    std::string GetLogFile() const
    {
        const auto& values = parser.get_values_by_key( "-log" );
        if (values.size() == 0)
            return std::string();
        return values[0];
    };

private:
    CCommandLineParser parser;
private:
    static const std::map<std::string, arg_constrain> rules;
    static const char* tcp_def_port;
    static const char* udp_def_port;
};
const std::map<std::string, arg_constrain> CServerCommandLineHelper::rules =
{ 
    { "-h", { 0, 0 } },
    { "-tcp", { 0, 1 } },
    { "-udp", { 0, 1 } },
    { "-log", { 0, 1 } }
};
const char* CServerCommandLineHelper::tcp_def_port = "12321";
const char* CServerCommandLineHelper::udp_def_port = "12321";
/*
-tcp 127013 -udp 11 
*/

int _tmain(int argc, const char* argv[])
{
   
    CServerCommandLineHelper command_hlp;
    if (!command_hlp.ParseArgs( size_t( argc-1 ), argv+1 ))
    {
        std::cout << "Invalid command arguments!" << std::endl;
        std::cout << s_help_str << std::endl;
        return -1;
    }
    if (command_hlp.IsHelpRequested())
    {
        std::cout << s_help_str << std::endl;
    }
    std::cout << "SimpleEchoServer arguments:" << std::endl;
    std::cout << "tcp_port: " << command_hlp.GetTCPPortStr() << std::endl;
    std::cout << "udp_port: " << command_hlp.GetUDPPortStr() << std::endl;
    std::cout << "Log infomation will be written to:"<<std::endl;
    if (command_hlp.GetLogFile().size() == 0)
        std::cout << "standard console output" << std::endl;
    else
        std::cout << "file: " << command_hlp.GetLogFile() << std::endl;

    Logger::SetOutput( command_hlp.GetLogFile().c_str() );
    
    std::cout << "SimpleEchoServer is starting..." << std::endl;
	WinSockInitializer wsInit;
    if (!wsInit.Init())
    {
        std::cout << "WinSock initialization: Failed" << std::endl;
        std::cout << "Shutdown server" << std::endl;
        return 1;
    }
    std::cout << "WinSock initialization: Success" << std::endl;

    size_t threads_number = std::thread::hardware_concurrency();
	ThreadWorkerPool pl(threads_number);
	pl.Init();
	UDPServer udp_srv;
	udp_srv.Init(command_hlp.GetUDPPortStr().c_str(), &pl);
	udp_srv.Run();
	TCPIPServer tcp_srv;
    tcp_srv.Init( command_hlp.GetTCPPortStr().c_str(), &pl );
	tcp_srv.Run();
	
    while (true)
    {
        std::string user_command;
        std::getline( std::cin, user_command, '\n' );
        if (user_command == "quit")
        {
            std::cout << "server is being shutdown..." << std::endl;
            udp_srv.Stop();
			tcp_srv.Stop();
            pl.Shutdown();
            std::cout << "server is shutdown!" << std::endl;
            return 0;
        }
    }
    return 0;
}

