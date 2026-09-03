#include "Workspace.h"

Error Workspace::Load(fs::path const& _exePath)
{
    return Error::Ok();
}

Error Workspace::Save()
{
    return Error::Ok();
}

SolutionManifest& Workspace::GetSolution()
{
    return m_solution;
}

Error Workspace::GetProjectManifest(std::string_view _projectName, ProjectManifest& _out)
{
    for (ProjectManifest& project: m_projects)
    {
        if (project.m_name == _projectName)
        {
            _out = project;
            return Error::Ok();
        }   
    }
    
    return Error::Validation("no project of name '" + std::string(_projectName) + "' found");
}
