#include "ProjectRemove.h"

#include "Command.h"

namespace
{
    Error ModifyManifests(Workspace& _ws, std::string _name)
    {
        SolutionManifest& s = _ws.GetSolution();
        bool found = false;
        for (size_t i = 0; i < s.projects.size(); i++)
        {
            if (s.projects[i].name == _name)
            {
                s.projects.erase(s.projects.begin() + static_cast<int64_t>(i));
                found = true;
                break;
            }
        }
        
        if ( found == false )
            return Error::Validation("project '" + _name + "' doesn't exist");
        
        size_t index = 0;
        found = false;
        for ( size_t i = 0; i < _ws.GetProjects().size(); i++)
        {
            ProjectManifest& project = _ws.GetProjects()[i];
            if (project.name == _name)
            {
                found = true;
                index = i;
                continue;
            }
            
            for ( size_t j = 0; j < project.dependencies.size(); j++)
            {
                if (project.dependencies[j] == _name)
                {
                    project.dependencies.erase(project.dependencies.begin() + static_cast<int64_t>(j));
                    break;
                }
            }
        }
        
        if ( found == true )
            _ws.GetProjects().erase(_ws.GetProjects().begin() + static_cast<int64_t>(index));
        
        return Error::Ok();
    }
    
    bool Prompt(std::string _name, bool _prompt, bool _cascade)
    {
        std::vector<std::string> plan;
        
        plan.push_back("Remove the project '" + _name + "' from the solution manifest file");
        plan.push_back("Remove all dependencies to this project");
        plan.push_back("Delete the project '" + _name + "' manifest file");
        plan.push_back("Delete all '" + _name + "' related bat helpers");
        
        if (_cascade)
        {
            plan.push_back("Delete the '/src/" + _name + "' & '/res/" + _name + "' directories & their content");
        }
        
        Cmd::PrintPlan(plan);
        
        if (_prompt == false)
            return true;
        return Cmd::ApplyPrompt(static_cast<int>(plan.size()));
    }
}

Error Cmd::ProjectRemove(Workspace& _ws, Invocation const& _inv)
{
    ArgBag const& bag = _inv.argBag;
    
    if (bag.Exists("name") == false)
        return Error::Validation("missing required 'name' argument");
    
    std::string name = bag.Get("name");
    bool isVerbose = _inv.globalFlags.verbose;
    bool cascade = false;
    if (bag.GetFlag("cascade"))
        cascade = true;
    
    fs::path manifestPath;
    if (_inv.globalFlags.hasConfigPath)
        manifestPath = _inv.globalFlags.configPath;
    else if (Error e = Cmd::GetDefaultManifestPath(manifestPath))
        return e;
    
    if (Error e = _ws.Load(manifestPath))
        return e;
    
    if (!_ws.HasProject(name))
        return Error::Validation("project '" + name + "' doesn't exist");
    
    if (_inv.globalFlags.dryRun)
    {
        Prompt(name, false, cascade);
        return Error::Ok();
    }
    if (_inv.globalFlags.yes == false)
    {
        bool res = Prompt(name, true, cascade);
        if (res == false)
            return Error::Declined("user declined any changes");
    }
    
    fs::path root;
    if (Error e = Cmd::GetRootPath(root))
        return e;
    
    if (Error e = ModifyManifests(_ws, name))
        return e;
    Verbose("Modified the solution manifest file", isVerbose);
    Verbose("Removed dependencies to project '" + name + "'", isVerbose);
    
    fs::path projectM = manifestPath.parent_path() / "projects" / (name + ".json");
    if (Error e = FileHelper::RemoveFile(projectM))
        return e;
    Verbose("Deleted project '" + name + "' manifest file", isVerbose);
    
    std::vector<fs::path> tools;
    if (Error e = FileHelper::ListDir(root / "tools", tools))
        return e;
    
    if (Error e = _ws.Save(manifestPath.parent_path()))
        return e;
    
    for (fs::path t : tools)
    {
        if (t.string().find(name) != std::string::npos)
        {
            if (Error e = FileHelper::RemoveFile(t))
                return e;
            Verbose("Deleted helper '" + t.string() + "'", isVerbose);
        }
    }
    
    if (cascade)
    {
        if (Error e = FileHelper::DeleteDir(root / "src" / name))
            return e;
        Verbose("Deleted '/src/' directory and its content", isVerbose);
        if (Error e = FileHelper::DeleteDir(root / "res" / name))
            return e;
        Verbose("Deleted '/res/' directory and its content", isVerbose);
    }
    
    return Error::Ok();
}
