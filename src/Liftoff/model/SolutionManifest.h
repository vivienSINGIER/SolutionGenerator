#ifndef SOLUTIONMANIFEST_H_DEFINED
#define SOLUTIONMANIFEST_H_DEFINED

#include <string>
#include <vector>
#include <filesystem>

#include "core/Error.h"
#include "vendor/nlohmann/json.hpp"
#include "cli/ArgBag.h"

namespace fs = std::filesystem;
using js = nlohmann::json;

struct ProjectDesc
{
    std::string name;
    std::string group;
};

struct ConfigOverride
{
    std::string warningLvl;
    bool warningsAsErrors;
    bool conformanceMode;
    
    std::vector<std::string> defines;
    std::vector<std::string> includeDirs;
    
    bool overrides[5];
};

class SolutionManifest 
{
public:
    SolutionManifest();
    
    Error Serialize(fs::path const& _configPath);
    
    Error Deserialize(ArgBag const& _args);
    Error Deserialize(js const& _js);
    
private:
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
