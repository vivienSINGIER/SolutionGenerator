#include "Command.h"

#include <format>
#include <iostream>

#include "core/Logger.h"

void Cmd::PrintPlan(std::vector<std::string> _plan)
{
    std::string out = "Command actions\n";
    for (size_t i = 0; i < _plan.size(); i++)
    {
        std::string header = std::to_string(i + 1) + ".";
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

void Cmd::Verbose(std::string _msg, bool _b)
{
    if (_b)
        Cmd::LogChange(_msg);
}

Error Cmd::GetDefaultManifestPath(std::filesystem::path& _out)
{
    fs::path start;
    FileHelper::ExecutablePath(start);
        
    fs::path out;
    if (Error e = FileHelper::FindUpwards(start, ".liftoff", out))
        return e;
    
    std::string solutionName;
    if (Error e = FileHelper::ReadFile(out / ".liftoff", solutionName))
        return e;
    
    out = out / "config" / (solutionName + ".json");
    if (FileHelper::IsFile(out) == false)
        return Error::Io("unable to find solution manifest at '" + out.string() + "'");
    
    _out = out;
    return Error::Ok();
}

Error Cmd::GetRootPath(std::filesystem::path& _out)
{
    fs::path start;
    FileHelper::ExecutablePath(start);
        
    fs::path out;
    if (Error e = FileHelper::FindUpwards(start, ".liftoff", out))
    {
        Error err = Error::Io("unable to find solution root");
        err.details.push_back(e.Format());
        return err;
    }
    
    _out = out;
    return Error::Ok();
}
