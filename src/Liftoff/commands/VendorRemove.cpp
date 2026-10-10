#include "Command.h"

namespace
{
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


Error Cmd::VendorRemove(Workspace& _ws, Invocation const& _inv)
{
    ArgBag const& bag = _inv.argBag;

    if (bag.Exists("project") == false)
        return Error::Validation("missing required 'project' argument");
    if (bag.Exists("lib") == false)
        return Error::Validation("missing required 'lib' argument");

    std::string project = bag.Get("project");
    std::string lib = bag.Get("lib");
    bool isVerbose = _inv.globalFlags.verbose;

    fs::path manifestPath;
    if (_inv.globalFlags.hasConfigPath)
        manifestPath = _inv.globalFlags.configPath;
    else if (Error e = Cmd::GetDefaultManifestPath(manifestPath))
        return e;

    if (Error e = _ws.Load(manifestPath))
        return e;

    if (_ws.HasProject(project) == false)
        return Error::Validation("project '" + project + "' doesn't exist");

    ProjectManifest* pM = _ws.GetProjectManifest(project);

    if (pM->vendor.contains(lib) == false)
        return Error::Validation("project '" + project + "' has no vendor '" + lib + "'");

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

    pM->vendor.erase(lib);
    Verbose("removed vendor '" + lib + "' from project '" + project + "'", isVerbose);

    if (Error e = _ws.Save(manifestPath.parent_path()))
        return e;

    return Error::Ok();
}
