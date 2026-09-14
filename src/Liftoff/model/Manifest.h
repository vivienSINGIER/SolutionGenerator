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

struct VendorDep
{
    std::vector<std::string> include;   // include dirs to add for consumers of this project
    std::vector<std::string> libs;      // .lib files to link
    std::map<std::string, std::string> libDirs;   // config name -> lib search path
    std::vector<std::string> dlls;      // dlls to copy next to the executable
};

struct ConfigOverride
{
    std::string warningLvl;
    bool warningsAsErrors = false;
    bool conformanceMode = false;
    
    std::vector<std::string> defines;

    bool overrides[4] = { false, false, false, false };
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
