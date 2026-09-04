#include "DependencyGraph.h"

DependencyGraph::DependencyGraph(std::vector<ProjectManifest> const& _projects)
{
    for (auto const& p : _projects)
    {
        m_projects[p.name] = &p;
    }
}

Error DependencyGraph::ValidateGraph()
{
    if (Error e = HasHoles())
        return e;
    if (Error e = HasLoops())
        return e;
    
    return Error::Ok();
}

Error DependencyGraph::HasHoles()
{
    for (auto const& p : m_projects)
    {
        std::vector<std::string> validDependencies; 
        ProjectManifest const& m = *p.second;
        for (auto const& d : m.dependencies)
        {
            if (d == p.first)
                return Error::Validation("unvalid dependency list, a project can't depend on itself");
            if (Find(d) == false)
                return Error::Validation("unvalid dependency list: couldn't find project '" + d + "'");
            
            for (auto const& v : validDependencies)
                if (v == d)
                    return Error::Validation("unvalid dependency list: '" + d + "' is present too many times");
            
            validDependencies.push_back(d);
        }
    }
    
    m_path.clear();
    return Error::Ok();
}

Error DependencyGraph::HasLoops()
{
    for (auto const& p : m_projects)
    {
        if (m_nodeState[p.first] == NodeState::Unvisited)
        {
            if (Error e = Visit(p.first))
                return e;
        }
    }
    return Error::Ok();
}

Error DependencyGraph::Visit(std::string _name)
{
    m_nodeState[_name] = NodeState::InProgress;
    m_path.push_back(_name);
    for (auto const& n : m_projects[_name]->dependencies)
    {
        if (m_nodeState[n] == NodeState::InProgress)
        {
            Error err = Error::Validation("dependency loop detected");
            std::string rep = m_path.front();
            for (size_t i = 1; i < m_path.size(); i++)
            {
                rep += " -> " + m_path.at(i);
            }
            err.details.push_back(rep);
            return err;
        }
        
        if (m_nodeState[n] == NodeState::Unvisited)
            Visit(n);
    }
    m_path.pop_back();
    m_nodeState[_name] = NodeState::Visited;
    return Error::Ok();
}

bool DependencyGraph::Find(std::string_view _pName)
{
    for (auto& p : m_projects)
    {
        if (p.first == _pName)
            return true;
    }
    return false;
}
