//
// Created by 秋鱼 on 2022/6/15.
//

#include "mouse_tracker.hpp"

namespace yu {

MouseTracker::MouseTracker() : camera_{std::make_unique<Camera>()}
{

}

MouseTracker::~MouseTracker()
{

}

void MouseTracker::startTracking(int x, int y)
{
    prev_posX_ = x;
    prev_posY_ = y;

    start_tracking_ = true;
}

void MouseTracker::stopTracking()
{
    start_tracking_ = false;
}

void MouseTracker::update(int x, int y, CameraUpdateOp op)
{
    updatePos(x, y);
    
    if (op == CameraUpdateOp::Movement) {
        updateCamera();
    }
    else if (op == CameraUpdateOp::Offset) {
        offsetCamera();
    }
}

void MouseTracker::updateCamera()
{
    float yaw = camera_->yaw;
    float pitch = camera_->pitch;
    float rotSpeed = camera_->rot_speed;

    yaw -= dx * rotSpeed;
    pitch += dy * rotSpeed;
    pitch = std::max(-glm::half_pi<float>() + EPS_F, std::min(pitch, glm::half_pi<float>() - EPS_F));

    camera_->updateOrbit(yaw, pitch);
}

void MouseTracker::zoom(float dir)
{
    camera_->zoom(dir);
}

void MouseTracker::updatePos(int x, int y)
{
    if (!start_tracking_) {
        startTracking(x, y);
        return;
    }

    dx = static_cast<float>(x - prev_posX_);
    dy = static_cast<float>(y - prev_posY_);
    prev_posX_ = x;
    prev_posY_ = y;
}

void MouseTracker::offsetCamera() const
{
    float speed = camera_->offset_speed;
    
    camera_->offset(dx * speed, dy * speed);
}

} // namespace yu