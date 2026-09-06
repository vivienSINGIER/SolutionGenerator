#include "Command.h"

#include <algorithm>

namespace
{
    std::vector<std::pair<std::string, std::string>> fixedBats =
    {
        {"make.bat",    "../Liftoff.exe make"},
        {"build.bat",   "../Liftoff.exe build"},
        {"rebuild.bat", "../Liftoff.exe rebuild"},
        {"clean.bat",   "../Liftoff.exe clean"},
        {"clear.bat",   "../Liftoff.exe clear"},
    };

    std::vector<std::pair<std::string, std::string>> ProjectBats(ProjectManifest const& _p)
    {
        std::vector<std::pair<std::string, std::string>> bats =
        {
            {_p.name + "-build.bat", "../Liftoff.exe build --only " + _p.name},
            {_p.name + "-make.bat",  "../Liftoff.exe make --only "  + _p.name},
        };
        if (_p.kind == "console" || _p.kind == "window")
            bats.push_back({_p.name + "-run.bat", "../Liftoff.exe run " + _p.name});

        return bats;
    }

    bool Prompt(std::vector<ProjectManifest> const& _projects, bool _prompt)
    {
        std::vector<std::string> plan;

        plan.push_back("Regenerate the fixed .bat helpers (make, build, rebuild, clean, clear)");

        std::string perProject = "Regenerate the per-project .bat helpers :\n";
        for (ProjectManifest const& p : _projects)
            for (auto const& b : ProjectBats(p))
                perProject += "      - '" + b.first + "'\n";
        plan.push_back(perProject);

        plan.push_back("Delete any stale .bat helper left over from a removed project");

        Cmd::PrintPlan(plan);

        if (_prompt == false)
            return true;
        return Cmd::ApplyPrompt(static_cast<int>(plan.size()));
    }
}

Error Cmd::SyncBats(Workspace& _ws, Invocation const& _inv)
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

    fs::path batsDir = root / "tools" / "bats";

    std::vector<std::pair<std::string, std::string>> bats = fixedBats;
    for (ProjectManifest const& p : _ws.GetProjects())
        for (auto const& b : ProjectBats(p))
            bats.push_back(b);

    for (auto const& b : bats)
    {
        if (Error e = FileHelper::WriteFileAtomic(batsDir / b.first, b.second))
            return e;
        Cmd::Verbose("synced helper '" + b.first + "'", isVerbose);
    }

    std::vector<fs::path> existing;
    if (Error e = FileHelper::ListDir(batsDir, existing))
        return e;

    for (fs::path const& file : existing)
    {
        std::string name = file.filename().string();

        bool isExpected = std::find_if(bats.begin(), bats.end(),
            [&name](auto const& b) { return b.first == name; }) != bats.end();

        if (isExpected == false)
        {
            if (Error e = FileHelper::RemoveFile(file))
                return e;
            Cmd::Verbose("deleted stale helper '" + name + "'", isVerbose);
        }
    }

    return Error::Ok();
}
