#include "Gen.h"
#include "Guid.h"

#include "commands/Command.h"
#include "graph/DependencyGraph.h"
#include "io/FileHelper.h"

namespace
{
    std::string Braced(std::string const& _guid)
    {
        return "{" + _guid + "}";
    }
    
    std::string Tag(std::string const& _tag, std::vector<std::string> const& _values, int _indentCount = 0, std::vector<std::string> const& _tagArgs = {})
    {
        std::string out;
        std::string indent;
        
        for (int i = 0; i < _indentCount; i++)
            indent += "    ";
        out += indent + "<" + _tag;
        
        for (auto const& v : _tagArgs)
            out += " " + v;
        out += ">";
        
        if (_values.size() > 1)
        {
            out += "\n";
            for (auto const& v : _values)
                out += indent + "    " + v + "\n";
        }
        else if (_values.size() == 1)
            out += _values[0];
        out += "</" + _tag + ">\n";
        return out;
    }
    
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
        out += Tag("PropertyGroup", {type, useDebugD, useDebugR, toolset, opti, charSet}, 1, {"Label=\"" + std::string("Configuation") + "\""});
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
    
    void AddRootRelative(fs::path const& _dir, std::vector<std::string>& _out)
    {
        _out.push_back("$(SolutionDir)..\\" + FileHelper::ToBackSlashes(_dir));
    }
    
    std::string GetIncludeDirs(ProjectManifest const& _p, ResolvedVendor const& _vendor,
                                std::vector<fs::path> const& _exports, fs::path const& _root)
    {
        std::vector<std::string> dirs;

        AddRootRelative(fs::path("src") / _p.name, dirs);

        for (auto const& dir : _exports)
            AddRootRelative(fs::relative(dir, _root), dirs);
        for (auto const& dir : _vendor.includeDirs)
            AddRootRelative(fs::relative(dir, _root), dirs);

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

        return Tag("ClCompile", fields, 2, {});
    }
    
    std::string ItemDeffLink(SolutionManifest const& _s, ProjectManifest const& _p, ResolvedVendor const& _vendor)
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
        for (auto const& c : _vendor.libDirs)
            addLibDirV += c.second.string() + ";";
        addLibDirV += "%(AdditionalLibraryDirectories)";
        
        std::string addLibDirD = Tag("AdditionalLibraryDirectories", {addLibDirV}, 0, { "Condition=\"'$(Configuration)'=='Debug'\""});
        std::string addLibDirR = Tag("AdditionalLibraryDirectories", {addLibDirV}, 0, { "Condition=\"'$(Configuration)'=='Release'\""});
        
        std::string addDepV = "";
        for (auto const& c : _vendor.libs)
            addDepV += c + ";";
        addDepV += "%(AdditionalDependencies)";
        
        std::string addDepD = Tag("AdditionalDependencies", {addDepV}, 0, { "Condition=\"'$(Configuration)'=='Debug'\""});
        std::string addDepR = Tag("AdditionalDependencies", {addDepV}, 0, { "Condition=\"'$(Configuration)'=='Release'\""});
    }

}

Error Gen::GenVcProj(SolutionManifest const& _solution, ProjectManifest const& _project,
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

    std::string content = Header();
    content += ProjectConfigurations(_solution);
    content += Globals(_solution, _project);
    content += Configuration(_solution, _project);
    content += OutDirs();
    content += Vcpkg(_project);
    content += Tag("ItemDefinitionGroup", { ItemDeffClCompile(_solution, _project, vendor, exports, root) }, 1, {});

    return FileHelper::WriteFileAtomic(_path, content);
}