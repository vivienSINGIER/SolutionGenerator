#ifndef COMMAND_SPECS_HPP
#define COMMAND_SPECS_HPP

#include <string>
#include <vector>

#include "CommandId.h"

enum class CommandValueType
{
    Flag, String, Path, Enum, List,
};

struct OptionSpec
{
    std::string name;
    char alias = '\0';
    
    CommandValueType type = CommandValueType::Flag;
    bool repeatable = false;
    bool required = false;
    std::vector<std::string> enumValues;
    
    std::string help;
};

struct PositionalSpec
{
    std::string name;
    CommandValueType type = CommandValueType::Flag;
    bool required = false;
    std::vector<std::string> enumValues;

    std::string help;
};

struct CommandSpec
{
    CommandId id = CommandId::None;
    std::string noun;
    std::string verb;
    
    std::vector<PositionalSpec> positionals;
    std::vector<OptionSpec> options;
    
    std::string summary;
    
    std::string Path() const
    {
        return noun.empty() ? verb : noun + " " + verb;
    }
    
    bool IsOption(std::string_view _option) const
    {
        for (const auto& option : options)
            if (option.name == _option)
                return true;
        return false;
    }
    
    bool IsRequired(std::string_view _option) const
    {
        for (const auto& option : options)
            if (option.name == _option)
                return option.required;
        return false;
    }
    
    OptionSpec const* FindOption(std::string_view _option) const
    {
        for (const auto& option : options)
            if (option.name == _option)
                return &option;
        return nullptr;
    }
};

#endif