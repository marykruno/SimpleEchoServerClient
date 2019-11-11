#pragma once
#include <string>
#include <map>
#include <vector>


struct arg_constrain
{
    arg_constrain( size_t min_,size_t  max_ ) : min_num( min_ ), max_num(max_) {}
    arg_constrain() : min_num( 0 ), max_num( 0 ) {}
    size_t min_num;
    size_t max_num;
};
class CCommandLineParser
{
public:

    CCommandLineParser( const std::map<std::string, arg_constrain>& allowed_rules,
        arg_constrain free_arguments_constrain_ )
        :
        m_rules( allowed_rules ),
		m_FreeArgumentsConstrain( free_arguments_constrain_ )
    {}
public:
    bool parse( size_t arg_num, const char* arguments[] )
    {
        bool ret_val = internal_parse( arg_num, arguments );
        if (!ret_val)
            reset();
        return ret_val;
    }
    bool is_empty() const { return m_options.size() != 0; }
    bool is_key_presented( const char* key ) const
    {
        return m_options.find( key ) != m_options.end();
    }
    const std::vector<std::string>& get_values_by_key( const char* key ) const
    {
        auto fnd = m_options.find( key );
        if (fnd == m_options.end())
            return m_AlwaysEmpty;
        return fnd->second;
    }
    const std::vector<std::string>&  get_free_parameters() const
    {
        return m_NonKeyValues;
    }


private:
    bool internal_parse( size_t arg_num, const char* arguments[] );
    void reset()
    {
        m_options.clear();
		m_NonKeyValues.clear();
    }
    bool push_to_options( const char* key, const std::vector<std::string>& values )
    {
        auto it = m_options.find( key );
        //check if already exist, then return error
        if (it != m_options.end())
            return false;
        //check if key is allowed
        auto c_itr = m_rules.find( key );
        if (c_itr == m_rules.end())
            return false;
        //check if number of parameters satisfy conatrain
        if (values.size() < c_itr->second.min_num || values.size() > c_itr->second.max_num)
            return false;
        m_options.insert( std::make_pair( key, values ) );
        return true;
    }
    bool is_key_allowed( const char* key ) const
    {
        return (m_rules.find( key ) != m_rules.end());
    }
private:
    std::vector<std::string> m_AlwaysEmpty;
    
    std::map<std::string, arg_constrain> m_rules;
    std::map<std::string, std::vector<std::string>> m_options;
    //free parameters
    std::vector<std::string> m_NonKeyValues;
    arg_constrain m_FreeArgumentsConstrain;
};
