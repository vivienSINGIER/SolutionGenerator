#include "Command.h"

#include <format>
#include <iostream>

#include "core/Logger.h"

void Cmd::PrintPlan(std::vector<std::string> _plan)
{
    std::string out = "Command actions\n";
    for (size_t i = 0; i < _plan.size(); i++)
    {
        std::string header = std::to_string(i) + ".";
        out += std::format("  {:>3}  {}\n", header, _plan[i]);
    }
    out += "\n";
    
    Log::Log(LogType::PromptInfo, out);
}

bool Cmd::ApplyPrompt(int _changeCount)
{
    Log::Log(LogType::Prompt, "Apply these " + std::to_string(_changeCount) + " changes? [y/N]");
    
    char c;
    std::cin >> c;
    if (c != 'y' && c != 'Y')
        return false;
    return true;
}

void Cmd::LogChange(std::string _change)
{
    Log::Log(LogType::Info, _change + '\n');
}
