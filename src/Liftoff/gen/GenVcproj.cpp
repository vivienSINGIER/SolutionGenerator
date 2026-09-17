#include "Gen.h"
#include "Guid.h"

#include "commands/Command.h"
#include "graph/DependencyGraph.h"
#include "io/FileHelper.h"
#include "scan/SourceDiscovery.h"

namespace
{
    using Gen::Braced;
    using Gen::Tag;

    std::string Header()
    {
        std::string out;
        out += "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
        out += "<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">";
        out += "\n";
        return out;
    }
    
    std::string ProjectConfigurations(SolutionManifest const& _s)
    {
        std::vector<std::string> values;
        
        for (auto const& p : _s.platforms)
        {
            for (auto const& c : _s.configs)
            {
                std::string config = Tag("Configuration", {c}, 0, {});
                std::string platform = Tag("Platform", {p}, 0, {});
                
                std::string v = Tag("ProjectConfiguration", {config, platform}, 0, {"Include=\"" + c + "|" + p + "\""});
                values.push_back(v);
            }
        }
        
        return Tag("ItemGroup", values, 1, {"Label=\"ProjectConfigurations\""});
    }
    
    std::string Globals(SolutionManifest const& _s, ProjectManifest const& _p)
    {
        std::string guid = Tag("ProjectGuid", {Braced(GUID::GenerateGuid(_s.name, _p.name))}, 0, {});
        std::string rNamespace = Tag("RootNamespace", {_p.name}, 0, {});
        std::string keyword = Tag("Keyword", {"Win32Proj"}, 0, {});
        
        return Tag("PropertyGroup", {guid, rNamespace, keyword}, 1, {"Label=\"" + std::string("Globals") + "\""});
    }
    
    std::string Configuration(SolutionManifest const& _s, ProjectManifest const& _p)
    {
        std::string pType = "Application";
        if (_p.kind == "staticlib")
            pType = "StaticLibrary";
        else if (_p.kind == "dynamiclib")
            pType = "DynamicLibrary";
        
        std::string type = Tag("ConfigurationType", {pType}, 0, {});
        std::string useDebugD = Tag("UseDebugLibraries", {"true"}, 0, {"Condition=\"'$(Configuration)'=='Debug'\""});
        std::string useDebugR = Tag("UseDebugLibraries", {"false"}, 0, {"Condition=\"'$(Configuration)'=='Release'\""});
        std::string toolset = Tag("PlatformToolset", {_s.toolset}, 0, {});
        std::string opti = Tag("WholeProgramOptimization", {"true"}, 0, {"Condition=\"'$(Configuration)'=='Release'\""});
        std::string charSet = Tag("CharacterSet", {_s.characterSet}, 0, {});
        
        std::string indent = "    ";
        
        std::string out = indent + "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n";
        out += Tag("PropertyGroup", {type, useDebugD, useDebugR, toolset, opti, charSet}, 1, {"Label=\"" + std::string("Configuration") + "\""});
        out += indent + "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n";
        out += indent + "<ImportGroup Label=\"ExtensionSettings\" />\n";
        out += indent + "<ImportGroup Label=\"PropertySheets\">\n";
        out += indent + indent + 
            "<Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" "
            "Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" "
            "Label=\"LocalAppDataPlatform\" />\n";
        out += indent + "</ImportGroup>\n";
        out += indent + "<PropertyGroup Label=\"UserMacros\" />\n";
        return out;
    }
    
    std::string OutDirs()
    {
        std::string outDir = Tag("OutDir", {"$(ProjectDir)$(Platform)\\$(Configuration)\\"}, 0, {});
        std::string intDir = Tag("IntDir", {"$(ProjectDir)$(Platform)\\$(Configuration)\\intermediate\\"}, 0, {});
        
        return Tag("PropertyGroup", {outDir, intDir}, 1, {});
    }
    
    std::string Vcpkg(ProjectManifest const& _p)
    {
        std::string enabled = Tag("VcpkgEnabled", {"false"}, 0, {});
        std::string manifest = Tag("VcpkgEnableManifest", {"false"}, 0, {});
        std::string root = Tag("VcpkgManifestRoot", {"$(SolutionDir)..\\config\\vcpkg\\" + _p.name}, 0, {});
        std::string dir = Tag("VcpkgInstalledDir", {"$(SolutionDir)..\\vendor\\$(VcpkgTriplet)"}, 0, {});
        
        return Tag("PropertyGroup", {enabled, manifest, root, dir}, 1, {});
    }
    
    ConfigOverride const* FindOverride(std::map<std::string, ConfigOverride> const& _overrides,
                                        std::string const& _config, int _fieldIndex)
    {
        auto it = _overrides.find(_config);
        if (it == _overrides.end() || it->second.overrides[_fieldIndex] == false)
            return nullptr;
        return &it->second;
    }

    std::string GetWarningLevel(SolutionManifest const& _s, ProjectManifest const& _p, std::string const& _config)
    {
        std::string level = _s.warningLvl;

        if (ConfigOverride const* o = FindOverride(_s.configOverrides, _config, 0))
            level = o->warningLvl;
        if (ConfigOverride const* o = FindOverride(_p.configOverrides, _config, 0))
            level = o->warningLvl;

        return level;
    }

    bool GetWarningsAsErrors(SolutionManifest const& _s, ProjectManifest const& _p, std::string const& _config)
    {
        bool value = _s.warningsAsErrors;

        if (ConfigOverride const* o = FindOverride(_s.configOverrides, _config, 1))
            value = o->warningsAsErrors;
        if (ConfigOverride const* o = FindOverride(_p.configOverrides, _config, 1))
            value = o->warningsAsErrors;

        return value;
    }

    bool GetConformanceMode(SolutionManifest const& _s, ProjectManifest const& _p, std::string const& _config)
    {
        bool value = _s.conformanceMode;

        if (ConfigOverride const* o = FindOverride(_s.configOverrides, _config, 2))
            value = o->conformanceMode;
        if (ConfigOverride const* o = FindOverride(_p.configOverrides, _config, 2))
            value = o->conformanceMode;

        return value;
    }
    
    std::string GetPreprocessorDeff(SolutionManifest const& _s, ProjectManifest const& _p,
                                     std::string const& _config, std::string const& _platform)
    {
        std::vector<std::string> defines;

        if (_platform == "Win32")
            defines.push_back("_WIN32_");

        defines.push_back(_config == "Debug" ? "_DEBUG_" : "_NO_DEBUG_");

        if (_p.kind == "console")
            defines.push_back("_CONSOLE_APP_");
        else if (_p.kind == "window")
            defines.push_back("_WINDOWS_APP_");

        for (auto const& d : _s.defines)
            defines.push_back(d);
        for (auto const& d : _p.defines)
            defines.push_back(d);

        if (ConfigOverride const* o = FindOverride(_s.configOverrides, _config, 3))
            for (auto const& d : o->defines)
                defines.push_back(d);
        if (ConfigOverride const* o = FindOverride(_p.configOverrides, _config, 3))
            for (auto const& d : o->defines)
                defines.push_back(d);

        std::string out;
        for (auto const& d : defines)
            out += d + ";";
        out += "%(PreprocessorDefinitions)";
        return out;
    }
    
    using Gen::AddRootRelative;

    std::string GetIncludeDirs(ProjectManifest const& _p, ResolvedVendor const& _vendor,
                                std::vector<fs::path> const& _exports, fs::path const& _root)
    {
        std::vector<std::string> dirs;

        dirs.push_back(AddRootRelative(fs::path("src") / _p.name));

        for (auto const& dir : _exports)
            dirs.push_back(AddRootRelative(fs::relative(dir, _root)));
        for (auto const& dir : _vendor.includeDirs)
            dirs.push_back(AddRootRelative(fs::relative(dir, _root)));

        std::string out;
        for (auto const& dir : dirs)
            out += dir + ";";
        out += "%(AdditionalIncludeDirectories)";
        return out;
    }

    std::string ItemDeffClCompile(SolutionManifest const& _s, ProjectManifest const& _p,
                                   ResolvedVendor const& _vendor, std::vector<fs::path> const& _exports,
                                   fs::path const& _root)
    {
        std::string funcLvlLinking = Tag("FunctionLevelLinking", {"true"}, 0, {"Condition=\"'$(Configuration)'=='Release'\""});
        std::string intrinsicFuncs = Tag("IntrinsicFunctions", {"true"}, 0, {"Condition=\"'$(Configuration)'=='Release'\""});
        std::string sdl = Tag("SDLCheck", {"true"});
        std::string cppStd = Tag("LanguageStandard", {"stdcpp" + _s.cppStandard});
        std::string includeDirs = Tag("AdditionalIncludeDirectories", {GetIncludeDirs(_p, _vendor, _exports, _root)});

        std::vector<std::string> warningLevels;
        std::vector<std::string> treatWarnAsErrors;
        std::vector<std::string> conformanceModes;
        std::vector<std::string> preprocessorDeffs;

        for (auto const& c : _s.configs)
        {
            std::string condition = "Condition=\"'$(Configuration)'=='" + c + "'\"";

            warningLevels.push_back(Tag("WarningLevel", {GetWarningLevel(_s, _p, c)}, 0, {condition}));
            treatWarnAsErrors.push_back(Tag("TreatWarningAsError", {GetWarningsAsErrors(_s, _p, c) ? "true" : "false"}, 0, {condition}));
            conformanceModes.push_back(Tag("ConformanceMode", {GetConformanceMode(_s, _p, c) ? "true" : "false"}, 0, {condition}));

            for (auto const& p : _s.platforms)
                preprocessorDeffs.push_back(Tag("PreprocessorDefinitions", {GetPreprocessorDeff(_s, _p, c, p)}, 0,
                    {"Condition=\"'$(Configuration)|$(Platform)'=='" + c  + "|" + p + "'\""}));
        }

        std::string runTimeLib1 = Tag("RuntimeLibrary", {"MultiThreadedDebug"}, 0, {"Condition=\"'$(Configuration)'=='Debug'\""});
        std::string runTimeLib2 = Tag("RuntimeLibrary", {"MultiThreaded"}, 0, {"Condition=\"'$(Configuration)'=='Release'\""});
        std::string pchState = Tag("PrecompiledHeader", {"NotUsing"});

        std::vector<std::string> fields = { funcLvlLinking, intrinsicFuncs, sdl };
        for (auto const& w : warningLevels)      
            fields.push_back(w);
        for (auto const& t : treatWarnAsErrors)  
            fields.push_back(t);
        for (auto const& d : preprocessorDeffs)  
            fields.push_back(d);
        for (auto const& c : conformanceModes)   
            fields.push_back(c);
        fields.push_back(cppStd);
        fields.push_back(runTimeLib1);
        fields.push_back(runTimeLib2);
        fields.push_back(pchState);
        fields.push_back(includeDirs);

        return Tag("ClCompile", fields, 0, {});
    }
    
    std::string ItemDeffLink(Workspace& _w, ProjectManifest const& _p, ResolvedVendor const& _vendor)
    {
        std::string subsystem = "";
        if (_p.kind == "console")
            subsystem = Tag("Subsystem", {"Console"});
        else if (_p.kind == "window")
            subsystem = Tag("Subsystem", {"Application"});
        
        std::string comDatFold = Tag("EnableCOMDATFolding", {"true"}, 0, {"Condition=\"'$(Configuration)'=='Release'\""});
        std::string optRef = Tag("OptimizeReferences", {"true"}, 0, {"Condition=\"'$(Configuration)'=='Release'\""});
        std::string genDebugInfo = Tag("GenerateDebugInformation", {"true"});
        
        std::string addLibDirV = "";
        std::string addDepV = "";
        for (auto const& d : _p.dependencies)
        {
            ProjectManifest depM;
            if (Error e = _w.GetProjectManifest(d, depM))
                continue;
            
            if (depM.kind == "console" || depM.kind == "window")
                continue;
            
            std::string ext = ".lib";
            if (depM.kind == "dynamiclib")
                ext = ".dll";
            
            addLibDirV += "$(SolutionDir)" + _p.name + "\\$(Platform)\\$(Configuration);";
            addDepV += depM.name + ext + ";";
        }
        
        for (auto const& c : _vendor.libDirs)
            addLibDirV += AddRootRelative(c.second.string()) + ";";
        addLibDirV += "%(AdditionalLibraryDirectories)";
        
        for (auto const& c : _vendor.libs)
            addDepV += AddRootRelative(c) + ";";
        addDepV += "%(AdditionalDependencies)";
        
        std::string addLibDirD = Tag("AdditionalLibraryDirectories", {addLibDirV}, 0, { "Condition=\"'$(Configuration)'=='Debug'\""});
        std::string addLibDirR = Tag("AdditionalLibraryDirectories", {addLibDirV}, 0, { "Condition=\"'$(Configuration)'=='Release'\""});
        
        std::string addDepD = Tag("AdditionalDependencies", {addDepV}, 0, { "Condition=\"'$(Configuration)'=='Debug'\""});
        std::string addDepR = Tag("AdditionalDependencies", {addDepV}, 0, { "Condition=\"'$(Configuration)'=='Release'\""});
        
        return Tag("Link", {subsystem, comDatFold, optRef, genDebugInfo, addLibDirD, addLibDirR, addDepD, addDepR});
    }
    
    std::string ItemGrpClInclude(std::vector<fs::path> const& _includes)
    {
        std::vector<std::string> values;
        
        for (auto const& i : _includes)
            values.push_back("<ClInclude Include=\"" + AddRootRelative(i) + "\"/>");
        
        return Tag("ItemGroup", values, 1, {});
    }
    
    std::string ItemGrpClCompile(std::vector<fs::path> const& _compiles)
    {
        std::vector<std::string> values;
        
        for (auto const& i : _compiles)
            values.push_back("<ClCompile Include=\"" + AddRootRelative(i) + "\"/>");
        
        return Tag("ItemGroup", values, 1, {});
    }
    
    std::string ItemGrpNone(std::vector<fs::path> const& _nones, std::vector<fs::path> const& _res)
    {
        std::vector<std::string> values;
        for (auto const& i : _nones)
            values.push_back("<None Include=\"" + AddRootRelative(i) + "\"/>");
        for (auto const& i : _res)
            values.push_back("<None Include=\"" + AddRootRelative(i) + "\"/>");
        
        return Tag("ItemGroup", values, 1, {});
    }
    
    std::string ResourceCompile(std::vector<fs::path> const& _resCompiles)
    {
        std::vector<std::string> values;
        for (auto const& i : _resCompiles)
            values.push_back("<ResourceCompile Include=\"" + AddRootRelative(i) + "\"/>");
        return Tag("ItemGroup", values, 1, {});
    }
    
    std::string ProjectReferences(SolutionManifest const& _s, ProjectManifest const& _p)
    {
        std::vector<std::string> values;
        
        for (auto const& d : _p.dependencies)
        {
            std::string guid = Tag("Project", {Braced( GUID::GenerateGuid(_s.name, d)) });
            std::string path = "\"$(SolutionDir)\\" + d + "\\" + d + ".vcxproj\"";
            values.push_back(Tag("ProjectReferences", {guid}, 0, { "Include=" + path }));
        }
        
        return Tag("ItemGroup", values, 0, {});
    }
    
    std::string Footer()
    {
        std::string out = "    <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n";
        out += "    <ImportGroup Label=\"ExtensionTargets\" />\n";
        out += "</Project>\n";
        return out;
    }

}

Error Gen::GenVcProj(Workspace& _workspace, SolutionManifest const& _solution, ProjectManifest const& _project,
                      DependencyGraph& _graph, fs::path const& _path)
{
    fs::path root;
    if (Error e = Cmd::GetRootPath(root))
        return e;
    
    ResolvedVendor vendor;
    if (Error e = _graph.ResolveVendor(_project.name, vendor))
        return e;

    std::vector<fs::path> exports;
    if (Error e = _graph.ResolveExports(_project.name, exports))
        return e;

    ProjectFiles dis;
    if (Error e = Discovery::RunDiscovery(root / "src" / _project.name, root / "res" / _project.name, _project, dis))
        return e;
    
    std::string content = Header();
    content += ProjectConfigurations(_solution);
    content += Globals(_solution, _project);
    content += Configuration(_solution, _project);
    content += OutDirs();
    content += Vcpkg(_project);
    content += Tag("ItemDefinitionGroup", { ItemDeffClCompile(_solution, _project, vendor, exports, root),
                                                             ItemDeffLink(_workspace, _project, vendor)}, 1, {});
    content += ItemGrpClInclude(dis.clIncludeFiles);
    content += ItemGrpClCompile(dis.clCompileFiles);
    content += ItemGrpNone(dis.noneFiles, dis.resFiles);
    content += ResourceCompile(dis.resourcesCompileFiles);
    content += ProjectReferences(_solution, _project);
    content += Footer();

    if (Error e = FileHelper::WriteFileAtomic(_path, content))
        return e;

    fs::path filtersPath = _path;
    filtersPath += ".filters";
    return Gen::GenFilters(_solution, _project, dis, filtersPath);
}