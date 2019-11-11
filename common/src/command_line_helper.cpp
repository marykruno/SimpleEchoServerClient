#include "stdafx.h"
#include "command_line_helper.h"
#include <utility>
namespace internal_utilities
{
    static bool is_key_string( const char* arg )
    {
        return arg[0] == '-';
    }
    struct ParseState
    {
        ParseState() : m_KeyStr( nullptr ), m_MaxAllowedArguments( 0 ) {}
        bool empty() const { return m_KeyStr == nullptr; }
        bool full() const { return m_MaxAllowedArguments == m_values.size();  }
        void reset()
        {
			m_KeyStr = nullptr;
            m_values.clear();
			m_MaxAllowedArguments = 0;
        }
        const char* m_KeyStr;
        std::vector<std::string> m_values;
        size_t m_MaxAllowedArguments;
    };
}
using namespace internal_utilities;
bool CCommandLineParser::internal_parse( size_t arg_num, const char* arguments[] )
{
    reset();

    //to keep state related to key
    ParseState p_state;
    //const char* key_str = nullptr;
    //std::vector<std::string> values;
    //size_t max_allowed_max_arguments = 0;

    for (size_t i = 0; i < arg_num; ++i)
    {
        if (::is_key_string( arguments[i] ))
        {
            
            //at first we need to store option if p_state non empty
            //since we read new key
            if (!p_state.empty())
            {
                if (!push_to_options( p_state.m_KeyStr, p_state.m_values )) //invalid command line keya are repeated
                    return false;
            }
            p_state.reset();

            //check if key is allowed
            auto c_itr = m_rules.find( arguments[i] );
            if (c_itr == m_rules.end())
                return false;

            p_state.m_KeyStr = arguments[i];
            p_state.m_MaxAllowedArguments = c_itr->second.max_num;

        }
        else //we detect that argument is value
        {
            if (!p_state.empty() && !p_state.full()) //since state is non empty we need add value to it
            {
                p_state.m_values.emplace_back( arguments[i] );
                //if (max_allowed_max_arguments == values.size()) //need to store
                if (p_state.full()) //need to store it in options
                {
                    if (!push_to_options( p_state.m_KeyStr, p_state.m_values )) //invalid command line keya are repeated
                        return false;
                    p_state.reset();
                }
            }
            else //it means arguments[i] should be added to non_key_values
            {
                if (m_NonKeyValues.size() == m_FreeArgumentsConstrain.max_num) //invalid command line 
                    return false;
				m_NonKeyValues.emplace_back( arguments[i] );
            }
        }
    }
    //if we here we need to check if p_state is not empty to put into options
    if (!p_state.empty())
    {
        if (!push_to_options( p_state.m_KeyStr, p_state.m_values )) //invalid command line keya are repeated
            return false;
    }
    
    //check that free arguments satisfies constrain
    if (m_NonKeyValues.size() > m_FreeArgumentsConstrain.max_num || m_NonKeyValues.size() < m_FreeArgumentsConstrain.min_num)
        return false;
    
    //check that mandatory keys (having non zero minimum required arguments are presented
    for (const auto& r : m_rules)
    {
        if (r.second.min_num != 0) //search for if this is presented in options
        {
            auto it = m_options.find( r.first );
            if (it == m_options.end())
                return false;
        }
    }
    
    return true;
}

