#ifndef SOURCE_DISCOVERY_H_DEFINED
#define SOURCE_DISCOVERY_H_DEFINED

#include <vector>
#include <filesystem>

#include "core/Error.h"
#include "model/ProjectManifest.h"

namespace fs = std::filesystem;

namespace Discovery
{
    struct ProjectFiles
    {
        std::vector<fs::path> clCompileFiles;
        std::vector<fs::path> clIncludeFiles;
        std::vector<fs::path> resourcesCompileFiles;
        std::vector<fs::path> noneFiles;
        std::vector<fs::path> resFiles;
    };
    
    Error RunDiscovery(fs::path const& _src, fs::path const& _res, ProjectManifest const& _manifest, ProjectFiles& _out);
    
    Error GetSourceFiles(fs::path const& _src, ProjectManifest const& _manifest, ProjectFiles& _out);
    void ClassifyFile(ProjectFiles& _out, fs::path const& _file);
    Error ApplyExcludes(std::vector<std::string> _excludes, ProjectFiles& _out);
    Error GetResFiles(fs::path const& _res, ProjectFiles& _out);
};

#endif
