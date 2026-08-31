#ifndef HELP_H_DEFINED
#define HELP_H_DEFINED
#include <span>

#include "CommandSpecs.h"

namespace  Help
{   
    void General();
    void Group(std::string_view _group);
    void Command(CommandSpec const& _spec);
    
    std::string ValuePlaceholder(CommandValueType _type, std::span<std::string const> _enumValues);
    std::string OptionSyntax(OptionSpec const& _option);
    std::string UsageLine(CommandSpec const& _command);
    std::string FormatCommandList(std::span<CommandSpec const> _commands);
    std::string SubcommandLabel(std::string_view _noun);

    std::string ArgumentLabel(CommandSpec const& _command);
    std::string OptionsLabel(CommandSpec const& _command);
    std::string GlobalLabel();
};

#endif
