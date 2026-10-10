#include "DependencyGraph.h"

#include <algorithm>

#include "commands/Command.h"
#include "core/Logger.h"

namespace
{
    void WarnVendor(std::string const& _msg)
    {
        Log::Log(LogType::Warning, "warning: " + _msg + '\n');
    }

    template <typename T>
    void AppendUnique(std::vector<T>& _vec, T const& _value)
    {
        if (std::find(_vec.begin(), _vec.end(), _value) == _vec.end())
            _vec.push_back(_value);
    }

    // A path holding an MSBuild macro ($(Platform), $(Configuration)) only resolves at build time.
    bool HasMacro(std::string const& _path)
    {
        return _path.find("$(") != std::string::npos;
    }

    void ResolvePaths(std::vector<std::string> const& _paths, fs::path const& _libRoot, bool _isDir,
                      std::string const& _what, std::string const& _where, std::vector<fs::path>& _out)
    {
        for (auto const& path : _paths)
        {
            fs::path p = _libRoot / path;
            bool exists = _isDir ? FileHelper::IsDir(p) : FileHelper::IsFile(p);
            if (HasMacro(path) == false && exists == false)
            {
                WarnVendor("vendor " + _what + " '" + p.string() + "' " + _where + " doesn't exist");
                continue;
            }

            AppendUnique(_out, p);
        }
    }

    // Warns for each lib that none of the lib dirs holds. Skipped when a dir can't be checked.
    void WarnMissingLibs(std::vector<std::string> const& _libs, std::vector<std::string> const& _dirs,
                         fs::path const& _libRoot, std::string const& _where)
    {
        if (_dirs.empty())
            return;

        for (auto const& dir : _dirs)
            if (HasMacro(dir))
                return;

        for (auto const& lib : _libs)
        {
            bool found = false;
            for (auto const& dir : _dirs)
                if (FileHelper::IsFile(_libRoot / dir / lib))
                    found = true;

            if (found == false)
                WarnVendor("vendor lib '" + lib + "' " + _where + " not found in its lib dirs");
        }
    }
}

Error DependencyGraph::BuildGraph(std::vector<ProjectManifest> const& _projects)
{
    m_graph.clear();
    m_graph.reserve(_projects.size());
    for (auto const& p : _projects)
    {
        Node n;
        n.project = &p;
        m_graph[p.name] = n;
    }

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
        pair.second.state = NodeState::Unvisited;

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
        std::string where = "(project '" + _name + "', lib '" + libName + "')";
        fs::path libRoot = _vendorRoot / libName;

        ResolvePaths(v.include, libRoot, true,  "include dir", where, _out.includeDirs);
        ResolvePaths(v.libDirs, libRoot, true,  "lib dir",     where, _out.libDirs);
        ResolvePaths(v.dlls,    libRoot, false, "dll",         where, _out.dlls);

        for (auto const& lib : v.libs)
            AppendUnique(_out.libs, lib);

        if (v.configs.empty())
            WarnMissingLibs(v.libs, v.libDirs, libRoot, where);

        for (auto const& [config, vc] : v.configs)
        {
            std::string whereConfig = "(project '" + _name + "', lib '" + libName + "', config '" + config + "')";
            ResolvedVendorConfig& rc = _out.configs[config];

            ResolvePaths(vc.libDirs, libRoot, true,  "lib dir", whereConfig, rc.libDirs);
            ResolvePaths(vc.dlls,    libRoot, false, "dll",     whereConfig, rc.dlls);

            for (auto const& lib : vc.libs)
                AppendUnique(rc.libs, lib);

            std::vector<std::string> libs = v.libs;
            libs.insert(libs.end(), vc.libs.begin(), vc.libs.end());
            std::vector<std::string> dirs = v.libDirs;
            dirs.insert(dirs.end(), vc.libDirs.begin(), vc.libDirs.end());

            WarnMissingLibs(libs, dirs, libRoot, whereConfig);
        }
    }

    n.state = NodeState::Visited;

    return Error::Ok();
}
