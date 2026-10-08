#include "Gen.h"
#include "Guid.h"

#include "io/FileHelper.h"

#include <set>

namespace
{
    using Gen::AddRootRelative;
    using Gen::Braced;
    using Gen::Tag;

    // Files are root-relative (src/<project>/a/b.cpp); the filter is the
    // directory below src/<project>/ (or res/<project>/), so drop the first two parts.
    fs::path FilterDir(fs::path const& _file)
    {
        fs::path out;
        int index = 0;
        for (auto const& part : _file.parent_path())
        {
            if (index++ < 2)
                continue;
            out /= part;
        }
        return out;
    }

    std::string FilterOf(fs::path const& _file)
    {
        return FileHelper::ToBackSlashes(FilterDir(_file));
    }

    void CollectFilterDirs(std::vector<fs::path> const& _files, std::set<std::string>& _dirs)
    {
        for (auto const& f : _files)
        {
            fs::path accum;
            for (auto const& part : FilterDir(f))
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
            values.push_back(Tag("Filter", {uid}, {"Include=\"" + dir + "\""}));
        }

        return Tag("ItemGroup", values);
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

            values.push_back(Tag(_tag, children, {"Include=\"" + include + "\""}));
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

    std::string body;
    body += FilterDeclarations(_solution, _project, dirs);
    body += Tag("ItemGroup", FilterItemValues("ClCompile", _files.clCompileFiles));
    body += Tag("ItemGroup", FilterItemValues("ClInclude", _files.clIncludeFiles));
    body += Tag("ItemGroup", otherItems);

    std::string content = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    content += "<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n";
    content += Gen::Indent(body);
    content += "</Project>\n";

    return FileHelper::WriteFileAtomic(_path, content);
}
