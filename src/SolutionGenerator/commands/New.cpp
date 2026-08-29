#include "commands/New.hpp"

#include "io/FileHelper.h"

Error New(fs::path const& _path, std::string _solutionName, std::string _projectName)
{
    if (_projectName.empty())
        _projectName = _solutionName;

    fs::path const root = _path / _solutionName;

    fs::path const dirs[] = {
        root / "src" / _projectName,
        root / "res" / _projectName,
        root / "bin",
        root / "config",
    };

    for (fs::path const& dir : dirs)
    {
        if (Error e = FileHelper::CreateDir(dir))
            return e;
    }

    FileHelper::WriteOutcome outcome = FileHelper::WriteOutcome::Unchanged;
    return FileHelper::TouchFile(root / "config" / "config.json", outcome);
}
