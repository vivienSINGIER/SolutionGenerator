#include "Command.h"

namespace
{
    std::string main_h = std::string("#ifndef MAIN_H_DEFINED\n") 
        + "#define MAIN_H_DEFINED\n\n" + "#endif\n";
    std::string main_cpp = std::string("#include \"main.h\"\n\n") 
        + "int main(int _argc, char** _argv)\n" + "{\n" 
        + "return 0;\n" + "}\n";
    std::string wmain_cpp = std::string("#include <windows.h>\n")
        + "#include \"main.h\"\n\n" 
        + "int WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)"
        + "{\n" + "return 0;\n" + "}\n";
    
    std::vector<std::pair<std::string, std::string>> bats =
    {
        {"-make.bat", "\"%~dp0..\\Liftoff.exe\" make -y -v --only "},
    };
    
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
    
    Error FillManifest(ProjectManifest& _m, ArgBag const& _bag)
    {
        _m.name = _bag.Get("name");
        
        std::string kind = _bag.Get("kind");
        bool isCorrect = (kind == "console" || kind == "window" || kind == "staticlib" || kind == "dynamiclib");
        if (isCorrect == false)
            return Error::Validation("incorrect 'kind' argument given '" + kind + "'");
        
        _m.kind = kind;
        std::vector<std::string> dependencies = _bag.GetList("depends-on");
        for (auto const& dependency : dependencies)
            _m.dependencies.push_back(dependency);
        
        return Error::Ok();
    }
    
    bool Prompt(std::string _name, bool _prompt, bool _scaffold, bool _hasDefCpp)
    {
        std::vector<std::string> plan;
        
        if (_scaffold)
        {
            plan.push_back("Create the '/src/" + _name + "' & '/res/" + _name + "' directories");
            if (_hasDefCpp)
                plan.push_back("Create the default 'main.h' & 'main.cpp' files");
        }
        
        if (_scaffold)
        {
            std::string batStr = "Create the following .bat helpers :\n";
        
            for (auto const& b : bats)
                batStr += "      - '" + b.first + "'\n";
            plan.push_back(batStr);
        }
        
        plan.push_back("Create the project manifest file");
        plan.push_back("Modify the solution manifest file");
        
        Cmd::PrintPlan(plan);
        
        if (_prompt == false)
            return true;
        return Cmd::ApplyPrompt(static_cast<int>(plan.size()));
    }
}

Error Cmd::ProjectAdd(Workspace& _ws, Invocation const& _inv)
{
    ArgBag const& bag = _inv.argBag;
    
    if (bag.Exists("name") == false)
        return Error::Validation("missing required 'name' argument");
    if (bag.Exists("kind") == false)
        return Error::Validation("missing required 'kind' argument");
    
    std::string name = bag.Get("name");
    std::string kind = bag.Get("kind");
    bool isVerbose = _inv.globalFlags.verbose;
    
    fs::path manifestPath;
    if (_inv.globalFlags.hasConfigPath)
        manifestPath = _inv.globalFlags.configPath;
    else if (Error e = Cmd::GetDefaultManifestPath(manifestPath))
        return e;
    
    if (Error e = _ws.Load(manifestPath))
        return e;
    
    if (_ws.HasProject(name))
        return Error::Validation("project '" + name + "' already exists");
    
    for (auto& b : bats)
    {
        b.first = name + b.first;
        b.second.append(name);
    }
    
    bool hasDefCpp = kind == "console" || kind == "window";
    bool scaffold = true;
    if (bag.GetFlag("no-scaffold"))
        scaffold = false;
    
    if (_inv.globalFlags.dryRun)
    {
        Prompt(name, false, scaffold, hasDefCpp);
        return Error::Ok();
    }
    if (_inv.globalFlags.yes == false)
    {
       bool res = Prompt(name, true, scaffold, hasDefCpp);
        if (res == false)
            return Error::Declined("user declined any changes");
    }
    
    fs::path root;
    if (Error e = Cmd::GetRootPath(root))
        return e;
    
    if (scaffold)
    {
        if (Error e = FileHelper::CreateDir(root / "src" / name))
            return e;
        if (Error e = FileHelper::CreateDir(root / "res" / name))
            return e;
        Verbose("Created '/src/" + name + "' & '/res/" + name + "' directories", isVerbose);
        
        if (kind == "console")
        {
            if (Error e = FileHelper::WriteFileAtomic(root / "src" / name / "main.h", main_h))
                return e;
            if (Error e = FileHelper::WriteFileAtomic(root / "src" / name / "main.cpp", main_cpp))
                return e;
            Verbose("Created 'main.h' & 'main.cpp' template files", isVerbose);
        }
        else if (kind == "window")
        {
            if (Error e = FileHelper::WriteFileAtomic(root / "src" / name / "main.h", main_h))
                return e;
            if (Error e = FileHelper::WriteFileAtomic(root / "src" / name / "main.cpp", wmain_cpp))
                return e;
            Verbose("Created 'main.h' & 'main.cpp' template files", isVerbose);
        }

        if (Error e = CreateBats(root / "tools" / "bats", isVerbose))
            return e;
    }
    
    ProjectManifest projectManifest;
    if (Error e = FillManifest(projectManifest, bag))
        return e;
    
    if (Error e = _ws.AddProject(projectManifest))
        return e;
    if (Error e = _ws.Save(manifestPath.parent_path()))
        return e;
    Verbose("Modified solution manifest", isVerbose);
    Verbose("Created project manifest", isVerbose);
    
    return Error::Ok();
}
