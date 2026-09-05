#include "DepList.h"

#include "Command.h"
#include "core/Logger.h"

Error Cmd::DepList(Workspace& _ws, Invocation const& _inv)
{
    ArgBag const& bag = _inv.argBag;
    
    if (!bag.Exists("project"))
        return Error::Validation("missing required 'project' argument");
    
    std::string project = bag.Get("project");
    
    fs::path manifestPath;
    if (_inv.globalFlags.hasConfigPath)
        manifestPath = _inv.globalFlags.configPath;
    else if (Error e = Cmd::GetDefaultManifestPath(manifestPath))
        return e;
    
    if (Error e = _ws.Load(manifestPath))
        return e;
    
    if (!_ws.HasProject(project))
        return Error::Validation("project '" + project + "' does not exist");
    
    ProjectManifest manifest;
    if (Error e = _ws.GetProjectManifest(project, manifest))
        return e;
    
    std::string out = "'" + project + "' Dependency list\n";
    for (auto const& d : manifest.dependencies)
        out += "  - " + d + "\n";
    
    Log::Log(LogType::Help, out);
    
    return Error::Ok();
}
