#include "Gen.h"
#include "Guid.h"

#include "io/FileHelper.h"

#include <set>

namespace
{
    using Gen::AddRootRelative;
    using Gen::Braced;
    using Gen::Tag;

    std::string FilterOf(fs::path const& _file)
    {
        return FileHelper::ToBackSlashes(_file.parent_path());
    }

    void CollectFilterDirs(std::vector<fs::path> const& _files, std::set<std::string>& _dirs)
    {
        for (auto const& f : _files)
        {
            fs::path accum;
            for (auto const& part : f.parent_path())
            {
                accum /= part;
                _dirs.insert(FileHelper::ToBackSlashes(accum));
            }
        }
    }

    std::string FilterDeclarations(SolutionManifest const& _s, ProjectManifest const& _p, std::set<std::string> const& _dirs)
    {
        std::vector<std::string> values;

        for (auto const& dir : _dirs)
        {
            std::string guid = Braced(GUID::GenerateGuid(_s.name, _p.name + "|filter|" + dir));
            std::string uid = Tag("UniqueIdentifier", {guid});
            values.push_back(Tag("Filter", {uid}, 0, {"Include=\"" + dir + "\""}));
        }

        return Tag("ItemGroup", values, 1, {});
    }

    std::vector<std::string> FilterItemValues(std::string const& _tag, std::vector<fs::path> const& _files)
    {
        std::vector<std::string> values;

        for (auto const& f : _files)
        {
            std::string include = AddRootRelative(f);
            std::string filter = FilterOf(f);

            std::vector<std::string> children;
            if (!filter.empty())
                children.push_back(Tag("Filter", {filter}));

            values.push_back(Tag(_tag, children, 0, {"Include=\"" + include + "\""}));
        }

        return values;
    }
}

Error Gen::GenFilters(SolutionManifest const& _solution, ProjectManifest const& _project,
                       ProjectFiles const& _files, fs::path const& _path)
{
    std::set<std::string> dirs;
    CollectFilterDirs(_files.clCompileFiles, dirs);
    CollectFilterDirs(_files.clIncludeFiles, dirs);
    CollectFilterDirs(_files.resourcesCompileFiles, dirs);
    CollectFilterDirs(_files.noneFiles, dirs);
    CollectFilterDirs(_files.resFiles, dirs);

    std::vector<std::string> otherItems = FilterItemValues("ResourceCompile", _files.resourcesCompileFiles);
    std::vector<std::string> noneItems = FilterItemValues("None", _files.noneFiles);
    std::vector<std::string> resItems = FilterItemValues("None", _files.resFiles);
    otherItems.insert(otherItems.end(), noneItems.begin(), noneItems.end());
    otherItems.insert(otherItems.end(), resItems.begin(), resItems.end());

    std::string content;
    content += "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    content += "<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n";
    content += FilterDeclarations(_solution, _project, dirs);
    content += Tag("ItemGroup", FilterItemValues("ClCompile", _files.clCompileFiles), 1, {});
    content += Tag("ItemGroup", FilterItemValues("ClInclude", _files.clIncludeFiles), 1, {});
    content += Tag("ItemGroup", otherItems, 1, {});
    content += "</Project>\n";

    return FileHelper::WriteFileAtomic(_path, content);
}
