#include "Command.h"

#include <algorithm>

namespace
{
    VendorDep& GetVendor(ProjectManifest& _m, std::string const& _vName)
    {
        if (_m.vendor.contains(_vName))
            return _m.vendor[_vName];

        VendorDep v;
        _m.vendor[_vName] = v;
        return _m.vendor[_vName];
    }

    void Store(std::vector<std::string> const& _values, std::vector<std::string>& _out)
    {
        for (auto const& value : _values)
        {
            if (std::find(_out.begin(), _out.end(), value) == _out.end())
                _out.push_back(value);
        }
    }

    bool IsConfig(SolutionManifest const& _s, std::string const& _config)
    {
        for (auto const& c : _s.configs)
            if (c == _config)
                return true;
        return false;
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


Error Cmd::VendorAdd(Workspace& _ws, Invocation const& _inv)
{
    ArgBag const& bag = _inv.argBag;

    if (bag.Exists("project") == false)
        return Error::Validation("missing required 'project' argument");
    if (bag.Exists("lib") == false)
        return Error::Validation("missing required 'lib' argument");

    std::string project = bag.Get("project");
    std::string lib = bag.Get("lib");
    std::string config = bag.Get("configuration");
    std::vector<std::string> includes = bag.GetList("include");
    std::vector<std::string> libDirs = bag.GetList("lib-dir");
    std::vector<std::string> libs = bag.GetList("libs");
    std::vector<std::string> dlls = bag.GetList("dlls");

    bool isVerbose = _inv.globalFlags.verbose;

    if (includes.empty() && libDirs.empty() && libs.empty() && dlls.empty())
        return Error::Validation("nothing to add, give at least one of --include, --lib-dir, --libs or --dlls");
    if (config.empty() == false && includes.empty() == false)
        return Error::Validation("--include applies to every configuration, it can't be used with --configuration");

    fs::path manifestPath;
    if (_inv.globalFlags.hasConfigPath)
        manifestPath = _inv.globalFlags.configPath;
    else if (Error e = Cmd::GetDefaultManifestPath(manifestPath))
        return e;

    if (Error e = _ws.Load(manifestPath))
        return e;

    if (_ws.HasProject(project) == false)
        return Error::Validation("project '" + project + "' doesn't exist");
    if (config.empty() == false && IsConfig(_ws.GetSolution(), config) == false)
        return Error::Validation("configuration '" + config + "' isn't one of the solution's configs");

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

    ProjectManifest* pM = _ws.GetProjectManifest(project);

    VendorDep& dep = GetVendor(*pM, lib);

    Store(includes, dep.include);

    if (config.empty())
    {
        Store(libDirs, dep.libDirs);
        Store(libs, dep.libs);
        Store(dlls, dep.dlls);
        Verbose("added to vendor '" + lib + "' of project '" + project + "'", isVerbose);
    }
    else
    {
        VendorConfig& vc = dep.configs[config];
        Store(libDirs, vc.libDirs);
        Store(libs, vc.libs);
        Store(dlls, vc.dlls);
        Verbose("added to vendor '" + lib + "' of project '" + project + "' for config '" + config + "'", isVerbose);
    }

    if (Error e = _ws.Save(manifestPath.parent_path()))
        return e;

    return Error::Ok();
}
