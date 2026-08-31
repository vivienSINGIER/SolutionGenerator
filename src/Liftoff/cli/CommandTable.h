#ifndef COMMANDTABLE_H_DEFINED
#define COMMANDTABLE_H_DEFINED


#include <span>
#include <string_view>

#include "CommandSpecs.hpp"
#include "core/Error.h"

namespace CommandTable
{
    std::span<CommandSpec const> All();
    
    Error Resolve(std::span<std::string const> _tokens, CommandSpec const*& _out, size_t& _consumed);
    
    bool IsNounGroup(std::string_view _token);
}

#endif
