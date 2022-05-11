//
// Created by 秋鱼 on 2022/5/11.
//

#include "vk_appbase.hpp"

namespace ST {

bool vk_AppBase::prepare(Platform& platform)
{
    if (!Application::prepare(platform)) {
        return false;
    }
    
    
    return true;
}

void vk_AppBase::update(float delta_time)
{
    Application::update(delta_time);
}

void vk_AppBase::finish()
{
}

bool vk_AppBase::resize(const uint32_t width, const uint32_t height)
{
    
    
    return true;
}

void vk_AppBase::input_event(const InputEvent& input_event)
{
}

} // namespace ST