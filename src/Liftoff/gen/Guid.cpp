#include "Guid.h"

std::string GUID::GenerateGuid(std::string const& _sName, std::string const& _pName)
{
    std::string data = "Liftoff project GUID | solution '" + _sName
                    + "' | project '" + _pName + "'";
    
    uint64_t hi = Fn1a64(data, 0xCBF29CE484222325ULL, "H");
    uint64_t lo = Fn1a64(data, 0x9E3779B97F4A7C15ULL, "L");

    // canonical 8-4-4-4-12: 32 nibbles, hi then lo, most significant first
    std::string const hex = "0123456789ABCDEF";
    std::string guid;
    guid.reserve(36);
    for (int i = 0; i < 32; ++i)
    {
        if (i == 8 || i == 12 || i == 16 || i == 20)
            guid += '-';

        uint64_t word  = (i < 16) ? hi : lo;
        int      shift = 60 - 4 * (i % 16);
        guid += hex[(word >> shift) & 0xF];
    }

    return guid;
}

uint64_t GUID::Fn1a64(std::string const& _data, uint64_t _start, std::string const& _prefix)
{
    constexpr uint64_t prime = 1099511628211;
    uint64_t hash = _start;
    std::string data = _prefix + " | " + _data;
    for (unsigned char c : data)
    {
        hash ^= c;
        hash *= prime;
    }
    return hash;
}
