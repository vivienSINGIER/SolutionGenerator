#include "DependencyGraph.h"

#include <algorithm>

#include "commands/Command.h"

namespace
{
    template <typename T>
    void AppendUnique(std::vector<T>& _vec, T const& _value)
    {
        if (std::find(_vec.begin(), _vec.end(), _value) == _vec.end())
            _vec.push_back(_value);
    }
}

DependencyGraph::DependencyGraph(std::vector<ProjectManifest> const& _projects)
{
    m_graph.reserve(_projects.size());
    for (auto const& p : _projects)
    {
        Node n;
        n.project = &p;
        m_graph[p.name] = n;
    }
}

Error DependencyGraph::BuildGraph()
{
    for (auto& n : m_graph)
    {
        for (std::string const& d : n.second.project->dependencies)
        {
            if (d == n.second.project->name)
                return Error::Validation("invalid dependency list: project '" + d + "' cannot depend on itself");
            
            auto it = m_graph.find(d);
            if (it == m_graph.end())
                return Error::Validation("invalid dependency list: couldn't find project '" + d + "'");

            n.second.dependencies.push_back(&it->second);
        }
    }

    return Error::Ok();
}

Error DependencyGraph::ValidateGraph()
{
    for (auto& pair : m_graph)
    {
        pair.second.dependencies.clear();
        pair.second.state = NodeState::Unvisited;
    }

    if (Error e = BuildGraph())
        return e;
    if (Error e = HasLoops())
        return e;
    
    return Error::Ok();
}

Error DependencyGraph::ResolveExports(std::string _pName, std::vector<fs::path>& _exports)
{
    if (Error e = ValidateGraph())
        return e;

    // HasLoops() leaves every node Visited; reset before TravelExports walks the graph.
    for (auto& pair : m_graph)
        pair.second.state = NodeState::Unvisited;

    _exports.clear();

    fs::path root;
    if (Error e = Cmd::GetRootPath(root))
        return e;
    
    if (FileHelper::IsDir(root / "src" / _pName) == false)
        return Error::Io("directory '" + (root / "src" / _pName).string() + "' doesn't exist");

    if (Error e = TravelExports(_pName, root, _exports))
        return e;

    return Error::Ok();
}

Error DependencyGraph::ResolveVendor(std::string _pName, ResolvedVendor& _out)
{
    if (Error e = ValidateGraph())
        return e;

    // HasLoops() leaves every node Visited; reset before TravelVendor walks the graph.
    for (auto& pair : m_graph)
        pair.second.state = NodeState::Unvisited;

    _out = ResolvedVendor{};

    fs::path root;
    if (Error e = Cmd::GetRootPath(root))
        return e;

    return TravelVendor(_pName, root / "vendor", _out);
}

Error DependencyGraph::HasLoops()
{
    for (auto& pair : m_graph)
    {
        Node& n = pair.second;
        if (n.state == NodeState::Unvisited)
        {
            if (Error e = Visit(n.project->name))
                return e;
        }
    }
    return Error::Ok();
}

Error DependencyGraph::Visit(std::string _name)
{
    Node& n  = m_graph[_name];
    n.state = NodeState::InProgress;
    m_path.push_back(_name);
    for (auto& d : n.project->dependencies)
    {
        if (m_graph[d].state == NodeState::InProgress)
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
        
        if (m_graph[d].state == NodeState::Unvisited)
        {
            if (Error e = Visit(d))
                return e;
        }
    }
    m_path.pop_back();
    n.state = NodeState::Visited;
    return Error::Ok();
}

Error DependencyGraph::TravelExports(std::string const& _name, fs::path const& _root, std::vector<fs::path>& _exports)
{
    if (m_graph.contains(_name) == false)
        return Error::Validation("project '" + _name + "' isn't in the dependency graph");
    
    Node& n = m_graph[_name];
    
    if (n.state == NodeState::Visited)
        return Error::Ok();
    
    for (auto& d : n.project->dependencies)
    {
        if (Error e = TravelExports(d, _root, _exports))
            return e;
    }
    
    for (auto& e : n.project->exports)
    {
        fs::path p = _root / "src" / _name / e;
        if (FileHelper::IsDir(p) == false)
            return Error::Io("directory '" + p.string() + "' doesn't exist");
        
        _exports.push_back(p);
    }
    
    n.state = NodeState::Visited;

    return Error::Ok();
}

Error DependencyGraph::TravelVendor(std::string const& _name, fs::path const& _vendorRoot, ResolvedVendor& _out)
{
    if (m_graph.contains(_name) == false)
        return Error::Validation("project '" + _name + "' isn't in the dependency graph");

    Node& n = m_graph[_name];

    if (n.state == NodeState::Visited)
        return Error::Ok();

    for (auto& d : n.project->dependencies)
    {
        if (Error e = TravelVendor(d, _vendorRoot, _out))
            return e;
    }

    for (auto const& [libName, v] : n.project->vendor)
    {
        for (auto const& inc : v.include)
        {
            fs::path p = _vendorRoot / inc;
            if (FileHelper::IsDir(p) == false)
                return Error::Io("vendor include dir '" + p.string() + "' (project '" + _name + "', lib '" + libName + "') doesn't exist");

            AppendUnique(_out.includeDirs, p);
        }

        for (auto const& lib : v.libs)
            AppendUnique(_out.libs, lib);

        for (auto const& [config, dir] : v.libDirs)
        {
            fs::path p = _vendorRoot / dir;
            if (FileHelper::IsDir(p) == false)
                return Error::Io("vendor lib dir '" + p.string() + "' (project '" + _name + "', lib '" + libName + "', config '" + config + "') doesn't exist");

            auto it = _out.libDirs.find(config);
            if (it != _out.libDirs.end() && it->second != p)
                return Error::Validation("conflicting vendor lib dir for config '" + config + "': '" + it->second.string() + "' vs '" + p.string() + "'");

            _out.libDirs[config] = p;
        }

        for (auto const& dll : v.dlls)
        {
            fs::path p = _vendorRoot / dll;
            if (FileHelper::IsFile(p) == false)
                return Error::Io("vendor dll '" + p.string() + "' (project '" + _name + "', lib '" + libName + "') doesn't exist");

            AppendUnique(_out.dlls, p);
        }
    }

    n.state = NodeState::Visited;

    return Error::Ok();
}
