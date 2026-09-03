#ifndef SOLUTIONMANIFEST_H_DEFINED
#define SOLUTIONMANIFEST_H_DEFINED

#include "Manifest.h"

class SolutionManifest 
{
public:
    SolutionManifest();
    
    Error Serialize(fs::path const& _configPath);
    
    Error Deserialize(ArgBag const& _args);
    Error Deserialize(fs::path const& _configPath);
    
    int m_schemaVersion;
    
    std::string         m_name;
    std::string m_startProject;
    
    std::vector<std::string> m_configs;
    std::string m_platform;
    std::string m_toolset;
    std::string m_cppStandard;
    
    std::string m_characterSet;
    bool m_conformanceMode;
    std::string m_warningLvl;
    bool m_warningsAsErrors;
    
    std::vector<std::string> m_defines;
    std::vector<std::string> m_includeDirs;
    
    std::map<std::string, ConfigOverride> m_configOverrides;
    
    std::vector<ProjectDesc> m_projects;
};

#endif
