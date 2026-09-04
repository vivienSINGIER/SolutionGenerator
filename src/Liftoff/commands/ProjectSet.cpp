#include "ProjectSet.h"

#include "Command.h"
#include "core/Logger.h"

namespace
{
    void GetValues(std::string _in, std::vector<std::string>& _out)
    {
        if (_in.find(',') == std::string::npos)
        {
            _out.push_back(_in);
            return;
        }       
        
        std::stringstream ss(_in);
        std::string item;
        char delim = ',';
        while (std::getline(ss, item, delim))
        {
            _out.push_back(item);
        }
    }
    
    bool Prompt(std::string _name, bool _prompt)
    {
        std::vector<std::string> plan;
        
        plan.push_back("Modify the '" + _name + ".json' project manifest file");
        
        Cmd::PrintPlan(plan);
        
        if (_prompt == false)
            return true;
        return Cmd::ApplyPrompt(static_cast<int>(plan.size()));
    }
}

Error Cmd::ProjectSet(Invocation const& _inv)
{
    ArgBag const& bag = _inv.argBag;
    
    if (bag.Exists("name") == false)
        return Error::Validation("missing required 'name' argument");
    if (bag.Exists("key") == false)
        return Error::Validation("missing required 'key' argument");
    if (bag.Exists("value") == false)
        return Error::Validation("missing required 'value' argument");
    
    std::string name = bag.Get("name");
    std::string key = bag.Get("key");
    std::string value = bag.Get("value");
    bool isVerbose = _inv.globalFlags.verbose;
    
    fs::path manifestPath;
    if (_inv.globalFlags.hasConfigPath)
        manifestPath = _inv.globalFlags.configPath;
    else if (Error e = Cmd::GetDefaultManifestPath(manifestPath))
        return e;
    
    fs::path projectM = manifestPath.parent_path() / "projects" / (name + ".json");
    
    if (_inv.globalFlags.dryRun)
    {
        Prompt(name, false);
        return Error::Ok();
    }
    if (_inv.globalFlags.yes == false)
    {
        bool res = Prompt(name, true);
        if (res == false)
            return Error::Declined("user declined any changes");
    }
    
    std::string str;
    if (Error e = FileHelper::ReadFile(projectM, str)) 
        return e;
    
    js j;
    
    try { j = js::parse(str); }
    catch (js::exception&) { return Error::Validation("unable to parse solution manifest file"); }
    
    std::vector<std::string> values;
    GetValues(value, values);
    
    Verbose("Opened '" + projectM.string() + "'", isVerbose);
    
    if (values.empty() || values[0].empty())
        return Error::Validation("missing required 'value' argument");
    
    bool found = false;
    for (auto& [mkey, mvalue] : j.items())
    {
        if (mkey != key)
            continue;
        
        if (mvalue.is_object())
            return Error::Validation("unhandled key '" + key + "', please modify the '" + name + ".json' manifest directly");
        
        found = true;
        
        if (mvalue.is_string())
        {
            mvalue = values[0]; 
            break;
        }
        if (mvalue.is_number_integer())
        {
            mvalue = std::stoi(values[0]); 
            break;
        }
        if (mvalue.is_number_float())
        {
            mvalue = std::stof(values[0]);
            break;
        }
        if (mvalue.is_array())
            mvalue = values;
    }
    
    if (found == false)
        return Error::Validation("'" + key + "' is not a member of the manifest");
    
    Verbose("Set '" + key + "' to '" + value + "'", isVerbose);
    
    if (Error e = FileHelper::WriteFileAtomic(projectM, j.dump(4)))
        return e;
    
    Verbose("Saved '" + projectM.string() + "'", isVerbose);
    
    return Error::Ok();
}
