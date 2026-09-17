#include "Gen.h"

#include "io/FileHelper.h"

std::string Gen::Braced(std::string const& _guid)
{
    return "{" + _guid + "}";
}

std::string Gen::Tag(std::string const& _tag, std::vector<std::string> const& _values,
                      int _indentCount, std::vector<std::string> const& _tagArgs)
{
    std::string out;
    std::string indent;

    for (int i = 0; i < _indentCount; i++)
        indent += "    ";
    out += indent + "<" + _tag;

    for (auto const& v : _tagArgs)
        out += " " + v;
    out += ">";

    if (_values.size() > 1)
    {
        out += "\n";
        for (auto const& v : _values)
            out += indent + "    " + v + "\n";
    }
    else if (_values.size() == 1)
        out += _values[0];
    out += "</" + _tag + ">\n";
    return out;
}

std::string Gen::AddRootRelative(fs::path const& _dir)
{
    return "$(SolutionDir)..\\" + FileHelper::ToBackSlashes(_dir);
}
