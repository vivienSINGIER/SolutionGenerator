#ifndef CORE_VERSION_H
#define CORE_VERSION_H

#include <string_view>

namespace Version
{
    inline constexpr int major = 0;
    inline constexpr int minor = 1;
    inline constexpr int patch = 0;

    inline constexpr std::string_view string = "0.1.0";
}

#endif
