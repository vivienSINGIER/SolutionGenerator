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
    using Gen::AddRootRelative;
    
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
                std::string config = Tag("Configuration", {c});
                std::string platform = Tag("Platform", {p});

                std::string v = Tag("ProjectConfiguration", {config, platform}, {"Include=\"" + c + "|" + p + "\""});
                values.push_back(v);
            }
        }

        return Tag("ItemGroup", values, {"Label=\"ProjectConfigurations\""});
    }

    std::string Globals(SolutionManifest const& _s, ProjectManifest const& _p)
    {
        std::string guid = Tag("ProjectGuid", {Braced(GUID::GenerateGuid(_s.name, _p.name))});
        std::string rNamespace = Tag("RootNamespace", {_p.name});
        std::string keyword = Tag("Keyword", {"Win32Proj"});

        return Tag("PropertyGroup", {guid, rNamespace, keyword}, {"Label=\"Globals\""});
    }

    std::string Configuration(SolutionManifest const& _s, ProjectManifest const& _p)
    {
        std::string pType = "Application";
        if (_p.kind == "staticlib")
            pType = "StaticLibrary";
        else if (_p.kind == "dynamiclib")
            pType = "DynamicLibrary";

        std::string type = Tag("ConfigurationType", {pType});
        std::string useDebugD = Tag("UseDebugLibraries", {"true"}, {"Condition=\"'$(Configuration)'=='Debug'\""});
        std::string useDebugR = Tag("UseDebugLibraries", {"false"}, {"Condition=\"'$(Configuration)'=='Release'\""});
        std::string toolset = Tag("PlatformToolset", {_s.toolset});
        std::string opti = Tag("WholeProgramOptimization", {"true"}, {"Condition=\"'$(Configuration)'=='Release'\""});
        std::string charSet = Tag("CharacterSet", {_s.characterSet});

        std::string localAppData = Tag("Import", {}, {
            "Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\"",
            "Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\"",
            "Label=\"LocalAppDataPlatform\""});

        std::string out = "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n";
        out += Tag("PropertyGroup", {type, useDebugD, useDebugR, toolset, opti, charSet}, {"Label=\"Configuration\""});
        out += "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n";
        out += "<ImportGroup Label=\"ExtensionSettings\" />\n";
        out += Tag("ImportGroup", {localAppData}, {"Label=\"PropertySheets\""});
        out += "<PropertyGroup Label=\"UserMacros\" />\n";
        return out;
    }

    std::string OutDirs()
    {
        std::string outDir = Tag("OutDir", {"$(ProjectDir)$(Platform)\\$(Configuration)\\"});
        std::string intDir = Tag("IntDir", {"$(ProjectDir)$(Platform)\\$(Configuration)\\intermediate\\"});

        return Tag("PropertyGroup", {outDir, intDir});
    }

    std::string Vcpkg(ProjectManifest const& _p)
    {
        std::string enabled = Tag("VcpkgEnabled", {"false"});
        std::string manifest = Tag("VcpkgEnableManifest", {"false"});
        std::string root = Tag("VcpkgManifestRoot", {"$(SolutionDir)..\\config\\vcpkg\\" + _p.name});
        std::string dir = Tag("VcpkgInstalledDir", {"$(SolutionDir)..\\vendor\\$(VcpkgTriplet)"});

        return Tag("PropertyGroup", {enabled, manifest, root, dir});
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

    // Joins the manifest's raw flags into a single AdditionalOptions value,
    // escaped for XML since flags may carry quotes (e.g. /D"NAME=\"v\"").
    std::string GetAdditionalOptions(std::vector<std::string> const& _options)
    {
        std::string out;
        for (auto const& o : _options)
        {
            for (char c : o)
            {
                switch (c)
                {
                    case '&':  out += "&amp;";  break;
                    case '<':  out += "&lt;";   break;
                    case '>':  out += "&gt;";   break;
                    default:   out += c;        break;
                }
            }
            out += " ";
        }
        out += "%(AdditionalOptions)";
        return out;
    }

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
        std::vector<std::string> fields;
        
        fields.push_back( Tag( "Optimization",                  {"Disabled"},           {"Condition=\"'$(Configuration)'=='Debug'\""} ) );
        fields.push_back( Tag( "Optimization",                  {"MaxSpeed"},           {"Condition=\"'$(Configuration)'=='Release'\""} ) );
        fields.push_back( Tag( "InlineFunctionExpansion",       {"Disabled"},           {"Condition=\"'$(Configuration)'=='Debug'\""} ) );
        fields.push_back( Tag( "InlineFunctionExpansion",       {"AnySuitable"},        {"Condition=\"'$(Configuration)'=='Release'\""} ) );
        fields.push_back( Tag( "IntrinsicFunctions",            {"false"},              {"Condition=\"'$(Configuration)'=='Debug'\""} ) );
        fields.push_back( Tag( "IntrinsicFunctions",            {"true"},               {"Condition=\"'$(Configuration)'=='Release'\""} ) );
        fields.push_back( Tag( "WholeProgramOptimization",      {"false"},              {"Condition=\"'$(Configuration)'=='Debug'\""} ) ); 
        fields.push_back( Tag( "WholeProgramOptimization",      {"true"},               {"Condition=\"'$(Configuration)'=='Release'\""} ) ); 
        fields.push_back( Tag( "RuntimeLibrary",                {"MultiThreadedDebug"}, {"Condition=\"'$(Configuration)'=='Debug'\""} ) );
        fields.push_back( Tag( "RuntimeLibrary",                {"MultiThreaded"},      {"Condition=\"'$(Configuration)'=='Release'\""} ) );
        fields.push_back( Tag( "BasicRuntimeLibrary",           {"EnableFastChecks"},   {"Condition=\"'$(Configuration)'=='Debug'\""} ) );
        fields.push_back( Tag( "BasicRuntimeLibrary",           {"Default"},            {"Condition=\"'$(Configuration)'=='Release'\""} ) );
        fields.push_back( Tag( "FunctionLevelLinking",          {"false"},              {"Condition=\"'$(Configuration)'=='Debug'\""} ) );
        fields.push_back( Tag( "FunctionLevelLinking",          {"true"},               {"Condition=\"'$(Configuration)'=='Release'\""} ) );
        fields.push_back( Tag( "SupportJustMyCode",             {"true"},               {"Condition=\"'$(Configuration)'=='Debug'\""} ) );
        fields.push_back( Tag( "SupportJustMyCode",             {"false"},              {"Condition=\"'$(Configuration)'=='Release'\""} ) );
        fields.push_back( Tag( "DebugInformationFormat",        {"ProgramDatabase"} ) );
        fields.push_back( Tag( "MultiProcessorCompilation",     {"true"} ) );
        fields.push_back( Tag( "SDLCheck",                      {"true"} ) );
        fields.push_back( Tag( "LanguageStandard",              {"stdcpp" + _s.cppStandard} ) );
        fields.push_back( Tag( "AdditionalIncludeDirectories",  {GetIncludeDirs(_p, _vendor, _exports, _root)} ) );
        fields.push_back( Tag( "PrecompiledHeader",             {"NotUsing"} ) );

        if (!_s.additionalOptions.compiler.empty())
            fields.push_back( Tag( "AdditionalOptions",         {GetAdditionalOptions(_s.additionalOptions.compiler)} ) );

        std::vector<std::string> warningLevels;
        std::vector<std::string> treatWarnAsErrors;
        std::vector<std::string> conformanceModes;
        std::vector<std::string> preprocessorDeffs;

        for (auto const& c : _s.configs)
        {
            std::string condition = "Condition=\"'$(Configuration)'=='" + c + "'\"";

            warningLevels.push_back(Tag("WarningLevel", {GetWarningLevel(_s, _p, c)}, {condition}));
            treatWarnAsErrors.push_back(Tag("TreatWarningAsError", {GetWarningsAsErrors(_s, _p, c) ? "true" : "false"}, {condition}));
            conformanceModes.push_back(Tag("ConformanceMode", {GetConformanceMode(_s, _p, c) ? "true" : "false"}, {condition}));

            for (auto const& p : _s.platforms)
                preprocessorDeffs.push_back(Tag("PreprocessorDefinitions", {GetPreprocessorDeff(_s, _p, c, p)},
                    {"Condition=\"'$(Configuration)|$(Platform)'=='" + c  + "|" + p + "'\""}));
        }
        
        for (auto const& w : warningLevels)      
            fields.push_back(w);
        for (auto const& t : treatWarnAsErrors)  
            fields.push_back(t);
        for (auto const& d : preprocessorDeffs)  
            fields.push_back(d);
        for (auto const& c : conformanceModes)   
            fields.push_back(c);

        return Tag("ClCompile", fields);
    }
    
    std::string ItemDeffLink(Workspace& _w, SolutionManifest const& _s, ProjectManifest const& _p,
                              ResolvedVendor const& _vendor)
    {
        std::vector<std::string> fields;
        
        std::string subsystem = "";
        if (_p.kind == "console")
            fields.push_back(Tag("Subsystem", {"Console"}));
        else if (_p.kind == "window")
            fields.push_back(Tag("Subsystem", {"Windows"}));
        
        fields.push_back(Tag("GenerateDebugInformation", {"true"}));
        fields.push_back(Tag("LinkIncrementAll", {"true"}, {"Condition=\"'$(Configuration)'=='Debug'"}));
        fields.push_back(Tag("LinkIncrementAll", {"false"}, {"Condition=\"'$(Configuration)'=='Release'"}));
        fields.push_back(Tag("OptimizeReferences", {"false"}, {"Condition=\"'$(Configuration)'=='Debug'\""}));
        fields.push_back(Tag("OptimizeReferences", {"true"}, {"Condition=\"'$(Configuration)'=='Release'\""}));
        fields.push_back(Tag("EnableCOMDATFolding", {"false"}, {"Condition=\"'$(Configuration)'=='Debug'\""}));
        fields.push_back(Tag("EnableCOMDATFolding", {"true"}, {"Condition=\"'$(Configuration)'=='Release'\""}));
        fields.push_back(Tag("LinkTimeCodeGeneration", {"Default"}, {"Condition=\"'$(Configuration)'=='Debug'"}));
        fields.push_back(Tag("LinkTimeCodeGeneration", {"UseLinkTimeCodeGeneration"}, {"Condition=\"'$(Configuration)'=='Release'"}));
        
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
        
        fields.push_back(Tag("AdditionalLibraryDirectories", {addLibDirV}, { "Condition=\"'$(Configuration)'=='Debug'\""}));
        fields.push_back(Tag("AdditionalLibraryDirectories", {addLibDirV}, { "Condition=\"'$(Configuration)'=='Release'\""}));

        fields.push_back(Tag("AdditionalDependencies", {addDepV}, { "Condition=\"'$(Configuration)'=='Debug'\""}));
        fields.push_back(Tag("AdditionalDependencies", {addDepV}, { "Condition=\"'$(Configuration)'=='Release'\""}));

        if (!_s.additionalOptions.linker.empty())
            fields.push_back(Tag("AdditionalOptions", {GetAdditionalOptions(_s.additionalOptions.linker)}));

        return Tag("Link", fields);
    }
    
    std::string ItemDeffPostBuild(ResolvedVendor const& _vendor)
    {
        fs::path root;
        if (Error e = Cmd::GetRootPath(root))
            return Tag("PostBuildEvent", {});
        
        if (_vendor.dlls.empty())
            return Tag("PostBuildEvent", {});
        
        std::string cmd;
        for (auto const& d : _vendor.dlls)
        {
            if (!cmd.empty())
                cmd += "&#10;";
            cmd += "xcopy /y /d /q \"" + AddRootRelative(fs::relative(d, root)) + "\" \"$(OutDir)\"";
        }
        
        return Tag("PostBuildEvent",  { Tag( "Command", {cmd } ), Tag(  "Message", {"Copying vendor DLLs"} ) } );
    }
    
    std::string ItemGrpClInclude(std::vector<fs::path> const& _includes)
    {
        std::vector<std::string> values;

        for (auto const& i : _includes)
            values.push_back(Tag("ClInclude", {}, {"Include=\"" + AddRootRelative(i) + "\""}));

        return Tag("ItemGroup", values);
    }

    std::string ItemGrpClCompile(std::vector<fs::path> const& _compiles)
    {
        std::vector<std::string> values;

        for (auto const& i : _compiles)
            values.push_back(Tag("ClCompile", {}, {"Include=\"" + AddRootRelative(i) + "\""}));

        return Tag("ItemGroup", values);
    }

    std::string ItemGrpNone(std::vector<fs::path> const& _nones, std::vector<fs::path> const& _res)
    {
        std::vector<std::string> values;
        for (auto const& i : _nones)
            values.push_back(Tag("None", {}, {"Include=\"" + AddRootRelative(i) + "\""}));
        for (auto const& i : _res)
            values.push_back(Tag("None", {}, {"Include=\"" + AddRootRelative(i) + "\""}));

        return Tag("ItemGroup", values);
    }

    std::string ResourceCompile(std::vector<fs::path> const& _resCompiles)
    {
        std::vector<std::string> values;
        for (auto const& i : _resCompiles)
            values.push_back(Tag("ResourceCompile", {}, {"Include=\"" + AddRootRelative(i) + "\""}));
        return Tag("ItemGroup", values);
    }

    std::string ProjectReferences(SolutionManifest const& _s, ProjectManifest const& _p)
    {
        std::vector<std::string> values;

        for (auto const& d : _p.dependencies)
        {
            std::string guid = Tag("Project", {Braced( GUID::GenerateGuid(_s.name, d)) });
            std::string path = "\"$(SolutionDir)" + d + "\\" + d + ".vcxproj\"";
            values.push_back(Tag("ProjectReference", {guid}, { "Include=" + path }));
        }

        return Tag("ItemGroup", values);
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
    
    std::string body;
    body += ProjectConfigurations(_solution);
    body += Globals(_solution, _project);
    body += Configuration(_solution, _project);
    body += OutDirs();
    body += Vcpkg(_project);
    body += Tag("ItemDefinitionGroup", { ItemDeffClCompile(_solution, _project, vendor, exports, root),
                                          ItemDeffLink(_workspace, _solution, _project, vendor),
                                          ItemDeffPostBuild(vendor)});
    body += ItemGrpClInclude(dis.clIncludeFiles);
    body += ItemGrpClCompile(dis.clCompileFiles);
    body += ItemGrpNone(dis.noneFiles, dis.resFiles);
    body += ResourceCompile(dis.resourcesCompileFiles);
    body += ProjectReferences(_solution, _project);
    body += "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n";
    body += "<ImportGroup Label=\"ExtensionTargets\" />\n";

    std::string content = Header();
    content += Gen::Indent(body);
    content += "</Project>\n";

    if (Error e = FileHelper::WriteFileAtomic(_path, content))
        return e;

    fs::path filtersPath = _path;
    filtersPath += ".filters";
    return Gen::GenFilters(_solution, _project, dis, filtersPath);
}