#ifndef WORKSPACE_H_DEFINED
#define WORKSPACE_H_DEFINED

#include "core/Error.h"
#include "graph/DependencyGraph.h"
#include "model/ProjectManifest.h"
#include "model/SolutionManifest.h"

class Workspace 
{
public:
    Error Load(fs::path const& _manifestPath);
    Error Save(fs::path const& _configPath);
    
    SolutionManifest& GetSolution();
    
    Error AddProject(ProjectManifest _projectManifest);
    
    std::vector<ProjectManifest>& GetProjects();
    bool HasProject(std::string_view _projectName);
    ProjectManifest* GetProjectManifest(std::string_view _projectName);

    DependencyGraph& GetDependencyGraph();
    
private:
    SolutionManifest m_solution;
    std::vector<ProjectManifest> m_projects;
    DependencyGraph m_depGraph;
};

#endif
