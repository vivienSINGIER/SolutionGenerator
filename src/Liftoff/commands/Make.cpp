#include "Command.h"
#include "gen/Gen.h"

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
    ArgBag bag = _inv.argBag;
    
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
    
    SolutionManifest& s = _ws.GetSolution();
    fs::path slnPath = root / "ide" / (s.name + ".sln");
    
    if (Error e = Gen::GenSln(s, slnPath))
        return e;
    Verbose("Generated solution .sln file at '" + slnPath.string() + "'", isVerbose);
    
    if (bag.Exists("only"))
    {
        std::vector<std::string> projects = bag.GetList("only");
        
        for (auto const& p : projects)
        {
            if (_ws.HasProject(p) == false)
                return Error::Validation("couldn't find project '" + p + "' in solution manifest");
            
            ProjectManifest pM;
            _ws.GetProjectManifest(p, pM);
            
            fs::path vcProjPath = slnPath.parent_path() / p / (p + ".vcxproj");
            fs::path vcFilterPath = slnPath.parent_path() / p / (p + ".vcxproj.filters");
            
            if (Error e = Gen::GenVcProj(_ws, s, pM, _ws.GetDependencyGraph(), vcProjPath))
                return e;
            Verbose("Generated project .vcxproj file at '" + vcProjPath.string() + "'", isVerbose);
            Verbose("Generated project .vcxproj.filters file at '" + vcFilterPath.string() + "'", isVerbose);
        }
    }
    else
    {
        for (auto const& p : _ws.GetProjects())
        {
            fs::path vcProjPath = slnPath.parent_path() / p.name / (p.name + ".vcxproj");
            fs::path vcFilterPath = slnPath.parent_path() / p.name / (p.name + ".vcxproj.filters");
            
            if (Error e = Gen::GenVcProj(_ws, s, p, _ws.GetDependencyGraph(), vcProjPath))
                return e;
            Verbose("Generated project .vcxproj file at '" + vcProjPath.string() + "'", isVerbose);
            Verbose("Generated project .vcxproj.filters file at '" + vcFilterPath.string() + "'", isVerbose);
        }
    }
    
    return Error::Ok();
}
