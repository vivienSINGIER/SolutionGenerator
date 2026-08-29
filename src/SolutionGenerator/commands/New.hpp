#ifndef COMMANDS_NEW_HPP
#define COMMANDS_NEW_HPP

#include <filesystem>
#include <string>

#include "core/Error.h"

namespace fs = std::filesystem;

Error New(fs::path const& _path, std::string _solutionName, std::string _projectName = "");

#endif
