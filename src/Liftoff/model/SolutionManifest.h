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
    
    int schemaVersion;
    
    std::string         name;
    std::string m_startProject;
    
    std::vector<std::string> configs;
    std::string platform;
    std::string toolset;
    std::string cppStandard;
    
    std::string characterSet;
    bool conformanceMode;
    std::string warningLvl;
    bool warningsAsErrors;
    
    std::vector<std::string> defines;
    std::vector<std::string> includeDirs;
    
    std::map<std::string, ConfigOverride> configOverrides;
    
    std::vector<ProjectDesc> projects;
};

#endif
