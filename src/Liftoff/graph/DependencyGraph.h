#ifndef DEPENDENCYGRAPH_H_DEFINED
#define DEPENDENCYGRAPH_H_DEFINED

#include "model/ProjectManifest.h"

struct ResolvedVendorConfig
{
    std::vector<std::string>       libs;
    std::vector<fs::path>          libDirs;
    std::vector<fs::path>          dlls;
};

// Flattened, path-resolved vendor data for a project plus its transitive dependencies.
// libs / libDirs / dlls apply to every config; configs adds to them for one config.
struct ResolvedVendor
{
    std::vector<fs::path>          includeDirs;
    std::vector<std::string>       libs;
    std::vector<fs::path>          libDirs;
    std::vector<fs::path>          dlls;
    std::map<std::string, ResolvedVendorConfig> configs;   // config name -> additions
};

class DependencyGraph
{
public:
    DependencyGraph() = default;

    Error BuildGraph(std::vector<ProjectManifest> const& _projects);
    Error ValidateGraph();
    Error ResolveExports(std::string _pName, std::vector<fs::path>& _exports);
    Error ResolveVendor(std::string _pName, ResolvedVendor& _out);
private:
    enum class NodeState { Unvisited, InProgress, Visited };
    
    struct Node
    {
        ProjectManifest const* project = nullptr;
        NodeState state = NodeState::Unvisited;
        std::vector<Node*> dependencies;
    };
    
    std::unordered_map<std::string, Node> m_graph;
    std::vector<std::string> m_path;
    
    Error HasLoops();
    
    Error Visit(std::string _name);
    
    Error TravelExports(std::string const& _name, fs::path const& _root, std::vector<fs::path>& _exports);
    Error TravelVendor(std::string const& _name, fs::path const& _vendorRoot, ResolvedVendor& _out);
};

#endif
