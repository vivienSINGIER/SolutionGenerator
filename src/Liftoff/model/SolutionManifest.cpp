#include "SolutionManifest.h"

#include "io/FileHelper.h"

using Manifest::ReadRequired;
using Manifest::ReadOptional;

SolutionManifest::SolutionManifest()
{
    m_schemaVersion = MANIFEST_SCHEMA_VERSION;
    
    m_name = "MySolution";
    m_startProject = "";
    
    m_configs = { "Debug", "Release" };
    m_platform = "x64";
    m_toolset = "v143";
    m_cppStandard = "20";
    
    m_characterSet = "Unicode";
    m_conformanceMode = true;
    m_warningLvl = "W4";
    m_warningsAsErrors = false;
}

Error SolutionManifest::Serialize(fs::path const& _configPath)
{
    js c;
    
    c["schemaVersion"] = m_schemaVersion;
    
    c["name"] = m_name;
    c["startProject"] = m_startProject;
    
    c["configs"] = m_configs;
    c["platform"] = m_platform;
    c["toolset"] = m_toolset;
    c["cppStandard"] = m_cppStandard;
    
    c["characterSet"] = m_characterSet;
    c["conformanceMode"] = m_conformanceMode;
    c["warningLevel"] = m_warningLvl;
    c["warningsAsErrors"] = m_warningsAsErrors;
    
    c["defines"] = m_defines;
    c["includeDirs"] = m_includeDirs;
    
    c["configOverrides"] = js::object();
    for (auto const& pair : m_configOverrides)
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
    for (auto const& p : m_projects)
        c["projects"].push_back({ {"name", p.name}, {"group", p.group} });
    
    return FileHelper::WriteFileAtomic(_configPath, c.dump(4));
}

Error SolutionManifest::Deserialize(ArgBag const& _args)
{
    if (_args.Exists("name"))
        m_name = _args.Get("name");
    else
        return Error::Validation("missing required argument 'name'");
    
    if (_args.Exists("toolset"))
        m_toolset = _args.Get("toolset");
    
    if (_args.Exists("cppStandard"))
        m_cppStandard = _args.Get("cppStandard");
    
    return Error::Ok();
}

Error SolutionManifest::Deserialize(fs::path const& _configPath)
{
    std::string str;
    Error e = FileHelper::ReadFile(_configPath, str);
    if (e) return e;
    
    js j;
    
    try { j = js::parse(str); }
    catch (js::exception&) { return Error::Validation("unable to parse solution manifest file"); }
    
    Error err = Error::Validation("Invalid solution manifest");

    int version = 0;
    if (ReadRequired(j, "schemaVersion", version, err) && version != MANIFEST_SCHEMA_VERSION)
        err.details.push_back("Outdated schemaVersion, please update your solution");
    m_schemaVersion = version;

    ReadRequired(j, "name",             m_name,             err);
    ReadRequired(j, "configs",          m_configs,          err);
    ReadRequired(j, "platform",         m_platform,         err);
    ReadRequired(j, "toolset",          m_toolset,          err);
    ReadRequired(j, "cppStandard",      m_cppStandard,      err);
    ReadRequired(j, "characterSet",     m_characterSet,     err);
    ReadRequired(j, "conformanceMode",  m_conformanceMode,  err);
    ReadRequired(j, "warningLevel",     m_warningLvl,       err);
    ReadRequired(j, "warningsAsErrors", m_warningsAsErrors, err);
    ReadRequired(j, "defines",          m_defines,          err);
    ReadRequired(j, "includeDirs",      m_includeDirs,      err);

    ReadOptional(j, "startProject", m_startProject);

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
            
            m_configOverrides[configName] = o;
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
            
            m_projects.push_back(project);
        }
    }
    else 
        err.details.push_back("Missing required argument 'project'");
    
    if (!err.details.empty())
        return err;
        
    return Error::Ok();
}




