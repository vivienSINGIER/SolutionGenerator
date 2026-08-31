#ifndef CLI_H_DEFINED
#define CLI_H_DEFINED

#include <string>
#include <vector>

#include "CommandSpecs.hpp"
#include "Invocation.h"
#include "core/Error.h"

namespace Cli
{
    Error Parse(int _argc, char** _argv, Invocation& _out);
}

class Parser
{
public:
    Parser(std::vector<std::string> _tokens);
    Error Run(Invocation& _out);
    
private:
    std::string         ExtractCommandName(std::string_view _command);
    void                AddError(std::string _message);
    bool                CheckEnumValue(std::string_view _label, std::string const& _value,
                                       std::vector<std::string> const& _allowed);

    void                ExtractGlobals();
    Error               ResolvePath(CommandSpec const*& _spec);
    void                BindArgs(CommandSpec const& _spec);
    
    bool                IsValue(std::string_view _command);
    bool                IsGlobal(std::string_view _command, GlobalId& _gIndex);
    void                ApplyGlobal(GlobalId index, std::string_view value);

    std::vector<std::string>    m_tokens;
    GlobalFlags                 m_globalFlags;
    bool                        m_wantHelp = false;
    bool                        m_wantVersion = false;
    ArgBag                      m_argBag;
    Error                       m_error;
};

#endif
