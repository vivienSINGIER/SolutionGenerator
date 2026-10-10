#include "ProjectManifest.h"

#include "io/FileHelper.h"

using Manifest::ReadRequired;
using Manifest::ReadOptional;

static bool IsValidKind(std::string const& _kind)
{
    return _kind == "console" || _kind == "window"
        || _kind == "staticlib"   || _kind == "dynamiclib";
}

static std::string const KIND_HINT =
    "must be 'console', 'window', 'staticlib' or 'dynamiclib'";

ProjectManifest::ProjectManifest()
{
    schemaVersion = MANIFEST_SCHEMA_VERSION;

    name = "MyProject";
    kind = "window";
}

Error ProjectManifest::Serialize(fs::path const& _configPath)
{
    js c;

    c["schemaVersion"] = schemaVersion;

    c["name"] = name;
    c["kind"] = kind;

    c["dependencies"] = dependencies;
    c["exports"] = exports;

    c["defines"] = defines;
    c["exclude"] = exclude;

    c["vendor"] = js::object();
    for (auto const& pair : vendor)
    {
        std::string const& name = pair.first;
        VendorDep const& v = pair.second;

        c["vendor"][name]["include"] = v.include;
        c["vendor"][name]["libs"] = v.libs;
        c["vendor"][name]["libDirs"] = v.libDirs;
        c["vendor"][name]["dlls"] = v.dlls;

        for (auto const& configPair : v.configs)
        {
            std::string const& config = configPair.first;
            VendorConfig const& vc = configPair.second;

            if (!vc.libs.empty())
                c["vendor"][name]["configs"][config]["libs"] = vc.libs;
            if (!vc.libDirs.empty())
                c["vendor"][name]["configs"][config]["libDirs"] = vc.libDirs;
            if (!vc.dlls.empty())
                c["vendor"][name]["configs"][config]["dlls"] = vc.dlls;
        }
    }

    c["configOverrides"] = js::object();
    for (auto const& pair : configOverrides)
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
    }

    return FileHelper::WriteFileAtomic(_configPath, c.dump(4));
}

Error ProjectManifest::Deserialize(ArgBag const& _args)
{
    if (_args.Exists("name"))
        name = _args.Get("name");
    else
        return Error::Validation("missing required argument 'name'");

    if (_args.Exists("kind"))
    {
        kind = _args.Get("kind");
        if (!IsValidKind(kind))
            return Error::Validation("invalid project kind '" + kind + "'. " + KIND_HINT);
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
    schemaVersion = version;

    ReadRequired(j, "name",         name,         err);
    ReadRequired(j, "dependencies", dependencies, err);
    ReadRequired(j, "defines",      defines,      err);

    if (ReadRequired(j, "kind", kind, err) && !IsValidKind(kind))
        err.details.push_back("invalid project kind '" + kind + "'. " + KIND_HINT);

    ReadOptional(j, "exports", exports);
    ReadOptional(j, "exclude", exclude);

    if (j.contains("vendor"))
    {
        for (auto& [name, settings] : j["vendor"].items())
        {
            VendorDep v;

            try
            {
                ReadOptional(settings, "include", v.include);
                ReadOptional(settings, "libs",    v.libs);
                ReadOptional(settings, "libDirs", v.libDirs);
                ReadOptional(settings, "dlls",    v.dlls);

                if (settings.contains("configs"))
                {
                    for (auto& [configName, configSettings] : settings["configs"].items())
                    {
                        if (configName != "Release" && configName != "Debug") {
                            err.details.push_back("incorrect config name '" + configName + "' in vendor '" + name + "'. must be 'Release' or 'Debug'");
                        }

                        VendorConfig vc;
                        ReadOptional(configSettings, "libs",    vc.libs);
                        ReadOptional(configSettings, "libDirs", vc.libDirs);
                        ReadOptional(configSettings, "dlls",    vc.dlls);

                        v.configs[configName] = vc;
                    }
                }
            }
            catch (js::exception const&)
            {
                err.details.push_back("vendor '" + name + "' has a field with the wrong type");
            }

            vendor[name] = v;
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

            configOverrides[configName] = o;
        }
    }
    else
        err.details.push_back("Missing required argument 'configOverrides'");

    if (!err.details.empty())
        return err;

    return Error::Ok();
}
