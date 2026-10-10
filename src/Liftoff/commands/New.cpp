#include "Command.h"
#include "cli/Invocation.h"
#include "io/FileHelper.h"

namespace
{
    std::vector<std::pair<std::string, std::string>> bats =
        {
        {"make.bat", "\"%~dp0..\\Liftoff.exe\" make -y -v"},
        {"clean.bat", "\"%~dp0..\\Liftoff.exe\" clean -y -v"},
        {"clear.bat", "\"%~dp0..\\Liftoff.exe\" clear -y -v"},
    };
    
    std::string validToolsets[] = { "v143" };
    std::string validStdVersions[] = { "20", "17", "14" };
    
    bool Prompt(std::vector<fs::path> const& _dirs, bool _prompt)
    {
        std::vector<std::string> plan;
        
        std::string dirs = "Create the following directories :\n";
        for (fs::path const& dir : _dirs)
            dirs += "      - '" + dir.string() + "'\n"; 
        plan.push_back(dirs);
        
        plan.push_back("Create '.liftoff' & '.gitignore' at root directory");
        plan.push_back("Copy 'Liftoff.exe' to /tools");
        
        std::string batStr = "Create the following .bat helpers :\n";
        
        for (auto const& b : bats)
            batStr += "      - '" + b.first + "'\n";
        plan.push_back(batStr);
        
        plan.push_back("Create or override the solution manifest file");
        
        Cmd::PrintPlan(plan);
        
        if (_prompt == false)
            return true;
        return Cmd::ApplyPrompt(static_cast<int>(plan.size()));
    }
    
    Error FillWorkspace(Workspace& _ws, ArgBag const& _args)
    {
        SolutionManifest& sm = _ws.GetSolution();
        
        sm.name = _args.Get("name");
        
        if (_args.Exists("toolset"))
        {
            std::string input = _args.Get("toolset");
            bool isValid = false;
            for (std::string const& toolset : validToolsets)
                if (input == toolset)
                    isValid = true;
            
            if (isValid == false)
                return Error::Validation("invalid or unimplemented toolset version");
            
            sm.toolset = _args.Get("toolset");
        }
        if (_args.Exists("cpp-standard"))
        {
            std::string input = _args.Get("cpp-standard");
            bool isValid = false;
            for (std::string const& cppStandard : validStdVersions)
                if (input == cppStandard)
                    isValid = true;
            
            if (isValid == false)
                return Error::Validation("invalid cpp-standard version");
            
            sm.cppStandard = _args.Get("cpp-standard");
        }
        if (_args.Exists("platform"))
        {
            sm.platforms = _args.GetList("platform");
        }

        return Error::Ok();
    }
    
    Error CreateBats(fs::path const& _path, bool _isV)
    {
        Error e;
        
        for (size_t i = 0; i < bats.size(); i++)
        {
            fs::path path = _path / bats[i].first;
            if (Error tempE = FileHelper::WriteFileAtomic(path, bats[i].second))
                e.details.push_back(tempE.Format());
            else
                Cmd::Verbose("created helper '" + bats[i].first + "'", _isV);
        }
        
        if (e.details.size() > 0)
            return e;
        return Error::Ok();
    }
}

Error Cmd::New(Workspace& _ws, Invocation const& _inv)
{
    ArgBag const& bag = _inv.argBag;
    
    if (bag.Exists("path") == false)
        return Error::Validation("missing required 'path' argument");       
    if (bag.Exists("name") == false)
        return Error::Validation("missing required 'name' argument");
    
    fs::path path = bag.GetPath("path");
    std::string name = bag.Get("name");
    
    fs::path const root = path / name;
    
    fs::path manifestPath = root / "config" / (name + ".json");
    if (_inv.globalFlags.hasConfigPath)
        manifestPath = _inv.globalFlags.configPath;
    
    bool configExists = FileHelper::IsFile(manifestPath);
    if (bag.GetFlag("force") == false && configExists)
        return Error::Validation("solution manifest already exists at '" + manifestPath.string() + "'");

    std::vector<fs::path> dirs = {
        root / "src",
        root / "res",
        root / "tools",
        root / "tools" / "bats",
        root / "config",
        root / "vendor"
    };

    if (_inv.globalFlags.dryRun)
    {
        Prompt(dirs, false);
        return Error::Ok();
    }
    if (_inv.globalFlags.yes == false)
    {
        bool res = Prompt(dirs, true);
        if (res == false)
            return Error::Declined("user declined any changes");
    }
    
    bool isVerbose = _inv.globalFlags.verbose;
    
    for (fs::path const& dir : dirs)
    {
        if (Error e = FileHelper::CreateDir(dir))
            return e;
        Verbose("created directory '" + dir.string() + "'", isVerbose);
    }
     
    if (Error e = FileHelper::WriteFileAtomic(root/ ".liftoff", name))
        return e;
    Verbose("created '.liftoff' root file", isVerbose);
    if (Error e = FileHelper::WriteFileAtomic(root/ ".gitignore", "ide"))
        return e;
    Verbose("created '.gitignore' root file", isVerbose);
    
    if (Error e = FillWorkspace(_ws, bag))
        return e;

    fs::path selfPath;
    FileHelper::ExecutablePath(selfPath);
    
    if (Error e = FileHelper::CopyFileTo(selfPath, root / "tools" / "Liftoff.exe", true))
        return e;
    Verbose("copied 'Liftoff.exe' to tools directory", isVerbose);
    
    if (Error e = CreateBats(root / "tools" / "bats", isVerbose))
        return e;
    
    if (Error e = _ws.GetSolution().Serialize(manifestPath))
        return e;
    if (configExists)
        Verbose("overwritten solution manifest", isVerbose);
    else 
        Verbose("created solution manifest", isVerbose);
    
    return Error::Ok();
}
