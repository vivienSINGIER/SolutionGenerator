#ifndef PROJECTMANIFEST_H_DEFINED
#define PROJECTMANIFEST_H_DEFINED

#include "Manifest.h"

class ProjectManifest
{
public:
    ProjectManifest();

    Error Serialize(fs::path const& _configPath);

    Error Deserialize(ArgBag const& _args);
    Error Deserialize(fs::path const& _configPath);
    
    int m_schemaVersion;

    std::string m_name;
    std::string m_kind;

    std::vector<std::string> m_dependencies;
    std::vector<std::string> m_exports;

    std::vector<std::string> m_defines;
    std::vector<std::string> m_includeDirs;
    std::vector<std::string> m_exclude;

    std::map<std::string, VendorDep> m_vendor;

    std::map<std::string, ConfigOverride> m_configOverrides;
};

#endif
