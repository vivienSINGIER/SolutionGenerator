#include "Command.h"
#include "core/Logger.h"

namespace
{
    // "<indent><label>: a, b" - nothing when _values is empty.
    std::string Line(std::string const& _indent, std::string const& _label, std::vector<std::string> const& _values)
    {
        if (_values.empty())
            return "";

        std::string out = _indent + _label + ": ";
        for (size_t i = 0; i < _values.size(); i++)
            out += (i == 0 ? "" : ", ") + _values[i];
        return out + "\n";
    }
}


Error Cmd::VendorList(Workspace& _ws, Invocation const& _inv)
{
    ArgBag const& bag = _inv.argBag;

    if (!bag.Exists("project"))
        return Error::Validation("missing required 'project' argument");

    std::string project = bag.Get("project");

    fs::path manifestPath;
    if (_inv.globalFlags.hasConfigPath)
        manifestPath = _inv.globalFlags.configPath;
    else if (Error e = Cmd::GetDefaultManifestPath(manifestPath))
        return e;

    if (Error e = _ws.Load(manifestPath))
        return e;

    if (!_ws.HasProject(project))
        return Error::Validation("project '" + project + "' does not exist");

    ProjectManifest manifest = *_ws.GetProjectManifest(project);

    std::string out = "'" + project + "' Vendor list\n";
    for (auto const& [lib, v] : manifest.vendor)
    {
        out += "  - " + lib + "\n";
        out += Line("      ", "include", v.include);
        out += Line("      ", "lib-dir", v.libDirs);
        out += Line("      ", "libs", v.libs);
        out += Line("      ", "dlls", v.dlls);

        for (auto const& [config, vc] : v.configs)
        {
            out += "      [" + config + "]\n";
            out += Line("        ", "lib-dir", vc.libDirs);
            out += Line("        ", "libs", vc.libs);
            out += Line("        ", "dlls", vc.dlls);
        }
    }

    Log::Log(LogType::Help, out);

    return Error::Ok();
}
