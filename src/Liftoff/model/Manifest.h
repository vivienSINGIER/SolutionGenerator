#ifndef MANIFEST_H_DEFINED
#define MANIFEST_H_DEFINED

#include <string>
#include <vector>
#include <map>
#include <filesystem>

#include "core/Error.h"
#include "vendor/nlohmann/json.hpp"
#include "cli/ArgBag.h"

#define MANIFEST_SCHEMA_VERSION 1

namespace fs = std::filesystem;
using js = nlohmann::ordered_json;

struct ProjectDesc
{
    std::string name;
    std::string group;
};

struct VendorConfig
{
    std::vector<std::string> libs;
    std::vector<std::string> libDirs;
    std::vector<std::string> dlls;
};

// libs / libDirs / dlls apply to every config; configs adds to them for one config.
struct VendorDep
{
    std::vector<std::string> include;
    std::vector<std::string> libs;
    std::vector<std::string> libDirs;
    std::vector<std::string> dlls;
    std::map<std::string, VendorConfig> configs;
};

struct ConfigOverride
{
    std::string warningLvl;
    bool warningsAsErrors = false;
    bool conformanceMode = false;
    
    std::vector<std::string> defines;

    bool overrides[4] = { false, false, false, false };
};

struct AdditionalOptions
{
    std::vector<std::string> compiler;
    std::vector<std::string> linker;
};

namespace Manifest
{
    template <typename T>
    bool ReadRequired(js const& _js, char const* _key, T& _out, Error& _err);
    
    template <typename T>
    void ReadOptional(js const& _js, char const* _key, T& _out);
}

#include "Manifest.inl"

#endif
