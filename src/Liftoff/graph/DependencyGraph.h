#ifndef DEPENDENCYGRAPH_H_DEFINED
#define DEPENDENCYGRAPH_H_DEFINED

#include "model/ProjectManifest.h"

class DependencyGraph 
{
public:
    DependencyGraph(std::vector<ProjectManifest> const& _projects);

    Error ValidateGraph();
private:
    enum class NodeState { Unvisited, InProgress, Visited };
    
    std::unordered_map<std::string, ProjectManifest const*> m_projects;
    std::unordered_map<std::string, NodeState> m_nodeState;
    
    std::vector<std::string> m_path;
    
    Error HasHoles();
    Error HasLoops();
    
    Error Visit(std::string _name);
    
    bool Find(std::string_view _pName);
};

#endif
