#include "Camera.h"
#include <iostream>

static const double kPi = 3.14159265358979323846;

Camera::Camera(int width, int height)
{
    const float viewportWidth = static_cast<float>(width);
    const float viewportHeight = static_cast<float>(height);
    std::cout << "Camera viewport: " << viewportWidth << "x" << viewportHeight << std::endl; 
    const float aspect = (viewportWidth > 0.0f && viewportHeight > 0.0f)
        ? (viewportWidth / viewportHeight)
        : (16.0f / 9.0f);
    projectionMatrix = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10000.0f);
    horizontalAngle = kPi;
    verticalAngle = 0.0;
    position = glm::vec3(0.0, 1.0, 0.0);
    aim(0.0, 0.0);
    update();
}


void Camera::aim(double x, double y)
{
    horizontalAngle -= x;
    verticalAngle += y;

    direction = glm::vec3(
		cos(verticalAngle) * sin(horizontalAngle),
		sin(verticalAngle),
		cos(verticalAngle) * cos(horizontalAngle)
	);
    right = glm::vec3(
        sin(horizontalAngle - kPi/2.0),
		0.0,
        cos(horizontalAngle - kPi/2.0)
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

