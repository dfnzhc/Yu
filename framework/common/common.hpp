//
// Created by 秋鱼 on 2022/5/8.
//

#pragma once

namespace filesystem {
class path;
class resolver;
}

#include <spdlog/fmt/fmt.h>

namespace ST {

#define EPS_F = 1e-5;
#define INF_F = std::numeric_limits<float>::infinity();
#define EPS_D = 1e-16;
#define INF_D = std::numeric_limits<double>::infinity();

/// for debugging purposes
using std::cout;
using std::cerr;
using std::endl;

/// Simple exception class
class STException : public std::runtime_error
{
public:
    template<typename... Args>
    explicit STException(std::string_view fmt_str, const Args& ... args)
        : std::runtime_error(fmt::format(fmt_str, args...)) {}
};

} // namespace ST