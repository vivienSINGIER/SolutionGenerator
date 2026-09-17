#ifndef GEN_H_DEFINED
#define GEN_H_DEFINED

#include "core/Error.h"
#include "model/ProjectManifest.h"
#include "model/SolutionManifest.h"
#include "graph/DependencyGraph.h"
#include "scan/SourceDiscovery.h"
#include "scan/Workspace.h"

namespace Gen
{
    Error GenSln(SolutionManifest const& _solution, fs::path const& _path);
    Error GenVcProj(Workspace& _workspace, SolutionManifest const& _solution,
                        ProjectManifest const& _project,
                        DependencyGraph& _graph, fs::path const& _path);
    Error GenFilters(SolutionManifest const& _solution, ProjectManifest const& _project,
                        ProjectFiles const& _files, fs::path const& _path);

    // Shared XML-fragment helpers used across the vcxproj/filters generators.
    std::string Braced(std::string const& _guid);
    std::string Tag(std::string const& _tag, std::vector<std::string> const& _values,
                        int _indentCount = 0, std::vector<std::string> const& _tagArgs = {});
    std::string AddRootRelative(fs::path const& _dir);
}

#endif
