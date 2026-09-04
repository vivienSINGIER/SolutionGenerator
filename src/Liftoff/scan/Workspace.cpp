#include "Workspace.h"

#include "graph/DependencyGraph.h"

Error Workspace::Load(fs::path const& _manifestPath)
{
    if (Error e = m_solution.Deserialize(_manifestPath))
        return e;
    
    fs::path projectManifestDir = _manifestPath.parent_path()  / "projects";
    for (ProjectDesc const& p : m_solution.projects)
    {
        ProjectManifest project;
        if (Error e = project.Deserialize( projectManifestDir / (p.name + ".json") ))
            return e;
        m_projects.push_back(project);
    }
    
    return Error::Ok();
}

Error Workspace::Save(fs::path const& _configPath)
{
    DependencyGraph g(m_projects);
    
    for (ProjectDesc const& d : m_solution.projects)
    {
        bool found = false;
        for (ProjectManifest const& p : m_projects)
        {
            if (p.name == d.name)
                found = true;
        }
        
        if (!found)
            return Error::Validation("project '" + d.name + "' not found");
    }
    
    if (Error e = g.ValidateGraph())
        return e;
    
    if (Error e = m_solution.Serialize(_configPath / (m_solution.name + ".json")))
        return e;
    for (ProjectManifest& p : m_projects)
    {
        if (Error e = p.Serialize(_configPath / "projects" / (p.name + ".json")))
            return e;
    }
    
    return Error::Ok();
}

SolutionManifest& Workspace::GetSolution()
{
    return m_solution;
}

Error Workspace::AddProject(ProjectManifest _projectManifest)
{
    if (HasProject(_projectManifest.name))
        return Error::Validation("project '" + _projectManifest.name + "' already exists");
    
    ProjectDesc desc;
    desc.name = _projectManifest.name;
    desc.group = "Apps";
    if (_projectManifest.kind == "dynamiclib" || _projectManifest.kind == "staticlib")
        desc.group = "Libs";
    
    m_solution.projects.push_back(desc);
    m_projects.push_back(_projectManifest);
    return Error::Ok();
}

std::vector<ProjectManifest>& Workspace::GetProjects()
{
    return m_projects;
}

bool Workspace::HasProject(std::string_view _projectName)
{
    for (ProjectManifest const& p : m_projects)
        if (p.name == _projectName)
            return true;
    
    return false;
}

Error Workspace::GetProjectManifest(std::string_view _projectName, ProjectManifest& _out)
{
    for (ProjectManifest& project: m_projects)
    {
        if (project.name == _projectName)
        {
            _out = project;
            return Error::Ok();
        }   
    }
    
    return Error::Validation("no project of name '" + std::string(_projectName) + "' found");
}
