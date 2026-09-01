#include "SolutionManifest.h"

#include "io/FileHelper.h"

SolutionManifest::SolutionManifest()
{
    m_schemaVersion = 1;
    
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
    
    for (auto const& p : m_projects)
    {
        c["projects"]["name"] = p.name;
        c["projects"]["group"] = p.group;
    }
    
    FileHelper::WriteFileAtomic(_configPath, c.dump(4));
    
    return Error::Ok();
}




