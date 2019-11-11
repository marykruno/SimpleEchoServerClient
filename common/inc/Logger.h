#pragma once
#include <mutex>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <ctime>

//helper static template functions:
template < typename T, typename ... Args> static void print_to_stream( std::ostream& os, T t, Args ... args )
{
    os << t;
    print_to_stream( os, std::forward<Args>( args )... );
}
template <typename T> static void print_to_stream( std::ostream& os, T t )
{
    os << t;
}


class Logger
{
public:

    static bool SetOutput( const char* file_name );
    static void Close() 
    { 
        std::lock_guard<std::mutex> lock( m_mutex );
        Logger::CloseInternal(); 
    }
    template< typename ... Args>
    static void Log( Args ... args )
    {
        //at the begin will print timestamp
        std::chrono::time_point<std::chrono::system_clock> cur_time = std::chrono::system_clock::now();
        std::time_t cur_time_t = std::chrono::system_clock::to_time_t(cur_time);
        char timestamp_buf[64];
        ctime_s(timestamp_buf, 64, &cur_time_t);
        
        //do as much as possible without mutex protection
        //probably, it slightly increases performance
        std::ostringstream local;
        print_to_stream( local, "Timestamp ", timestamp_buf, std::forward<Args>( args )... );
        
        //try to gain protection for only simple log string operation
        std::lock_guard<std::mutex> lock( m_mutex );
        (*Logger::m_logOut) << local.str() << '\n';
        (*Logger::m_logOut).flush();
    }
private:
    static std::mutex m_mutex;
    static std::ostream* m_logOut;
	static std::ofstream m_FileLogOut;;
    static void CloseInternal();
};