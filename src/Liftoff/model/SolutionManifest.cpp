#include "SolutionManifest.h"

#include "io/FileHelper.h"

using Manifest::ReadRequired;
using Manifest::ReadOptional;

constexpr std::string_view validConfigurations[] =   { "Debug", "Release" };
constexpr std::string_view validPlatforms[] =        { "x64" };
constexpr std::string_view validToolsets[] =         { "v143" };
constexpr std::string_view validCppStandards[] =      { "20", "17", "14"};
constexpr std::string_view validCharacterSet[] =     { "UTF-8", "Unicode" };
constexpr std::string_view validWarningLvl[] =       { "W4", "W3", "W2", "W1", "W0" };

SolutionManifest::SolutionManifest()
{
    schemaVersion = MANIFEST_SCHEMA_VERSION;
    
    name = "MySolution";
    startProject = "";
    
    configs = { "Debug", "Release" };
    platform = "x64";
    toolset = "v143";
    cppStandard = "20";
    
    characterSet = "Unicode";
    conformanceMode = true;
    warningLvl = "W4";
    warningsAsErrors = false;
}

Error SolutionManifest::Serialize(fs::path const& _manifestPath)
{
    js c;
    
    if (Error e = ValidateManifest())
        return e;
    
    c["schemaVersion"] = schemaVersion;
    
    c["name"] = name;
    c["startProject"] = startProject;
    
    c["configs"] = configs;
    c["platform"] = platform;
    c["toolset"] = toolset;
    c["cppStandard"] = cppStandard;
    
    c["characterSet"] = characterSet;
    c["conformanceMode"] = conformanceMode;
    c["warningLevel"] = warningLvl;
    c["warningsAsErrors"] = warningsAsErrors;
    
    c["defines"] = defines;
    c["includeDirs"] = includeDirs;
    
    c["configOverrides"] = js::object();
    for (auto const& pair : configOverrides)
    {
        std::string const& config = pair.first;
        ConfigOverride const& override = pair.second;
        
        if (override.overrides[0])
            c["configOverrides"][config]["warningLevel"] = override.warningLvl;
        if (override.overrides[1])
            c["configOverrides"][config]["warningsAsErrors"] = override.warningsAsErrors;
        if (override.overrides[2])
            c["configOverrides"][config]["conformanceMode"] = override.conformanceMode;
        if (override.overrides[3])
            c["configOverrides"][config]["defines"] = override.defines;
        if (override.overrides[4])
            c["configOverrides"][config]["includeDirs"] = override.includeDirs;
    }
    
    c["projects"] = js::array();
    for (auto const& p : projects)
        c["projects"].push_back({ {"name", p.name}, {"group", p.group} });
    
    return FileHelper::WriteFileAtomic(_manifestPath, c.dump(4));
}

Error SolutionManifest::Deserialize(ArgBag const& _args)
{
    if (_args.Exists("name"))
        name = _args.Get("name");
    else
        return Error::Validation("missing required argument 'name'");
    
    if (_args.Exists("toolset"))
        toolset = _args.Get("toolset");
    
    if (_args.Exists("cppStandard"))
        cppStandard = _args.Get("cppStandard");
    
    if (Error e = ValidateManifest())
        return e;
    
    return Error::Ok();
}

Error SolutionManifest::Deserialize(fs::path const& _configPath)
{
    std::string str;
    Error er = FileHelper::ReadFile(_configPath, str);
    if (er) return er;
    
    js j;
    
    try { j = js::parse(str); }
    catch (js::exception&) { return Error::Validation("unable to parse solution manifest file"); }
    
    Error err = Error::Validation("Invalid solution manifest");

    int version = 0;
    if (ReadRequired(j, "schemaVersion", version, err) && version != MANIFEST_SCHEMA_VERSION)
        err.details.push_back("Outdated schemaVersion, please update your solution");
    schemaVersion = version;

    ReadRequired(j, "name",             name,             err);
    ReadRequired(j, "configs",          configs,          err);
    ReadRequired(j, "platform",         platform,         err);
    ReadRequired(j, "toolset",          toolset,          err);
    ReadRequired(j, "cppStandard",      cppStandard,      err);
    ReadRequired(j, "characterSet",     characterSet,     err);
    ReadRequired(j, "conformanceMode",  conformanceMode,  err);
    ReadRequired(j, "warningLevel",     warningLvl,       err);
    ReadRequired(j, "warningsAsErrors", warningsAsErrors, err);
    ReadRequired(j, "defines",          defines,          err);
    ReadRequired(j, "includeDirs",      includeDirs,      err);

    ReadOptional(j, "startProject", startProject);

    if (j.contains("configOverrides"))
    {
        js const& overrides = j["configOverrides"];
        
        for (auto& [configName, settings] : overrides.items()) 
        {
            if (configName != "Release" && configName != "Debug") {
                err.details.push_back("incorrect config name '" + configName + "'. must be 'Release' or 'Debug'");
            }

            ConfigOverride o;
            if (settings.contains("warningLevel"))
            {
                o.warningLvl = settings["warningLevel"];
                o.overrides[0] = true;
            }
            if (settings.contains("warningsAsErrors"))
            {
                o.warningsAsErrors = settings["warningsAsErrors"];
                o.overrides[1] = true;
            }
            if (settings.contains("conformanceMode"))
            {
                o.conformanceMode = settings["conformanceMode"];
                o.overrides[2] = true;
            }
            if (settings.contains("defines"))
            {
                o.defines = settings["defines"];
                o.overrides[3] = true;
            }
            if (settings.contains("includeDirs"))
            {
                o.includeDirs = settings["includeDirs"];
                o.overrides[4] = true;
            }
            
            configOverrides[configName] = o;
        }
    }
    else 
        err.details.push_back("Missing required argument 'configOverrides'");

    if (j.contains("projects"))
    {
        for (auto& p : j.at("projects"))
        {
            ProjectDesc project;
            if (!p.contains("name"))
                err.details.push_back("Missing required project argument 'name'");
            else
                project.name = p["name"];
            
            if (!p.contains("group"))
                err.details.push_back("Missing required project argument 'group'");
            else
                project.group = p["group"];
            
            projects.push_back(project);
        }
    }
    else 
        err.details.push_back("Missing required argument 'project'");
    
    if (!err.details.empty())
        return err;
        
    if (Error e = ValidateManifest())
        return e;
    
    return Error::Ok();
}

Error SolutionManifest::ValidateManifest()
{
    Error err = Error::Validation("unvalid manifest values detected");
    
    for (std::string& c : configs)
    {
        if (Error e = ValidateValues(validConfigurations, c, "configs"))
            err.details.push_back(e.Format());
    }
    
    if (Error e = ValidateValues(validCharacterSet, characterSet, "characterSet"))
        err.details.push_back(e.Format());
    
    if (Error e = ValidateValues(validPlatforms, platform, "platform"))
        err.details.push_back(e.Format());
    
    if (Error e = ValidateValues(validToolsets, toolset, "toolset"))
        err.details.push_back(e.Format());
    
    if (Error e = ValidateValues(validCppStandards, cppStandard, "cppStandard"))
        err.details.push_back(e.Format());
    
    if (Error e = ValidateValues(validWarningLvl, warningLvl, "warningLevel"))
        err.details.push_back(e.Format());
    
    bool found = false;
    for (auto& p : projects)
    {
        if (startProject == p.name)
        {
            found = true;
            break;
        }
    }
    
    if (startProject != "" && found == false)
        err.details.push_back("selected start project'" + startProject + "' doesn't exist");
    
    if (err.details.empty() == false)
        return err;
    return Error::Ok();
}




