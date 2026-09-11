#ifndef GUID_H_DEFINED
#define GUID_H_DEFINED
#include <cstdint>
#include <string>

namespace GUID
{
    std::string GenerateGuid(std::string const& _sName, std::string const& _pName);
    
    uint64_t Fn1a64(std::string const& _data, uint64_t _start, std::string const& _prefix);
}

#endif