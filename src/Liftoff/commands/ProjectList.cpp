#include "Command.h"
#include "core/Logger.h"

Error Cmd::ProjectList(Workspace& _ws, Invocation const& _inv)
{
    fs::path manifestPath;
    if (_inv.globalFlags.hasConfigPath)
        manifestPath = _inv.globalFlags.configPath;
    else if (Error e = Cmd::GetDefaultManifestPath(manifestPath))
        return e;
    
    if (Error e = _ws.Load(manifestPath))
        return e;
    
    std::string out = "Project List\n";
    for (auto const& p : _ws.GetProjects())
    {
        out += "  - " + p.name + ": " + p.kind + "\n";
        for (auto const& d : p.dependencies)
            out += "    -> " + d + "\n";
    }
    
    Log::Log(LogType::Help, out);
    
    return Error::Ok();
}
