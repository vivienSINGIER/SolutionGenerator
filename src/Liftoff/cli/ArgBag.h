#ifndef ARG_BAG_H_DEFINED
#define ARG_BAG_H_DEFINED

#include <string>
#include <map>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

class ArgBag 
{
public:
    void Add(std::string _key, std::string _value);
    
    bool Exists(std::string const& _key) const;
    
    std::string Get(std::string const& _key)                       const;
    bool        Get(std::string const& _key, std::string& _value)  const;
    
    fs::path   GetPath(std::string const& _key) const;
    
    bool GetFlag(std::string const& _key) const;
    std::vector<std::string> GetList(std::string const& _key) const;
    
private:
    std::map<std::string, std::vector<std::string>> m_values;
};

#endif
