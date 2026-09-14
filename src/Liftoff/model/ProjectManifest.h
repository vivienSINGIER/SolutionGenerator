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
    
    int schemaVersion;

    std::string name;
    std::string kind;

    std::vector<std::string> dependencies;
    std::vector<std::string> exports;

    std::vector<std::string> defines;
    std::vector<std::string> exclude;

    std::map<std::string, VendorDep> vendor;

    std::map<std::string, ConfigOverride> configOverrides;
};

#endif
