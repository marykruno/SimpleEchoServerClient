#include "stdafx.h"
#include "Logger.h"


std::mutex  Logger::m_mutex;
std::ostream* Logger::m_logOut = &(std::cout);
std::ofstream Logger::m_FileLogOut;
bool Logger::SetOutput( const char* file_name )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    Logger::CloseInternal();
    Logger::m_logOut = &(std::cout);
    if (file_name == nullptr)
        return true;
    Logger::m_FileLogOut.open( file_name, std::ios_base::out | std::ios_base::app );
    if (!Logger::m_FileLogOut.is_open() || !Logger::m_FileLogOut.good())
    {
        Logger::m_FileLogOut.close();
        return false;
    }
    Logger::m_logOut = &(Logger::m_FileLogOut);
    return true;
}
void Logger::CloseInternal()
{
    (*Logger::m_logOut).flush();
    Logger::m_logOut = &(std::cout);
    if (Logger::m_FileLogOut.is_open())
        Logger::m_FileLogOut.close();
}


