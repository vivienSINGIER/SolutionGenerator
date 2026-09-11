#include "SourceDiscovery.h"

#include "io/FileHelper.h"

namespace
{
    bool MatchSegment(std::string _pattern, std::string _segment)
    {
        size_t p = 0;
        size_t s = 0;
        size_t star = std::string::npos;
        size_t mark = 0;
        while (s < _segment.size())
        {
            if (p < _pattern.size() && (_pattern[p] == '?' || _pattern[p] == _segment[s]))
            {
                p++;
                s++;
            }
            else if (p < _pattern.size() && _pattern[p] == '*')
            {
                star = p++;
                mark = s;
            }
            else if (star != std::string::npos)
            {
                p = star + 1;
                s = ++mark;
            }
            else
                return false;
        }
        while (p < _pattern.size() && _pattern[p] == '*')
            p++;
        return p == _pattern.size();
    }
    
    bool GlobMatchTravel(std::vector<std::string> _pattern, size_t _pi, std::vector<std::string> _segment, size_t _si)
    {
        while (_pi < _pattern.size())
        {
            if (_pattern[_pi] == "**")
            {
                for (size_t i = _si; i <= _segment.size(); i++)
                    if (GlobMatchTravel(_pattern, _pi + 1, _segment, i))
                        return true;
                return false;
            }
            
            if (_si >= _segment.size())
                return false;
            
            if (MatchSegment(_pattern[_pi], _segment[_si]) == false)
                return false;
            
            _pi++;
            _si++;
        }
        
        return _si == _segment.size();
    }
    
    bool GlobMatch(fs::path const& _pattern, fs::path const& _str)
    {
        std::vector<std::string> p;
        std::vector<std::string> s;
        std::stringstream sp(FileHelper::ToForwardSlashes(_pattern));
        std::stringstream ss(FileHelper::ToForwardSlashes(_str));
        std::string item;
        char delim = '/';
        while (std::getline(sp, item, delim))
        {
            for (size_t i = 0; i < item.size(); i++)
                item[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(item[i])));
            p.push_back(item);
        }
        while (std::getline(ss, item, delim))
        {
            for (size_t i = 0; i < item.size(); i++)
                item[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(item[i])));
            s.push_back(item);
        }
        
        return GlobMatchTravel(p, 0, s, 0);
    }
    
    bool IsExclude(std::vector<std::string> _excludes, fs::path const& _path)
    {
        for (auto const& ex : _excludes)
        {
            if (GlobMatch(ex, _path))
                return true;
        }
        return false;
    }
    
    void LoopExcludes(std::vector<std::string> _excludes, std::vector<fs::path>& _out)
    {
        std::vector<fs::path> temp = _out;
        _out.clear();
        
        for (size_t i = 0; i < temp.size(); i++)
        {
            if (IsExclude(_excludes, temp[i]))
                continue;
            _out.push_back(temp[i]);
        }
    }
}

Error Discovery::RunDiscovery(fs::path const& _src, fs::path const& _res, ProjectManifest const& _manifest, ProjectFiles& _out)
{
    if (Error e = GetSourceFiles(_src, _manifest, _out))
        return e;
    if (Error e = ApplyExcludes(_manifest.exclude, _out))
        return e;
    if (Error e = GetResFiles(_res, _out))
        return e;
    
    return Error::Ok();
}

Error Discovery::GetSourceFiles(fs::path const& _src, ProjectManifest const& _manifest, ProjectFiles& _out)
{
    if (FileHelper::IsDir(_src) == false)
        return Error::Io("couldn't find source directory '" + _src.string() + "'");
    
    std::vector<fs::path> dirs;
    if (Error e = FileHelper::ListDir(_src, dirs))
        return e;
    
    for (auto& d : dirs)
    {
        if (FileHelper::IsDir(d))
            continue;
        
        fs::path p = fs::relative(d, _src);
        ClassifyFile(_out, p);
    }
    
    return Error::Ok();
}

void Discovery::ClassifyFile(ProjectFiles& _out, fs::path const& _file)
{
    std::string dExt = FileHelper::ExtLower(_file);

    bool isClCompile = dExt == ".c" || dExt == ".cpp" || dExt == ".cc" || dExt == ".cxx" || dExt == ".c++";
    if (isClCompile)
    {
        _out.clCompileFiles.push_back(_file); 
        return;
    }
    
    bool isClInclude = dExt == ".h" || dExt == ".hpp" || dExt == ".hh" || dExt == ".hxx" || dExt == ".ipp" || dExt == ".tpp" || dExt == ".inl";
    if (isClInclude)
    {
        _out.clIncludeFiles.push_back(_file); 
        return;
    }
    
    bool isResCompile = dExt == ".rc";
    if (isResCompile)
    {
        _out.resourcesCompileFiles.push_back(_file);
        return;
    }
    
    _out.noneFiles.push_back(_file);
}

Error Discovery::ApplyExcludes(std::vector<std::string> _excludes, ProjectFiles& _out)
{
    LoopExcludes(_excludes, _out.clCompileFiles);
    LoopExcludes(_excludes, _out.clIncludeFiles);
    LoopExcludes(_excludes, _out.resourcesCompileFiles);
    LoopExcludes(_excludes, _out.noneFiles);
    
    return Error::Ok();
}

Error Discovery::GetResFiles(fs::path const& _res, ProjectFiles& _out)
{
    if (FileHelper::IsDir(_res) == false)
        return Error::Io("couldn't find res directory '" + _res.string() + "'");
    
    std::vector<fs::path> dirs;
    if (Error e = FileHelper::ListDir(_res, dirs))
        return e;
    
    for (auto& d : dirs)
    {
        if (FileHelper::IsDir(d))
            continue;
        
        fs::path p = fs::relative(d, _res);
        _out.resFiles.push_back(p);
    }
    
    return Error::Ok();
}
