#include "Cli.h"

#include <iostream>

#include "ArgReader.h"
#include "CommandTable.h"
#include "Help.h"
#include "core/Logger.h"
#include "core/Version.h"

Error Cli::Parse(int _argc, char** _argv, Invocation& _out)
{
    if (_argc <= 1)
        return Error::Validation("not enough arguments");
    
    std::vector<std::string> tokens;
    
    tokens.reserve(_argc);
    for (int i = 1; i < _argc; i++)
    {
        if (_argv[i] == nullptr)
            return Error::Validation("empty argument detected");
        
        tokens.emplace_back(_argv[i]);
    }
    
    Parser parser(tokens);
    
    return parser.Run(_out);
}

Parser::Parser(std::vector<std::string> _tokens)
{
    m_tokens = _tokens;
}

Error Parser::Run(Invocation& _out)
{
    ExtractGlobals();

    _out.globalFlags = m_globalFlags;
    
    if (m_wantVersion)
    {
        Log::Log(LogType::Help, "Liftoff " + std::string(Version::string) + '\n');
        std::cout << "Liftoff " << Version::string << "\n";
        _out.done = true;
        return Error::Ok();
    }
    
    std::string token1;
    if (!m_tokens.empty())
        token1 = m_tokens[0];
    std::string token2;
    if (m_tokens.size() > 1)
        token2 = m_tokens[1];
    
    CommandSpec const* cSpec;
    Error resErr = ResolvePath(cSpec);
    
    if (m_tokens.empty() && resErr)
    {
        Help::General();
        _out.done = true;
        return m_error;
    }
    
    if (!resErr && m_wantHelp)
    {
        Help::Command(*cSpec);
        _out.done = true;
        return m_error;
    }
    else if (CommandTable::IsNounGroup(token1) == true)
    {
        if (token2.empty() || resErr)
        {
            Help::Group(token1);
            _out.done = true;
            return m_error;
        }
    }
    
    if (resErr)
    {
        AddError(resErr.Format());
        return m_error;
    }
    
    _out.cmdId = cSpec->id;
    
    BindArgs(*cSpec);
    
    if (m_error.details.empty() == false)
    {
        m_error.code = ErrorCode::Validation;
        m_error.message = "invalid '" + cSpec->Path() + "' invocation";
    }
    
    return m_error;
}

void Parser::ExtractGlobals()
{
    bool takesValue = false;
    GlobalId globalId = GlobalId::Config;
    int index = 0;
    std::vector<std::string> keptTokens;
    for (size_t i = 0; i < m_tokens.size(); i++)
    {
        std::string token = m_tokens[i];
        
        if (token.empty())
            AddError("missing value argument detected for flag '" + GlobalFlagsTable::All()[index].name + "'");
        
        if (takesValue)
        {
            if (IsValue(token))
            {
                ApplyGlobal(globalId, token);
                takesValue = false;
                continue;
            }
            AddError("missing value argument detected for flag '" + GlobalFlagsTable::All()[index].name + "'");
        }
        takesValue = false;
        
        bool isGlobal = IsGlobal(token, globalId);
        index = static_cast<int>(globalId);
        
        if (isGlobal)
        {
            takesValue = GlobalFlagsTable::All()[index].takesValue;
            if (!takesValue)
                ApplyGlobal(globalId, token);
        }
        else
            keptTokens.push_back(token);
    }
    
    if (takesValue)
        AddError("missing value argument detected for flag '" + GlobalFlagsTable::All()[index].name + "'");
    
    m_tokens = std::move(keptTokens);
}

Error Parser::ResolvePath(CommandSpec const*& _spec)
{
    size_t tokenCount = 0;
    
    Error e = CommandTable::Resolve(m_tokens, _spec, tokenCount);
    
    if (!e)
    {
        for (size_t i = 0; i < tokenCount; i++)
            m_tokens.erase(m_tokens.begin());
    }
    return e;
}

void Parser::BindArgs(CommandSpec const& _spec)
{
    ArgReader reader(m_tokens);
    
    bool optionsSeen = false;
    std::vector<std::string> positionalArgs;
    while (reader.IsEnd() == false)
    {
        bool isOption = reader.IsNextOption();
        std::string arg = reader.Next();
        
        if (isOption) optionsSeen = true;
        
        if (!isOption)
        {
            if (optionsSeen)
                m_error.details.push_back("unexpected argument '" + arg + "'");
            else
                positionalArgs.push_back(arg);
            continue;
        }
        
        std::string optionName = ExtractCommandName(arg);
        
        if (_spec.IsOption(optionName) == false)
        {
            m_error.details.push_back("unknown option '" + arg + "'");
            continue;
        }
        
        OptionSpec const* opt = _spec.FindOption(optionName);
        
        if (opt->repeatable == false && m_argBag.Exists(optionName))
        {
            m_error.details.push_back(arg + " given more than once");
        }
        
        if (opt->type == CommandValueType::Flag)
        {
            m_argBag.Add(optionName, "true");
            continue;
        }
        
        if (reader.IsNextOption() == true)
        {
            m_error.details.push_back(arg + " needs a value");
            continue;
        }
        
        std::string value = reader.Next();
        
        if (value.empty())
        {
            m_error.details.push_back("missing value for option " + arg);
        }
        
        if (opt->type == CommandValueType::Enum)
        {
            if (CheckEnumValue(arg, value, opt->enumValues) == false)
                continue;
        }
        
        if (opt->type == CommandValueType::List)
        {
            std::stringstream ss(value);
            std::string item;
            char delim = ',';
            while (std::getline(ss, item, delim))
            {
                m_argBag.Add(optionName, item);
            }
            continue;
        }
        
        m_argBag.Add(optionName, value);
    }
    
    for (size_t i = 0; i < _spec.positionals.size(); i++)
    {
        PositionalSpec const& pos = _spec.positionals[i];

        if (i >= positionalArgs.size())
        {
            if (pos.required)
                m_error.details.push_back("missing <" + pos.name + ">");
            continue;
        }

        if (pos.type == CommandValueType::Enum
            && CheckEnumValue("<" + pos.name + ">", positionalArgs[i], pos.enumValues) == false)
            continue;

        m_argBag.Add(pos.name, positionalArgs[i]);
    }
    
    if (positionalArgs.size() > _spec.positionals.size())
        for (size_t i = _spec.positionals.size(); i < positionalArgs.size(); i++)
            m_error.details.push_back("unexpected argument '" + positionalArgs[i] + "'");
    
    for (OptionSpec const& opt : _spec.options)
    {
        if (opt.required == false)
            continue;
        
        if (m_argBag.Exists(opt.name) == false)
            m_error.details.push_back("missing required option --" + opt.name + "");
    }
}

std::string Parser::ExtractCommandName(std::string_view _command)
{
    std::string result;
    for (size_t i = 0; i < _command.size(); i++)
    {
        if ( _command[i] != '-' )
        {
            result.append(_command.begin() + (long long)i, _command.end());
            break;
        }
    }
    
    return result;
}

void Parser::AddError(std::string _message)
{
    if (!m_error)
        m_error = Error::Validation(_message);
    else
        m_error.details.push_back(_message);
}

bool Parser::CheckEnumValue(std::string_view _label, std::string const& _value,
                            std::vector<std::string> const& _allowed)
{
    for (std::string const& candidate : _allowed)
        if (_value == candidate)
            return true;

    std::string errMsg = "unknown value, " + std::string(_label) + " must be one of <";
    for (size_t i = 0; i < _allowed.size(); i++)
        errMsg += (i == 0 ? "" : "|") + _allowed[i];
    errMsg += ">";
    m_error.details.push_back(errMsg);
    return false;
}

bool Parser::IsValue(std::string_view _command)
{
    return !_command.empty() && _command[0] != '-';
}

bool Parser::IsGlobal(std::string_view _command, GlobalId& _gId)
{
    for (size_t j = 0; j < GlobalFlagsTable::All().size(); j++)
    {
        bool isGlobalName = ( _command == "--" + GlobalFlagsTable::All()[j].name );
        bool isGlobalAlias = ( _command.size() == 2 && _command[0] == '-' && _command[1] == GlobalFlagsTable::All()[j].alias );
            
        bool isGlobal = isGlobalName || isGlobalAlias;
        if (isGlobal)
        {
            _gId = GlobalFlagsTable::All()[j].id;
            return true;
        }
    }
    return false;
}

void Parser::ApplyGlobal(GlobalId index, std::string_view value)
{
    switch (index)
    {
    case GlobalId::DryRun:
        m_globalFlags.dryRun = true; break;
    case GlobalId::Yes:
        m_globalFlags.yes = true; break;
    case GlobalId::Verbose:
        m_globalFlags.verbose = true; break;
    case GlobalId::Help:
        m_wantHelp = true; break;
    case GlobalId::Version:
        m_wantVersion = true; break;
    case GlobalId::Config:
        m_globalFlags.configPath = value;
        m_globalFlags.hasConfigPath = true;
        break;
    }
}
