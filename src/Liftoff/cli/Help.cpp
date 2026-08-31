#include "Help.h"

#include <format>
#include <iostream>
#include <ranges>

#include "CommandTable.h"
#include "GlobalFlags.h"
#include "core/Version.h"

namespace Help
{
    void General()
    {
        std::string banner = std::format("Liftoff {} - MSBuild solution generator\n", Version::string);
        std::string usage = "usage: Liftoff <command> [<args>] [global options]\n";
        std::string instruction = "run 'Liftoff <command> --help' for details on a command";
        
        std::cout << banner << '\n' << usage << '\n';
        std::cout << FormatCommandList(CommandTable::All()) << '\n';
        std::cout << GlobalLabel() << '\n';
        std::cout << instruction << '\n';
    }

    void Group(std::string_view _group)
    {
        std::string usage = std::format("usage: Liftoff {} <subcommands> [<args>] [global options]\n", _group);
        std::string instruction = std::format("run 'Liftoff {} <subcommand> --help' for detail\n", _group);
        
        std::cout << usage << '\n';
        std::cout << SubcommandLabel(_group) << '\n';
        std::cout << GlobalLabel() << '\n';
        std::cout << instruction << '\n';
    }

    void Command(CommandSpec const& _spec)
    {
        std::string usage = "usage: " + UsageLine(_spec) + "\n";
        
        std::cout << usage << '\n' << _spec.summary << '\n' << '\n';
        std::cout << ArgumentLabel(_spec) << '\n';
        std::cout << OptionsLabel(_spec) << '\n';
        std::cout << GlobalLabel() << '\n';
    }

    std::string ValuePlaceholder(CommandValueType _type, std::span<std::string const> _enumValues)                                                                                                                             
    {                                                                                                                                                                                                                                
        switch (_type)                                                                                                                                                                                                               
        {                                                                                                                                                                                                                            
        case CommandValueType::Flag:   return "";                                                                                                                                                                                    
        case CommandValueType::String: return "<value>";                                                                                                                                                                             
        case CommandValueType::Path:   return "<path>";                                                                                                                                                                              
        case CommandValueType::List:   return "<value,...>";                                                                                                                                                                         
        case CommandValueType::Enum:                                                                                                                                                                                                 
        {                                                                                                                                                                                                                            
            std::string out = "<";                                                                                                                                                                                                   
            for (size_t i = 0; i < _enumValues.size(); ++i)                                                                                                                                                                          
                out += (i ? "|" : "") + _enumValues[i];                                                                                                                                                                              
            return out + ">";                                                                                                                                                                                                        
        }                                                                                                                                                                                                                            
        }                                                                                                                                                                                                                            
        return "";                                                                                                                                                                                                                   
    }

    std::string OptionSyntax(OptionSpec const& _option)
    {
        std::string core = "--" + _option.name;
        std::string placeholder = ValuePlaceholder(_option.type, _option.enumValues);
        if (!placeholder.empty())
            core += " " + placeholder;
        std::string out = _option.required ? core : "[" + core + "]";
        if (_option.repeatable)
            out += "...";
        return out;
    }

    std::string UsageLine(CommandSpec const& _command)
    {
        std::vector<std::string> parts = {
            "Liftoff", _command.Path()
        };
        
        for (PositionalSpec const& pos : _command.positionals)
        {
            std::string p = "<" + pos.name + ">";
            parts.push_back(pos.required ? p : "[" + p + "]");
        }
        
        for (OptionSpec const& opt : _command.options)
        {
            parts.push_back(OptionSyntax(opt));
        }
        
        std::string result;
        for (size_t i = 0; i < parts.size(); ++i)
        {
            result += parts[i];
            if (i != parts.size() - 1)
                result += " ";
        }
        return result;
    }

    std::string FormatCommandList(std::span<CommandSpec const> _commands)
    {
        size_t width = 0;
        for (CommandSpec const& command : _commands)
            width = std::max(width, command.Path().length());
        
        std::string out;
        for (CommandSpec const& command : _commands)
            out += std::format("  {:<{}}   {}\n", command.Path(), width, command.summary);
        return out;
    }

    std::string SubcommandLabel(std::string_view _noun)
    {
        size_t width = 0;
        for (CommandSpec const& c : CommandTable::All())
            if (c.noun == _noun)
                width = std::max(width, c.verb.length());

        std::string out = "Subcommands\n";
        for (CommandSpec const& c : CommandTable::All())
            if (c.noun == _noun)
                out += std::format("  {:<{}}   {}\n", c.verb, width, c.summary);
        return out;
    }

    std::string ArgumentLabel(CommandSpec const& _command)
    {
        size_t width = 0;
        for (PositionalSpec const& p : _command.positionals)
            width = std::max(width, p.name.length());
        
        std::string out = "Argument\n";
        for (PositionalSpec const& p : _command.positionals)
            out += std::format("  {:<{}}   {}\n", p.name, width, p.help);
        return out;
    }

    std::string OptionsLabel(CommandSpec const& _command)
    {
        size_t width = 0;
        size_t valueWidth = 0;
        
        for (OptionSpec const& o : _command.options)
        {
            width = std::max(width, o.name.length());
            valueWidth = std::max(valueWidth, OptionSyntax(o).length());
        }
        
        std::string out = "Options\n";
        for (OptionSpec const& o : _command.options)
            out += std::format("  {:<{}}   {:<{}}   {}\n", o.name, width, OptionSyntax(o), valueWidth, o.help );
        return out;
    }
    
    std::string GlobalLabel()
    {
        size_t width = 0;
        for (GlobalFlagDef const& g : GlobalFlagsTable::All())
            width = std::max(width, g.name.length());
        
        std::string out = "Global\n";
        for (GlobalFlagDef const& g : GlobalFlagsTable::All())
            out += std::format("  {}, {:<{}}   {}\n", ((g.alias == '\0') ? "  " : std::string("-") + g.alias) , g.name, width, g.help);
        return out;
    }
}
