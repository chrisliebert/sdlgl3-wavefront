#include "Camera.h"

Camera::Camera()
{
    // get viewport
    GLint mViewport[4];
    glGetIntegerv( GL_VIEWPORT, mViewport );
    projectionMatrix = glm::perspective(45.0f, (float)mViewport[2] /(float) mViewport[3], 0.1f, 10000.0f);
    horizontalAngle = M_PI; //3.1415926539
    verticalAngle = 0.0;
    position = glm::vec3(0.0, 1.0, 0.0);
    aim(0.0, 0.0);
    update();
}

void Camera::aim(double x, double y)
{
    horizontalAngle += x;
    verticalAngle += y;

    direction = glm::vec3(
                    cos(verticalAngle) * sin(horizontalAngle),
                    sin(verticalAngle),
                    cos(verticalAngle) * cos(horizontalAngle)
                );
    right = glm::vec3(
                sin(horizontalAngle - M_PI/2.0),
                0.0,
                cos(horizontalAngle - M_PI/2.0)
            );

    up = glm::cross(right, direction);
}

void Camera::moveForward(double amount)
{
    glm::vec3 scaledDirection = glm::vec3(
                                    direction.x * amount,
                                    direction.y * amount,
                                    direction.z * amount
                                );
    position += scaledDirection;
}

void Camera::moveBackward(double amount)
{
    moveForward(amount * -1.0);
}

void Camera::moveLeft(double amount)
{
    moveRight(amount * -1.0);
}

void Camera::moveRight(double amount)
{
    glm::vec3 scaledRight = glm::vec3(
                                right.x * amount,
                                right.y * amount,
                                right.z * amount
                            );
    position += scaledRight;
}

void Camera::update()
{
    modelViewMatrix = glm::lookAt(
                          position,
                          position + direction,
                          up
                      );
}
