#include "ArgBag.h"

void ArgBag::Add(std::string _key, std::string _value)
{
    // if (!Exists(_key))
    //     m_values[_key] = std::vector<std::string>();
    
    m_values[_key].push_back(_value);
}

bool ArgBag::Exists(std::string const& _key) const
{
    return m_values.find(_key) != m_values.end();
}

std::string ArgBag::Get(std::string const& _key) const
{
    if (!Exists(_key))
        return "";
    
    return m_values.find(_key)->second.at(0);
}

bool ArgBag::Get(std::string const& _key, std::string& _value) const
{
    if (!Exists(_key))
        return false;
    
    _value = m_values.find(_key)->second.at(0);
    return true;
}

fs::path ArgBag::GetPath(std::string const& _key) const
{
    if (!Exists(_key))
        return "";
    
    return m_values.find(_key)->second.at(0);
}

bool ArgBag::GetFlag(std::string const& _key) const
{
    if (!Exists(_key))
        return false;
    
    std::string _value = Get(_key);
    if (_value == "true")
        return true;
    return false;
}

std::vector<std::string> ArgBag::GetList(std::string const& _key) const
{
    std::vector<std::string> result;
    if (!Exists(_key))
        return result;
    
    result = m_values.find(_key)->second;
    return result;
}
