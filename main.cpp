#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint viewLoc;
GLint projectionLoc;
GLint lightDirLoc;
GLint lightColorLoc;

// camera
gps::Camera myCamera(
    glm::vec3(-12.0f, 4.0f, -12.0f), // where the camera is
    glm::vec3(0.0f, -4.0f, 0.0f), // where the camera looks
    glm::vec3(0.0f, 1.0f, 0.0f) // which way is up
);

GLfloat speed = 4.0f;
float lastFrame = 0.0f;

GLboolean pressedKeys[1024];

// models
gps::Model3D ground;

gps::Model3D tree;
struct TreeInstance {
    glm::vec3 position;
    float scale;
    float rotation;
};
std::vector<TreeInstance> trees;

gps::Model3D house;

gps::Model3D fence;
struct FenceSegment {
    glm::vec3 position;
    float rotation;
    float length;
    float halfWidth = 0.3f;
    float halfHeight = 0.5f;
    float halfLength = 2.0f;
};

std::vector<FenceSegment> fences;

struct Collider {
    glm::vec3 position;
    float radius;
};
std::vector<Collider> colliders;

gps::Model3D cat;
gps::Model3D chick;
gps::Model3D bird;
gps::Model3D horse;
gps::Model3D dog;
gps::Model3D duck;
gps::Model3D rabbit;
gps::Model3D cow;
gps::Model3D sheep;

gps::Model3D lamp;
glm::vec3 lampPosition = glm::vec3(3.0f, -3.0f, 5.0f);
glm::vec3 lampPosition2 = glm::vec3(2.0f, -3.3f, -13.5f);
glm::vec3 lampColor = glm::vec3(1.0f, 0.9f, 0.7f) * 3.0f;

gps::Model3D door;

bool doorOpen = false;
bool doorAnimating = false;
float doorAngle = 0.0f;
float doorTargetAngle = 0.0f;
float doorSpeed = 90.0f;

struct DoorCollider {
    glm::vec3 start, end;
};
DoorCollider doorColl;

GLfloat catAngle = 0.0f;

// shaders
gps::Shader myBasicShader;
gps::Shader skyboxShader;
gps::Shader treeShader;

// skybox
gps::SkyBox mySkyBox;

bool fogEnabled = false;

// shadow mapping
const GLuint SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
GLuint depthMapFBO, depthMap;
glm::mat4 lightProjection, lightView, lightSpaceMatrix;
GLint shadowMapLoc;
gps::Shader depthShader;

// camera animation
struct CameraKeyframe {
    glm::vec3 position;
    glm::vec3 target;
    float time;
};

std::vector<CameraKeyframe> path = {
    {glm::vec3(-12.0f, 4.0f, -12.0f), glm::vec3(0.0f, -4.0f, 0.0f), 0.0f},
    {glm::vec3(0.0f, 20.0f, -20.0f),  glm::vec3(0.0f, -4.0f, 0.0f), 4.0f},
    {glm::vec3(8.0f, 3.0f, 8.0f),     glm::vec3(0.0f, -2.0f, 0.0f), 8.0f},
};

bool isAnimating = false;
float animationStart = 0.0f;


GLenum glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM:
            error = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error = "INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            error = "INVALID_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            error = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
    glViewport(0, 0, width, height);

    projection = glm::perspective(
        glm::radians(65.0f),
        (float)width / (float)height,
        0.1f, 300.0f
    );
}

void updateCameraAnimation(float currentTime) {
    float animationSpeed = 1.0f;
    float elapsed = (currentTime - animationStart) * animationSpeed;

    if (elapsed > path.back().time) { isAnimating = false; return; }

    for (size_t i = 0; i < path.size() - 1; i++) {
        auto& start = path[i];
        auto& end = path[i + 1];

        if (elapsed >= start.time && elapsed <= end.time) {
            float t = (elapsed - start.time) / (end.time - start.time);

            myCamera.setPosition(glm::mix(start.position, end.position, t));
            myCamera.setTarget(glm::mix(start.target, end.target, t));
            break;
        }
    }
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {

    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        fogEnabled = !fogEnabled;
    }

    if (key == GLFW_KEY_G && action == GLFW_PRESS) {
        doorOpen = !doorOpen;
        doorTargetAngle = doorOpen ? 90.0f : 0.0f;
        doorAnimating = true;
    }

    if (key == GLFW_KEY_V && action == GLFW_PRESS) {
        isAnimating = !isAnimating;
        if (isAnimating) animationStart = (float)glfwGetTime();
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}


void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    static bool firstMouse = true;
    static float lastX = 512.0f;
    static float lastY = 384.0f;

    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
        return;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;

    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    myCamera.rotate(yoffset, xoffset);

}


bool pointInsideOBB(glm::vec3 point, const FenceSegment& fence) {
    glm::vec3 localPoint = point - fence.position;

    float angleRad = glm::radians(fence.rotation);
    float cosA = cos(angleRad);
    float sinA = sin(angleRad);

    glm::vec2 localXZ(
        cosA * localPoint.x + sinA * localPoint.z,
        -sinA * localPoint.x + cosA * localPoint.z
    );

    float camRadiusXZ = 0.6f;

    return (abs(localXZ.x) < fence.halfLength + camRadiusXZ) &&
        (abs(localXZ.y) < fence.halfWidth + camRadiusXZ);
}


bool checkCollisions(glm::vec3 cameraPos) {
    float camRadius = 0.6f;
    glm::vec2 camXZ(cameraPos.x, cameraPos.z);

    // tree collision
    for (const auto& c : colliders) {
        float dist = glm::length(camXZ - glm::vec2(c.position.x, c.position.z));
        if (dist < camRadius + c.radius) return true;
    }

    // fence collision
    for (const auto& f : fences) {
        if (pointInsideOBB(cameraPos, f)) return true;
    }

    // door collision
    if (!doorOpen && pointInsideOBB(cameraPos, { doorColl.start + (doorColl.end - doorColl.start) * 0.5f, -20.0f, 3.6f })) {
        return true;
    }

    return false;
}


void tryMoveCamera(gps::MOVE_DIRECTION dir, float cameraSpeed)
{
    glm::vec3 oldPos = myCamera.getPosition();

    myCamera.move(dir, cameraSpeed);

    if (checkCollisions(myCamera.getPosition()))
        myCamera.setPosition(oldPos);
}


void processMovement(float cameraSpeed) {

    if (pressedKeys[GLFW_KEY_W]) {
        tryMoveCamera(gps::MOVE_FORWARD, cameraSpeed);
    }

    if (pressedKeys[GLFW_KEY_S]) {
        tryMoveCamera(gps::MOVE_BACKWARD, cameraSpeed);
    }

    if (pressedKeys[GLFW_KEY_A]) {
        tryMoveCamera(gps::MOVE_LEFT, cameraSpeed);
    }

    if (pressedKeys[GLFW_KEY_D]) {
        tryMoveCamera(gps::MOVE_RIGHT, cameraSpeed);
    }

    if (pressedKeys[GLFW_KEY_Q]) {
        catAngle -= 60.0f * cameraSpeed;
    }
    if (pressedKeys[GLFW_KEY_E]) {
        catAngle += 60.0f * cameraSpeed;
    }
}


void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project Core");
}


void setWindowCallbacks() {
    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}


void initOpenGLState() {
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_BACK); // cull back face
    glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}


void initModels() {
    ground.LoadModel("models/ground/BlackdownRings02.obj", "models/ground/");
    tree.LoadModel("models/tree/Tree.obj", "models/tree/");
    house.LoadModel("models/house/cabin.obj", "models/house/");
    fence.LoadModel("models/fence/fence.obj", "models/fence/");
    door.LoadModel("models/fence/fence.obj", "models/fence/");
    cat.LoadModel("models/animals/12221_Cat_v1_l3.obj", "models/animals/");
    bird.LoadModel("models/animals/12248_Bird_v1_L2.obj", "models/animals/");
    horse.LoadModel("models/animals/10026_Horse_v01-it2.obj", "models/animals/");
    rabbit.LoadModel("models/animals/12956_WhiteHare_v1.obj", "models/animals/");
    chick.LoadModel("models/animals/chick.obj", "models/animals/");
    dog.LoadModel("models/animals/13044_Doberman_Pinscher_v1_l3.obj", "models/animals/");
    sheep.LoadModel("models/animals/13574_Marco_Polo_Sheep_v1_L3.obj", "models/animals/");
    duck.LoadModel("models/animals/12249_Bird_v1_L2.obj", "models/animals/");
    cow.LoadModel("models/animals/cow.obj", "models/animals/");
    lamp.LoadModel("models/lamp/uploads_files_3514027_Street_Lamp_1.obj", "models/lamp/");
}


void initShadowMap() {
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glGenFramebuffers(1, &depthMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void initSkybox()
{
    skyboxShader.loadShader(
        "shaders/skyboxShader.vert",
        "shaders/skyboxShader.frag"
    );

    std::vector<const GLchar*> faces = {
        "skybox/valley_ft.jpg",
        "skybox/valley_bk.jpg",
        "skybox/valley_up.jpg",
        "skybox/valley_dn.jpg",
        "skybox/valley_rt.jpg",
        "skybox/valley_lf.jpg"
    };

    mySkyBox.Load(faces);

}


void initShaders() {
    myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    depthShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
    treeShader.loadShader("shaders/tree.vert", "shaders/tree.frag");
}


void initUniforms() {
    myBasicShader.useShaderProgram();

    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "fogEnabled"), 1);
    glUniform3f(glGetUniformLocation(myBasicShader.shaderProgram, "fogColor"), 0.6f, 0.6f, 0.7f);
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity"), 0.001f);

    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));

    projection = glm::perspective(glm::radians(65.0f),
        (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
        0.1f, 300.0f);
    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
    lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

    lightColor = glm::vec3(0.25f, 0.25f, 0.30f);
    lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    glUniform3f(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPos"),
        lampPosition.x, lampPosition.y, lampPosition.z);
    glUniform3f(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightColor"),
        lampColor.x, lampColor.y, lampColor.z);
    glUniform3f(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPos2"),
        lampPosition2.x, lampPosition2.y, lampPosition2.z);
    glUniform3f(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightColor2"),
        lampColor.x, lampColor.y, lampColor.z);

    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "diffuseTexture"), 0);
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "specularTexture"), 1);

    shadowMapLoc = glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap");

    treeShader.useShaderProgram();

    glUniform2f(
        glGetUniformLocation(treeShader.shaderProgram, "windDir"),
        0.8f, 0.6f
    );

    glUniform1f(
        glGetUniformLocation(treeShader.shaderProgram, "windStrength"),
        0.15f
    );

}


static float rand01() {
    return (float)(rand() % 1000) / 1000.0f;
}


void generateTrees()
{
    trees.clear();
    srand((unsigned int)time(nullptr));

    float halfSize = 48.0f;
    float spacing = 12.5f;

    auto addTree = [&](float x, float z)
        {
            float jitterX = (rand01() - 0.5f) * 2.0f;
            float jitterZ = (rand01() - 0.5f) * 2.0f;

            // push trees inward from the edge
            x *= 0.4f;
            z *= 0.4f;

            TreeInstance t;
            t.position = glm::vec3(x + jitterX, -4.0f, z + jitterZ);

            // randomizing size variation
            float r = rand01();
            if (r < 0.6f)        t.scale = 1.35f + rand01() * 0.10f; // normal trees
            else if (r < 0.85f)  t.scale = 1.50f + rand01() * 0.15f; // big trees
            else                 t.scale = 1.25f + rand01() * 0.05f; // small trees

            t.rotation = rand01() * 360.0f;

            trees.push_back(t);
        };

    // front & back edges
    for (float x = -halfSize; x <= halfSize; x += spacing)
    {
        addTree(x, -halfSize);
        addTree(x, halfSize);
    }

    // left & right edges
    for (float z = -halfSize; z <= halfSize; z += spacing)
    {
        addTree(-halfSize, z);
        addTree(halfSize, z);
    }
}


void buildTreeColliders()
{
    colliders.clear();

    for (const auto& t : trees)
    {
        Collider c;
        c.position = t.position;
        c.radius = 2.5f * t.scale;
        colliders.push_back(c);
    }
}


void generateFence()
{
    fences.clear();

    glm::vec3 fenceCenter = glm::vec3(8.0f, -3.0f, -6.0f);

    float fenceHalfSize = 6.0f;
    float segmentLength = 4.0f;
    float gateWidth = 3.0f;

    auto addFence = [&](float localX, float localZ, float rotationDeg) {
        FenceSegment f;
        f.position = 0.2f + fenceCenter + glm::vec3(localX, 0.0f, localZ);
        f.rotation = rotationDeg;
        f.length = segmentLength;
        fences.push_back(f);
        };


    // front & back edges
    for (float x = -fenceHalfSize; x < fenceHalfSize; x += segmentLength)
    {
        addFence(x, -fenceHalfSize, 70.0f);
        addFence(x, fenceHalfSize, 70.0f);
    }

    // left and right edges
    for (float z = -fenceHalfSize; z < fenceHalfSize; z += segmentLength)
    {
        float gateCenter = -1.0f;
        if (abs(z - gateCenter) > gateWidth * 0.5f)
            addFence(-fenceHalfSize, z, -20.0f);
        addFence(fenceHalfSize, z, -20.0f);
    }

    // door collider
    float gateCenter = -1.0f;
    glm::vec3 doorPos = fenceCenter + glm::vec3(-fenceHalfSize, 0.0f, gateCenter);
    float doorHalfLength = 1.8f;

    glm::vec3 localStart(-doorHalfLength, 0, 0);
    glm::vec3 localEnd(doorHalfLength, 0, 0);
    glm::mat4 doorRot = glm::rotate(glm::mat4(1.0f), glm::radians(-20.0f), glm::vec3(0, 1, 0));
    doorColl.start = doorPos + glm::vec3(doorRot * glm::vec4(localStart, 1.0f));
    doorColl.end = doorPos + glm::vec3(doorRot * glm::vec4(localEnd, 1.0f));

}


void drawModel(gps::Model3D& object, gps::Shader& shader, bool depthPass,
        const glm::vec3& translate, const glm::vec3& rotateDeg,const glm::vec3& scale) 
{
    shader.useShaderProgram();

    model = glm::mat4(1.0f);
    model = glm::translate(model, translate);
    model = glm::rotate(model, glm::radians(rotateDeg.x), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(rotateDeg.y), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(rotateDeg.z), glm::vec3(0, 0, 1));
    model = glm::scale(model, scale);

    GLint modelLoc = glGetUniformLocation(shader.shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    if (!depthPass) {
        GLint normalLoc = glGetUniformLocation(shader.shaderProgram, "normalMatrix");
        if (normalLoc != -1) {
            normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
            glUniformMatrix3fv(normalLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
        }
    }

    object.Draw(shader);
}


void drawObjects(gps::Shader& shader, bool depthPass)
{
    shader.useShaderProgram();

    // ground
    drawModel(
        ground, shader, depthPass,
        glm::vec3(5.0f, -4.5f, -3.0f),
        glm::vec3(270.0f, 0.0f, 0.0f),
        glm::vec3(6.0f)
    );

    // house
    drawModel(
        house, shader, depthPass,
        glm::vec3(-8.0f, -2.0f, 0.0f),
        glm::vec3(0.0f, -100.0f, 0.0f),
        glm::vec3(0.9f)
    );

    // cat
    drawModel(
        cat, shader, depthPass,
        glm::vec3(0.0f, -3.0f, 2.0f),
        glm::vec3(-90.0f, 0.0f, 220.0f + catAngle),
        glm::vec3(0.02f)
    );

    // bird
    drawModel(
        bird, shader, depthPass,
        glm::vec3(5.0f, -3.0f, -10.0f),
        glm::vec3(-90.0f, 0.0f, 0.0f),
        glm::vec3(0.02f)
    );

    // dog
    drawModel(
        dog, shader, depthPass,
        glm::vec3(-5.0f, -2.9f, -2.0f),
        glm::vec3(-90.0f, 0.0f, 130.0f),
        glm::vec3(0.02f)
    );

    // horse
    drawModel(
        horse, shader, depthPass,
        glm::vec3(11.0f, -3.0f, -8.0f),
        glm::vec3(-90.0f, 0.0f, -60.0f),
        glm::vec3(0.002f)
    );

    // sheep
    drawModel(
        sheep, shader, depthPass,
        glm::vec3(8.0f, -3.0f, -4.0f),
        glm::vec3(-90.0f, 0.0f, -20.0f),
        glm::vec3(0.02f)
    );

    // ducks
    drawModel(
        duck, shader, depthPass,
        glm::vec3(10.0f, -3.0f, -1.0f),
        glm::vec3(-90.0f, 0.0f, -90.0f),
        glm::vec3(0.02f)
    );

    drawModel(
        duck, shader, depthPass,
        glm::vec3(6.0f, -3.0f, -9.0f),
        glm::vec3(-90.0f, 0.0f, -50.0f),
        glm::vec3(0.02f)
    );

    // cow
    drawModel(
        cow, shader, depthPass,
        glm::vec3(4.0f, -3.0f, -4.0f),
        glm::vec3(0.0f, 180.0f, 0.0f),
        glm::vec3(0.15f)
    );

    // rabbits
    drawModel(
        rabbit, shader, depthPass,
        glm::vec3(11.0f, -3.0f, -3.0f),
        glm::vec3(-90.0f, 0.0f, -200.0f),
        glm::vec3(0.1f)
    );

    drawModel(
        rabbit, shader, depthPass,
        glm::vec3(4.0f, -3.0f, -9.0f),
        glm::vec3(-90.0f, 0.0f, 200.0f),
        glm::vec3(0.1f)
    );

    // chicks
    drawModel(
        chick, shader, depthPass,
        glm::vec3(7.0f, -3.0f, -7.0f),
        glm::vec3(-90.0f, 0.0f, 0.0f),
        glm::vec3(0.005f)
    );

    drawModel(
        chick, shader, depthPass,
        glm::vec3(10.0f, -3.0f, -9.0f),
        glm::vec3(-90.0f, 0.0f, -40.0f),
        glm::vec3(0.005f)
    );

    // fence
    for (const auto& f : fences) {
        drawModel(
            fence, shader, depthPass,
            f.position,
            glm::vec3(0.0f, f.rotation, 0.0f),
            glm::vec3(1.0f)
        );
    }

    // gate
    drawModel(
        door, shader, depthPass,
        glm::vec3(2.4f, -2.7f, -3.7f),
        glm::vec3(0.0f, 158.0f + doorAngle, 0.0f),
        glm::vec3(1.0f)
    );

    for (const auto& t : trees) {
        gps::Shader& currentShader = depthPass ? shader : treeShader;

        if (!depthPass) {
            currentShader.useShaderProgram();
            glUniformMatrix4fv(glGetUniformLocation(currentShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(currentShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniform3fv(glGetUniformLocation(currentShader.shaderProgram, "lightDir"), 1, glm::value_ptr(lightDir));
            glUniform3fv(glGetUniformLocation(currentShader.shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));

            glUniform1i(glGetUniformLocation(currentShader.shaderProgram, "fogEnabled"), (int)fogEnabled);
            glUniform3f(glGetUniformLocation(currentShader.shaderProgram, "fogColor"), 0.6f, 0.6f, 0.7f);
            glUniform1f(glGetUniformLocation(currentShader.shaderProgram, "fogDensity"), 0.001f);
            glUniformMatrix4fv(glGetUniformLocation(currentShader.shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        }

        drawModel(
            tree,
            currentShader,
            depthPass,
            t.position,
            glm::vec3(0.0f, t.rotation, 0.0f),
            glm::vec3(t.scale)
        );
    }

    // lamps
    drawModel(
        lamp, shader, depthPass,
        lampPosition,
        glm::vec3(0.0f),
        glm::vec3(1.5f)
    );

    drawModel(
        lamp, shader, depthPass,
        lampPosition2,
        glm::vec3(0.0f),
        glm::vec3(1.5f)
    );
}


void renderScene()
{
    glm::vec3 lightPos = glm::vec3(-20.0f, 20.0f, -20.0f);
    lightProjection = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, 1.0f, 50.0f);
    lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    lightSpaceMatrix = lightProjection * lightView;

    // pass 1 - shadow map
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT);

    depthShader.useShaderProgram();
    GLint depthLightLoc = glGetUniformLocation(depthShader.shaderProgram, "lightSpaceTrMatrix");
    glUniformMatrix4fv(depthLightLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    drawObjects(depthShader, true);

    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // pass 2 - scene rendering
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, depthMap);

    myBasicShader.useShaderProgram();
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "fogEnabled"), (int)fogEnabled);
    glUniform1i(shadowMapLoc, 10);

    GLint basicLightSpaceLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceMatrix");
    glUniformMatrix4fv(basicLightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    // point lights
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPos"), 1, glm::value_ptr(lampPosition));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightColor"), 1, glm::value_ptr(lampColor));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPos2"), 1, glm::value_ptr(lampPosition2));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightColor2"), 1, glm::value_ptr(lampColor));

    float timeValue = glfwGetTime();

    // tree shader
    treeShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(treeShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(treeShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(treeShader.shaderProgram, "lightDir"), 1, glm::value_ptr(lightDir));
    glUniform3fv(glGetUniformLocation(treeShader.shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform1f(glGetUniformLocation(treeShader.shaderProgram, "time"), timeValue);

    glUniformMatrix4fv(glGetUniformLocation(treeShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(treeShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(treeShader.shaderProgram, "lightDir"), 1, glm::value_ptr(lightDir));
    glUniform3fv(glGetUniformLocation(treeShader.shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform1i(glGetUniformLocation(treeShader.shaderProgram, "fogEnabled"), (int)fogEnabled);

    drawObjects(myBasicShader, false);

    // skybox
    glDepthFunc(GL_LEQUAL);
    skyboxShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(glm::mat4(glm::mat3(view))));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    mySkyBox.Draw(skyboxShader, view, projection);
    glDepthFunc(GL_LESS);
}


void cleanup() {
    if (depthMapFBO) {
        glDeleteFramebuffers(1, &depthMapFBO);
    }
    if (depthMap) {
        glDeleteTextures(1, &depthMap);
    }

    glDeleteProgram(myBasicShader.shaderProgram);
    glDeleteProgram(skyboxShader.shaderProgram);
    glDeleteProgram(treeShader.shaderProgram);
    glDeleteProgram(depthShader.shaderProgram);

    myWindow.Delete();
}


int main(int argc, const char* argv[]) {

    try {
        initOpenGLWindow();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    colliders.clear();
    initOpenGLState();
    initShadowMap();
    initModels();
    generateTrees();
    buildTreeColliders();
    generateFence();
    initSkybox();
    initShaders();
    initUniforms();
    setWindowCallbacks();

    glCheckError();

    while (!glfwWindowShouldClose(myWindow.getWindow())) {

        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        float cameraSpeed = speed * deltaTime;

        if (doorAnimating) {
            float angleStep = doorSpeed * deltaTime;

            if (doorAngle < doorTargetAngle) {
                doorAngle = std::min(doorAngle + angleStep, doorTargetAngle);
            }
            else if (doorAngle > doorTargetAngle) {
                doorAngle = std::max(doorAngle - angleStep, doorTargetAngle);
            }

            if (std::abs(doorAngle - doorTargetAngle) < 1.0f) {
                doorAngle = doorTargetAngle;
                doorAnimating = false;
            }
        }

        if (isAnimating) {
            updateCameraAnimation(currentFrame);
        }
        else {
            processMovement(cameraSpeed);
        }
        renderScene();

        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());

        glCheckError();
    }

    cleanup();

    return EXIT_SUCCESS;
}
