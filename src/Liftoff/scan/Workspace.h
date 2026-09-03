#ifndef WORKSPACE_H_DEFINED
#define WORKSPACE_H_DEFINED

#include "core/Error.h"
#include "model/ProjectManifest.h"
#include "model/SolutionManifest.h"

class Workspace 
{
public:
    Error Load(fs::path const& _exePath);
    Error Save();
    
    SolutionManifest& GetSolution();
    
    Error GetProjectManifest(std::string_view _projectName, ProjectManifest& _out);
    
private:
    SolutionManifest m_solution;
    std::vector<ProjectManifest> m_projects;
};

#endif
