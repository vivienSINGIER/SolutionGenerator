#include "SolutionManifest.h"

template <std::size_t N>
Error SolutionManifest::ValidateValues(std::string_view const(& _values)[N], std::string_view value, std::string_view _key)
{
    bool found = std::find(std::begin(_values), std::end(_values), value) != std::end(_values);    
    
    if (!found)
    {
        std::string out = "unvalid '" + std::string(_key) + "', please try ";
        for (std::string_view s : _values)
        {
            out += "'" + std::string(s) + "', ";
        }
        out.pop_back();
        out.pop_back();
        out += "\n";
        
        return Error::Validation(out);
    }
    return Error::Ok();
}