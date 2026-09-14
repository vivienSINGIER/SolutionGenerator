#ifndef GEN_H_DEFINED
#define GEN_H_DEFINED

#include "core/Error.h"
#include "model/ProjectManifest.h"
#include "model/SolutionManifest.h"
#include "graph/DependencyGraph.h"

namespace Gen
{
    Error GenSln(SolutionManifest const& _solution, fs::path const& _path);
    Error GenVcProj(SolutionManifest const& _solution, ProjectManifest const& _project,
                     DependencyGraph& _graph, fs::path const& _path);
}

#endif
