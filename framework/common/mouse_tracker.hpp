//
// Created by 秋鱼 on 2022/6/15.
//

#pragma once

#include "camera.hpp"

namespace yu {

enum class CameraUpdateOp
{
    Movement, Offset
};

class MouseTracker
{
public:
    MouseTracker();
    ~MouseTracker();

    void startTracking(int x, int y);
    void stopTracking();

    void update(int x, int y, CameraUpdateOp op = CameraUpdateOp::Movement);

    void zoom(float dir);

public:
    std::unique_ptr<Camera> camera_ = nullptr;
private:
    void updatePos(int x, int y);
    void updateCamera();
    void offsetCamera() const;

    bool start_tracking_ = false;
    int prev_posX_ = 0;
    int prev_posY_ = 0;

    float dx = 0.0f, dy = 0.0f;
};

} // namespace yu