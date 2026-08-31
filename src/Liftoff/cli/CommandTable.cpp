#include "CommandTable.h"

namespace CommandTable
{
    
    static CommandSpec const g_commandTable[] = 
    {
        {
            CommandId::New, "", "new",
            /*positionals*/ { 
                {"path", CommandValueType::Path, true, "the location of the new solution"} , 
            },
            /*options*/{
                    { "name", '\0', CommandValueType::String, false, true, {},
                        "the name of the new solution" },
                    { "toolset", '\0', CommandValueType::String, false, false, {},
                        "the toolset" }, // TODO Complete help
                    { "cpp-standard", '\0', CommandValueType::String, false, false, {},
                        "the version of the cpp standard library" },
                    { "override", '\0', CommandValueType::Flag, false, false, {},
                        "Whether we override the previous solution manifest if it exists"}
            },
            "Creates a new solution at a chosen path"
        },

        {
            CommandId::ProjectAdd, "project", "add",
            /*positionals*/ {
                 {"name", CommandValueType::String, true, "the name of the new project"} 
            },
            /*options*/ {
                { "kind", 0, CommandValueType::Enum, false, true, 
        { "console", "window", "staticlib", "dynamiclib"} , "the type of project" },
                { "depends-on", '\0', CommandValueType::String, true, false, {},
                    "Another project this one is linked to" },
                { "no-scaffold", '\0', CommandValueType::Flag, false, false, {}, 
                    "Write the manifest only, no folders or starter files created" }
            },
            "Creates a new project"
        },
    };

    std::string TopLevelNames()
    {
        std::string out;                                                                                                                                                                                                             
        std::string_view last;                                                                                                                                                                                                       
        for (CommandSpec const& spec : g_commandTable)                                                                                                                                                                                 
        {                                                                                                                                                                                                                            
            std::string_view const head = spec.noun.empty() ? spec.verb : spec.noun;                                                                                                                                                 
            if (head == last)                                                                                                                                                                                                        
                continue;                                                                                                                                                                                                            
            if (!out.empty())                                                                                                                                                                                                        
                out += ", ";                                                                                                                                                                                                         
            out += head;                                                                                                                                                                                                             
            last = head;                                                                                                                                                                                                             
        }                                                                                                                                                                                                                            
        return out;  
    }
    
    std::string SubCommandsOf(std::string_view _token)
    {
        std::string out;
        
        for (CommandSpec const& spec : g_commandTable)
        {
            if (spec.noun == _token)                                                                                                                                                                                                 
            {                                                                                                                                                                                                                        
                if (!out.empty())                                                                                                                                                                                                    
                    out += ", ";                                                                                                                                                                                                     
                out += spec.verb;                                                                                                                                                                                                    
            }              
        }
        return out;
    }
    
    std::span<CommandSpec const> All()                                                                                                                                                                                               
    {                                                                                                                                                                                                                                
        return g_commandTable;                                                                                                                                                                                                         
    }        
    
    Error Resolve(std::span<std::string const> _tokens, CommandSpec const*& _out, size_t& _consumed)
    {
        _consumed = 0;
        _out = nullptr;
        
        if (_tokens.empty())
            return Error::Validation("no commands given. commands: " + TopLevelNames());
        
        std::string const& first = _tokens[0];
        
        for (CommandSpec const& spec : g_commandTable)
        {
            if (spec.noun.empty() && spec.verb == first)
            {
                _out = &spec;
                _consumed = 1;
                return Error::Ok();
            }
        }
        
        if (IsNounGroup(first))
        {
            if (_tokens.size() < 2)
                return Error::Validation("'" + first + "' needs a subcommand: " + SubCommandsOf(first));
            
            std::string const& second = _tokens[1];
            for (CommandSpec const& spec : g_commandTable)
            {
                if (spec.noun == first && spec.verb == second )
                {
                    _out = &spec;
                    _consumed = 2;
                    return Error::Ok();
                }
            }
            
            return Error::Validation("unknown '" + first + "' subcommand '" + second + "'. try: " + SubCommandsOf(first));
        }
        
        return Error::Validation("unknown command '" + first + "'. try: " + TopLevelNames());
    }

    bool IsNounGroup(std::string_view _token)
    {
        for (CommandSpec const& spec : g_commandTable)
        {
            if (spec.noun == _token)
                return true;
        }
        
        return false;
    }
}
