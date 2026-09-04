#include "DepAdd.h"

#include "Command.h"

namespace 
{
    Error ModifyManifest(ProjectManifest& _m, std::string _dep)
    {
        for (auto& d : _m.dependencies)
            if (d == _dep)
                return Error::Validation("dependency '" + _dep + "' already exists in project '" + _m.name + "'");
        
        _m.dependencies.push_back(_dep);
        return Error::Ok();
    }
    
    bool Prompt(std::string _name, bool _prompt)
    {
        std::vector<std::string> plan;
        
        plan.push_back("Modify the '" + _name + ".json' manifest file");
        
        Cmd::PrintPlan(plan);
        
        if (_prompt == false)
            return true;
        return Cmd::ApplyPrompt(static_cast<int>(plan.size()));
    }
}


Error Cmd::DepAdd(Workspace& _ws, Invocation const& _inv)
{
    ArgBag const& bag = _inv.argBag;
    
    if (bag.Exists("project") == false)
        return Error::Validation("missing required 'project' argument");
    if (bag.Exists("dependency") == false)
        return Error::Validation("missing required 'dependency' argument");
    
    std::string project = bag.Get("project");
    std::string dependency = bag.Get("dependency");
    bool isVerbose = _inv.globalFlags.verbose;
    
    fs::path manifestPath;
    if (_inv.globalFlags.hasConfigPath)
        manifestPath = _inv.globalFlags.configPath;
    else if (Error e = Cmd::GetDefaultManifestPath(manifestPath))
        return e;
    
    if (Error e = _ws.Load(manifestPath))
        return e;
    
    if (_inv.globalFlags.dryRun)
    {
        Prompt(project, false);
        return Error::Ok();
    }
    if (_inv.globalFlags.yes == false)
    {
        bool res = Prompt(project, true);
        if (res == false)
            return Error::Declined("user declined any changes");
    }
    
    bool projectFound = false;
    bool depFound = false;
    
    for (auto& p : _ws.GetProjects())
    {
        if (p.name == project)
        {
            projectFound = true;
            ModifyManifest(p, dependency);
            Verbose("added '" + dependency +"' as a dependency for project '" + project + "'", isVerbose);
            continue;
        }
        if (p.name == dependency)
            depFound = true;
    }
    
    if (projectFound == false)
        return Error::Validation("project '" + project + "' doesn't exist");
    if (depFound == false)
        return Error::Validation("project '" + dependency + "' doesn't exist");
    
    if (Error e = _ws.Save(manifestPath.parent_path()))
        return e;
    
    return Error::Ok();
}
