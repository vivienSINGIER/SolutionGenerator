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
    
    std::string Braced(std::string const& _guid);

    // Prefixes every line of _block with _levels * 4 spaces. Used to nest an
    // already-rendered (possibly multi-line) child one level deeper than its
    // parent tag, without either side needing to know the absolute depth.
    std::string Indent(std::string const& _block, int _levels = 1);

    // Renders <_tag _tagArgs...>_values...</_tag>, one _values entry per
    // line, each indented one level deeper than _tag itself. Self-closes
    // (<_tag _tagArgs.../>) when _values is empty.
    std::string Tag(std::string const& _tag, std::vector<std::string> const& _values = {},
                        std::vector<std::string> const& _tagArgs = {});
    std::string AddRootRelative(fs::path const& _dir);
}

#endif
