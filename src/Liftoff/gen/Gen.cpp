#include "Gen.h"

#include "io/FileHelper.h"

std::string Gen::Braced(std::string const& _guid)
{
    return "{" + _guid + "}";
}

std::string Gen::Indent(std::string const& _block, int _levels)
{
    if (_block.empty() || _levels <= 0)
        return _block;

    std::string prefix;
    for (int i = 0; i < _levels; i++)
        prefix += "    ";

    std::string out;
    size_t pos = 0;
    while (pos < _block.size())
    {
        size_t nl = _block.find('\n', pos);
        size_t len = (nl == std::string::npos) ? std::string::npos : nl - pos;
        std::string line = _block.substr(pos, len);

        if (!line.empty())
            out += prefix + line;

        if (nl == std::string::npos)
            break;

        out += "\n";
        pos = nl + 1;
    }
    return out;
}

std::string Gen::Tag(std::string const& _tag, std::vector<std::string> const& _values,
                      std::vector<std::string> const& _tagArgs)
{
    std::string out = "<" + _tag;
    for (auto const& v : _tagArgs)
        out += " " + v;

    if (_values.empty())
    {
        out += "/>\n";
        return out;
    }

    out += ">";

    bool inlineScalar = _values.size() == 1 && _values[0].find('\n') == std::string::npos;
    if (inlineScalar)
        out += _values[0];
    else
    {
        out += "\n";
        for (auto const& v : _values)
            out += Indent(v);
    }

    out += "</" + _tag + ">\n";
    return out;
}

std::string Gen::AddRootRelative(fs::path const& _dir)
{
    return "$(SolutionDir)..\\" + FileHelper::ToBackSlashes(_dir);
}
