#include "Command.h"

namespace
{
    bool Prompt(std::vector<ProjectManifest> const& _projects, bool _prompt)
    {
        std::vector<std::string> plan;

        plan.push_back("Generate the solution sln file");
        plan.push_back("Generate the project vcxproj files");
        plan.push_back("Generate the project vcxproj.filter files");

        Cmd::PrintPlan(plan);

        if (_prompt == false)
            return true;
        return Cmd::ApplyPrompt(static_cast<int>(plan.size()));
    }
}

Error Cmd::Make(Workspace& _ws, Invocation const& _inv)
{
    bool isVerbose = _inv.globalFlags.verbose;

    fs::path manifestPath;
    if (_inv.globalFlags.hasConfigPath)
        manifestPath = _inv.globalFlags.configPath;
    else if (Error e = Cmd::GetDefaultManifestPath(manifestPath))
        return e;
    
    if (Error e = _ws.Load(manifestPath))
        return e;

    fs::path root;
    if (Error e = Cmd::GetRootPath(root))
        return e;
    
    if (_inv.globalFlags.dryRun)
    {
        Prompt(_ws.GetProjects(), false);
        return Error::Ok();
    }
    if (_inv.globalFlags.yes == false)
    {
        bool res = Prompt(_ws.GetProjects(), true);
        if (res == false)
            return Error::Declined("user declined any changes");
    }
    
    return Error::Ok();
}
