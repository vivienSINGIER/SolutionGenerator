#include "Command.h"

namespace
{
    // Compiler output lives in ide/<project>/<platform>/ (OutDir + IntDir, see GenVcproj),
    // Visual Studio's cache in ide/.vs/
    std::vector<fs::path> OutputDirs(Workspace& _ws, fs::path const& _ide)
    {
        std::vector<fs::path> dirs;

        for (ProjectManifest const& p : _ws.GetProjects())
            for (std::string const& platform : _ws.GetSolution().platforms)
                dirs.push_back(_ide / p.name / platform);

        dirs.push_back(_ide / ".vs");
        return dirs;
    }

    void Prompt(std::vector<fs::path> const& _dirs)
    {
        std::vector<std::string> plan;

        std::string dirs = "Delete the compiler output & Visual Studio cache directories :\n";
        for (fs::path const& d : _dirs)
            dirs += "      - '" + d.string() + "'\n";
        plan.push_back(dirs);

        Cmd::PrintPlan(plan);
    }
}

Error Cmd::Clean(Workspace& _ws, Invocation const& _inv)
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

    fs::path ide = root / "ide";
    std::vector<fs::path> dirs = OutputDirs(_ws, ide);

    if (_inv.globalFlags.dryRun)
    {
        Prompt(dirs);
        return Error::Ok();
    }

    for (fs::path const& d : dirs)
    {
        if (FileHelper::IsDir(d) == false)
            continue;

        if (Error e = FileHelper::DeleteDir(d))
            return e;
        Verbose("Deleted directory '" + d.string() + "'", isVerbose);
    }

    return Error::Ok();
}
