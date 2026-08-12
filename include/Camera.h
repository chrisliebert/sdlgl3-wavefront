#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "Common.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera
{
public:
    Camera();
    
    // Non-copyable, movable
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    Camera(Camera&&) = default;
    Camera& operator=(Camera&&) = default;

    glm::mat4 modelViewMatrix{};
    glm::mat4 projectionMatrix{};
    glm::vec3 position{0.0f, 1.0f, 0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};
    glm::vec3 right{1.0f, 0.0f, 0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    double horizontalAngle = 3.14159265358979323846; // PI
    double verticalAngle = 0.0;

    void aim(double x, double y);
    void moveForward(double amount);
    void moveBackward(double amount);
    void moveLeft(double amount);
    void moveRight(double amount);
    void update();
};

#endif // _CAMERA_H_
