#include "Camera.hpp"

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraUpDirection = cameraUp;

        glm::vec3 front = glm::normalize(cameraTarget - cameraPosition);
        yaw = glm::degrees(atan2(front.x, front.z));
        pitch = glm::degrees(asin(front.y));

        if (pitch > 89.0f)  pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        cameraFrontDirection = front;

        cameraRightDirection = glm::normalize(
            glm::cross(cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f))
        );
    }


    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        //TODO

        return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, cameraUpDirection);
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        //TODO
        if (direction == MOVE_FORWARD)
            cameraPosition += speed * cameraFrontDirection;

        if (direction == MOVE_BACKWARD)
            cameraPosition -= speed * cameraFrontDirection;

        if (direction == MOVE_RIGHT)
            cameraPosition += speed * cameraRightDirection;

        if (direction == MOVE_LEFT)
            cameraPosition -= speed * cameraRightDirection;

        float groundLimit = 20.0f;

        cameraPosition.x = glm::clamp(cameraPosition.x, -groundLimit, groundLimit);
        cameraPosition.z = glm::clamp(cameraPosition.z, -groundLimit, groundLimit);

        if (cameraPosition.y < 0.0f)
            cameraPosition.y = 0.0f;

    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    void Camera::rotate(float pitchOffset, float yawOffset)
    {
        pitch += pitchOffset;
        yaw += yawOffset;

        if (pitch > 89.0f)  pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        cameraFrontDirection = glm::normalize(front);

        cameraRightDirection = glm::normalize(
            glm::cross(cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f))
        );
        cameraUpDirection = glm::normalize(
            glm::cross(cameraRightDirection, cameraFrontDirection)
        );
    }

    glm::vec3 Camera::getPosition() const
    {
        return cameraPosition;
    }

    void Camera::setPosition(const glm::vec3& pos)
    {
        cameraPosition = pos;
    }

}
