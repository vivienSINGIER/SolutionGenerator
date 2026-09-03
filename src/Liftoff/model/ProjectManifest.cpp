#include "ProjectManifest.h"

#include "io/FileHelper.h"

using Manifest::ReadRequired;
using Manifest::ReadOptional;

static bool IsValidKind(std::string const& _kind)
{
    return _kind == "Application" || _kind == "ConsoleApp"
        || _kind == "StaticLib"   || _kind == "DynamicLib";
}

static std::string const KIND_HINT =
    "must be 'Application', 'ConsoleApp', 'StaticLib' or 'DynamicLib'";

ProjectManifest::ProjectManifest()
{
    m_schemaVersion = MANIFEST_SCHEMA_VERSION;

    m_name = "MyProject";
    m_kind = "Application";
}

Error ProjectManifest::Serialize(fs::path const& _configPath)
{
    js c;

    c["schemaVersion"] = m_schemaVersion;

    c["name"] = m_name;
    c["kind"] = m_kind;

    c["dependencies"] = m_dependencies;
    c["exports"] = m_exports;

    c["defines"] = m_defines;
    c["includeDirs"] = m_includeDirs;
    c["exclude"] = m_exclude;

    c["vendor"] = js::object();
    for (auto const& pair : m_vendor)
    {
        std::string const& name = pair.first;
        VendorDep const& v = pair.second;

        c["vendor"][name]["include"] = v.include;
        c["vendor"][name]["libs"] = v.libs;
        c["vendor"][name]["libDirs"] = v.libDirs;
        c["vendor"][name]["dlls"] = v.dlls;
    }

    c["configOverrides"] = js::object();
    for (auto const& pair : m_configOverrides)
    {
        std::string const& config = pair.first;
        ConfigOverride const& override = pair.second;

        if (override.overrides[0])
            c["configOverrides"][config]["warningLevel"] = override.warningLvl;
        if (override.overrides[1])
            c["configOverrides"][config]["warningsAsErrors"] = override.warningsAsErrors;
        if (override.overrides[2])
            c["configOverrides"][config]["conformanceMode"] = override.conformanceMode;
        if (override.overrides[3])
            c["configOverrides"][config]["defines"] = override.defines;
        if (override.overrides[4])
            c["configOverrides"][config]["includeDirs"] = override.includeDirs;
    }

    return FileHelper::WriteFileAtomic(_configPath, c.dump(4));
}

Error ProjectManifest::Deserialize(ArgBag const& _args)
{
    if (_args.Exists("name"))
        m_name = _args.Get("name");
    else
        return Error::Validation("missing required argument 'name'");

    if (_args.Exists("kind"))
    {
        m_kind = _args.Get("kind");
        if (!IsValidKind(m_kind))
            return Error::Validation("invalid project kind '" + m_kind + "'. " + KIND_HINT);
    }

    return Error::Ok();
}

Error ProjectManifest::Deserialize(fs::path const& _configPath)
{
    std::string str;
    Error e = FileHelper::ReadFile(_configPath, str);
    if (e) return e;

    js j;

    try { j = js::parse(str); }
    catch (js::exception&) { return Error::Validation("unable to parse project manifest file"); }

    Error err = Error::Validation("Invalid project manifest");

    int version = 0;
    if (ReadRequired(j, "schemaVersion", version, err) && version != MANIFEST_SCHEMA_VERSION)
        err.details.push_back("Outdated schemaVersion, please update your project");
    m_schemaVersion = version;

    ReadRequired(j, "name",         m_name,         err);
    ReadRequired(j, "dependencies", m_dependencies, err);
    ReadRequired(j, "defines",      m_defines,      err);
    ReadRequired(j, "includeDirs",  m_includeDirs,  err);

    if (ReadRequired(j, "kind", m_kind, err) && !IsValidKind(m_kind))
        err.details.push_back("invalid project kind '" + m_kind + "'. " + KIND_HINT);

    ReadOptional(j, "exports", m_exports);
    ReadOptional(j, "exclude", m_exclude);

    if (j.contains("vendor"))
    {
        for (auto& [name, settings] : j["vendor"].items())
        {
            VendorDep v;
            ReadOptional(settings, "include", v.include);
            ReadOptional(settings, "libs",    v.libs);
            ReadOptional(settings, "libDirs", v.libDirs);
            ReadOptional(settings, "dlls",    v.dlls);

            m_vendor[name] = v;
        }
    }

    if (j.contains("configOverrides"))
    {
        js const& overrides = j["configOverrides"];

        for (auto& [configName, settings] : overrides.items())
        {
            if (configName != "Release" && configName != "Debug") {
                err.details.push_back("incorrect config name '" + configName + "'. must be 'Release' or 'Debug'");
            }

            ConfigOverride o;
            if (settings.contains("warningLevel"))
            {
                o.warningLvl = settings["warningLevel"];
                o.overrides[0] = true;
            }
            if (settings.contains("warningsAsErrors"))
            {
                o.warningsAsErrors = settings["warningsAsErrors"];
                o.overrides[1] = true;
            }
            if (settings.contains("conformanceMode"))
            {
                o.conformanceMode = settings["conformanceMode"];
                o.overrides[2] = true;
            }
            if (settings.contains("defines"))
            {
                o.defines = settings["defines"];
                o.overrides[3] = true;
            }
            if (settings.contains("includeDirs"))
            {
                o.includeDirs = settings["includeDirs"];
                o.overrides[4] = true;
            }

            m_configOverrides[configName] = o;
        }
    }
    else
        err.details.push_back("Missing required argument 'configOverrides'");

    if (!err.details.empty())
        return err;

    return Error::Ok();
}
