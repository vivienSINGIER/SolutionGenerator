#include "CommandTable.h"

namespace CommandTable
{
    
    static CommandSpec const g_commandTable[] = 
    {
        {
            CommandId::New, "", "new",
            /*positionals*/ {
                {"path", CommandValueType::Path, true, {}, "the location of the new solution"} ,
            },
            /*options*/{
                    { "name", '\0', CommandValueType::String, false, true, {},
                        "the name of the new solution" },
                    { "toolset", '\0', CommandValueType::String, false, false, {},
                        "the compiler toolset (default: v143)" },
                    { "cpp-standard", '\0', CommandValueType::String, false, false, {},
                        "the C++ language standard (default: 20)" },
                    { "platform", '\0', CommandValueType::List, false, false, {},
                        "the target platform(s), comma-separated (default: x64)" },
                    { "force", '\0', CommandValueType::Flag, false, false, {},
                        "overwrite an existing solution.json"}
            },
            "Creates a new solution at <path>"
        },

        {
            CommandId::ProjectAdd, "project", "add",
            /*positionals*/ {
                 {"name", CommandValueType::String, true, {}, "the name of the new project"}
            },
            /*options*/ {
                { "kind", '\0', CommandValueType::Enum, false, true, 
        { "console", "window", "staticlib", "dynamiclib"} , "the type of project" },
                { "depends-on", '\0', CommandValueType::String, true, false, {},
                    "Another project this one is linked to" },
                { "no-scaffold", '\0', CommandValueType::Flag, false, false, {}, 
                    "Write the manifest only, no folders or starter files created" }
            },
            "Creates a new project of type --kind linked to [--depend-on]"
        },
        
        {
            CommandId::ProjectRemove, "project", "remove",
            /*positionals*/{
                {"name", CommandValueType::String, true, {}, "the name of the project"}
            },
            /*options*/ {
                {
                    "cascade", '\0', CommandValueType::Flag, false, false, {},
                    "whether to delete the src & res file of the project" }
            },
            "Deletes the project"
        },
        
        {
            CommandId::ProjectList, "project", "list",{}, {},
            "Lists projects: name, kind, dependencies."
        },
        
        {
            CommandId::ProjectSet, "project", "set",
            /*positionals*/ {
                {"name", CommandValueType::String, true, {}, "the name of the project"},
                {"key", CommandValueType::String, true, {}, "the key to the targeted value"},
                {"value", CommandValueType::String, true, {}, "the new value"}
            }, {},
            "Sets a scalar field in the project's manifest config file"
        },
        
        {
            CommandId::DepAdd, "dep", "add",
            /*positionals*/ {
                {"project", CommandValueType::String, true, {}, "the name of the project"},
                {"dependency", CommandValueType::String, true, {}, "the name of the dependency"}
            }, {},
            "Adds a dependency to the project among the existing projects of the solution"
        },
        
        {
            CommandId::DepRemove, "dep", "remove",
            /*positionals*/ {
                {"project", CommandValueType::String, true, {}, "the name of the project"},
                {"dependency", CommandValueType::String, true, {}, "the name of the dependency"}
            }, {},
            "Removes a dependency off a project"
        },

        {
            CommandId::DepList, "dep", "list",
            /*positionals*/ {
                {"project", CommandValueType::String, true, {}, "the name of the project"}
            }, {},
            "Lists the project's current dependencies"
        },
        
        {
            CommandId::VendorAdd, "vendor", "add",
            /*positionals*/ {
                {"project", CommandValueType::String, true, {}, "the name of the project"},
                {"lib", CommandValueType::String, true, {}, "the name of the library"},
            },
            /*options*/ {
                { "include", '\0', CommandValueType::List, true, false, {},
                 "the additional include directories"},
                {"lib-dir", '\0', CommandValueType::List, true, false, {},
                "the additional library directories"},
                {"libs", '\0', CommandValueType::List, false, false, {},
                "the additional .libs dependencies"},
                {"dlls", '\0', CommandValueType::List, false, false, {},
                "the additional .dlls dependencies"},
                {"configuration", '\0', CommandValueType::String, false, false, {},
                "only add the lib-dir / libs / dlls for this configuration"},
            },
            "Adds additional dependencies to the project, or adds to an existing one"
        },
        
        {
            CommandId::VendorRemove, "vendor", "remove",
            /*positionals*/ {
                {"project", CommandValueType::String, true, {}, "the name of the project"},
                {"lib", CommandValueType::String, true, {}, "the name of the library"}
            }, {},
            "Removes an additional dependency off a project"
        },

        {
            CommandId::VendorList, "vendor", "list",
            /*positionals*/ {
                {"project", CommandValueType::String, true, {}, "the name of the project"}
            }, {},
            "Lists the project's vendor entries"
        },

        {
            CommandId::Set, "", "set",
            /*positionals*/ {
                {"key", CommandValueType::String, true, {}, "the key to the targeted value"},
                {"value", CommandValueType::String, true, {}, "the new value"}
            }, {},
            "Sets a scalar field in the solution's manifest config file"
        },

        {
            CommandId::SyncBats, "", "sync-bats", {}, {},
            "Regenerates the full .bat toolset"
        },
        
        {
            CommandId::Make, "", "make", {}, 
            /*options*/ {
                { "only", '\0', CommandValueType::List, false, false, {},
                "the list of projects to make"}
            },
            "Make the entire solution or the chosen projects"
        },
        
        {
            CommandId::Clean, "", "clean", {}, {},
            "Deletes all compiler output in the /ide folder"
        },
        
        {
            CommandId::Clear, "", "clear", {}, {},
            "Deletes the /ide folder"
        }
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
