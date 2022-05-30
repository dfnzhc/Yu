//
// Created by 秋鱼 on 2022/5/11.
//

#pragma once

#ifndef DEL_COPY_IN_CLASS
#define DEL_COPY_IN_CLASS(CLASS_NAME)                         \
        CLASS_NAME(const CLASS_NAME&) = delete;               \
        CLASS_NAME(CLASS_NAME&&) = delete;                    \
        CLASS_NAME& operator=(const CLASS_NAME&) = delete;    \
        CLASS_NAME& operator=(CLASS_NAME && )= delete;
#endif

namespace ST {

constexpr inline std::string AssetPath()
{
    return std::string{YU_ROOT_PATH} + "/data";
}

} // namespace ST