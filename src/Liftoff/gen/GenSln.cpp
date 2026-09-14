#include "Gen.h"

#include "Guid.h"
#include "io/FileHelper.h"

namespace
{
    constexpr char const* CPP_PROJECT_TYPE_GUID    = "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}";
    constexpr char const* FOLDER_PROJECT_TYPE_GUID = "{2150E333-8FDC-42A3-9474-1A3956D46DE8}";

    std::string Braced(std::string const& _guid)
    {
        return "{" + _guid + "}";
    }
    
    std::vector<std::string> CollectGroups(SolutionManifest const& _solution)
    {
        std::vector<std::string> groups;

        for (ProjectDesc const& p : _solution.projects)
        {
            if (p.group.empty())
                continue;

            bool found = false;
            for (std::string const& g : groups)
            {
                if (g == p.group)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
                groups.push_back(p.group);
        }

        return groups;
    }
    
    std::string ProjectRelativePath(ProjectDesc const& _project)
    {
        return _project.name + "\\" + _project.name + ".vcxproj";
    }

    std::string BuildHeader()
    {
        std::string header;
        header += "Microsoft Visual Studio Solution File, Format Version 12.00\n";
        header += "# Visual Studio Version 17\n";
        header += "VisualStudioVersion = 17.0.31903.59\n";
        header += "MinimumVisualStudioVersion = 10.0.40219.1\n";
        return header;
    }
    
    std::string BuildProjectSections(SolutionManifest const& _solution,
                                      std::map<std::string, std::string>& _projectGuids,
                                      std::map<std::string, std::string>& _folderGuids)
    {
        std::string out;

        for (std::string const& group : CollectGroups(_solution))
        {
            std::string guid = GUID::GenerateGuid(_solution.name, "folder:" + group);
            _folderGuids[group] = guid;

            out += "Project(\"" + std::string(FOLDER_PROJECT_TYPE_GUID) + "\") = \"" + group
                +  "\", \"" + group + "\", \"" + Braced(guid) + "\"\n";
            out += "EndProject\n";
        }

        for (ProjectDesc const& p : _solution.projects)
        {
            std::string guid = GUID::GenerateGuid(_solution.name, p.name);
            _projectGuids[p.name] = guid;

            out += "Project(\"" + std::string(CPP_PROJECT_TYPE_GUID) + "\") = \"" + p.name
                +  "\", \"" + ProjectRelativePath(p) + "\", \"" + Braced(guid) + "\"\n";
            out += "EndProject\n";
        }

        return out;
    }

    std::string BuildGlobalSection(SolutionManifest const& _solution,
                                    std::map<std::string, std::string> const& _projectGuids,
                                    std::map<std::string, std::string> const& _folderGuids)
    {
        std::string out = "Global\n";

        out += "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n";
        for (std::string const& config : _solution.configs)
        {
            for (std::string const& platform : _solution.platforms)
            {
                std::string cfgPlat = config + "|" + platform;
                out += "\t\t" + cfgPlat + " = " + cfgPlat + "\n";
            }
        }
        out += "\tEndGlobalSection\n";

        out += "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n";
        for (ProjectDesc const& p : _solution.projects)
        {
            std::string const& guid = Braced(_projectGuids.at(p.name));
            for (std::string const& config : _solution.configs)
            {
                for (std::string const& platform : _solution.platforms)
                {
                    std::string cfgPlat = config + "|" + platform;
                    out += "\t\t" + guid + "." + cfgPlat + ".ActiveCfg = " + cfgPlat + "\n";
                    out += "\t\t" + guid + "." + cfgPlat + ".Build.0 = "  + cfgPlat + "\n";
                }
            }
        }
        out += "\tEndGlobalSection\n";

        out += "\tGlobalSection(SolutionProperties) = preSolution\n";
        out += "\t\tHideSolutionNode = FALSE\n";
        out += "\tEndGlobalSection\n";

        if (!_folderGuids.empty())
        {
            out += "\tGlobalSection(NestedProjects) = preSolution\n";
            for (ProjectDesc const& p : _solution.projects)
            {
                if (p.group.empty())
                    continue;
                out += "\t\t" + Braced(_projectGuids.at(p.name)) + " = " + Braced(_folderGuids.at(p.group)) + "\n";
            }
            out += "\tEndGlobalSection\n";
        }

        out += "\tGlobalSection(ExtensibilityGlobals) = postSolution\n";
        out += "\t\tSolutionGuid = " + Braced(GUID::GenerateGuid(_solution.name, "__solution__")) + "\n";
        out += "\tEndGlobalSection\n";

        out += "EndGlobal\n";
        return out;
    }
}

Error Gen::GenSln(SolutionManifest const& _solution, fs::path const& _path)
{
    if (_solution.projects.empty())
        return Error::Validation("solution '" + _solution.name + "' has no projects to generate");

    std::map<std::string, std::string> projectGuids;
    std::map<std::string, std::string> folderGuids;

    std::string content = BuildHeader();
    content += BuildProjectSections(_solution, projectGuids, folderGuids);
    content += BuildGlobalSection(_solution, projectGuids, folderGuids);

    return FileHelper::WriteFileAtomic(_path, content);
}
