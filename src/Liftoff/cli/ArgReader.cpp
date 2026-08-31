#include "ArgReader.h"

ArgReader::ArgReader(std::vector<std::string> const& _args)
{
    m_args = &_args;
    m_pos = 0;
}

std::string const ArgReader::Peek() const
{
    if (IsEnd())
        return "";
    
    return (*m_args)[m_pos];
}

std::string ArgReader::Next()
{
    if (IsEnd())
        return "";
    
    std::string value = (*m_args)[m_pos];
    m_pos++;
    
    return value;
}

bool ArgReader::IsNextOption() const
{
    if (IsEnd())
        return false;
    
    std::string const& arg = Peek();
    if (arg.length() < 3)
        return false;
    
    if (arg[0] != '-' || arg[1] != '-')
        return false;
    
    return true;
}

bool ArgReader::IsEnd() const
{
    return m_pos >= m_args->size();
}

size_t ArgReader::GetPos() const
{
    return m_pos;
}


