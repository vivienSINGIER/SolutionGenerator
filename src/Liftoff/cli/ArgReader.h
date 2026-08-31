#ifndef ARGREADER_H_DEFINED
#define ARGREADER_H_DEFINED

#include "CommandSpecs.hpp"
#include "CommandTable.h"

class ArgReader
{
public:
    ArgReader(std::vector<std::string> const& _args);
    
    std::string const   Peek() const;
    std::string         Next();
    bool                IsNextOption() const;
    bool                IsEnd() const;
    size_t              GetPos() const;
    
private:
    std::vector<std::string> const* m_args;
    size_t                          m_pos;
};

#endif
