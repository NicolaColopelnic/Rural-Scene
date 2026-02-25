#version 410 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoords;

out vec3 fPosition;
out vec3 fNormal;
out vec2 fTexCoords;
out vec3 fragPosView;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

// wind
uniform float time;
uniform vec2 windDir;
uniform float windStrength;

void main()
{
    vec3 pos = vPosition;

    float heightFactor = clamp(pos.y, 0.0, 1.0);
    float sway = sin(time + pos.y * 2.5 + pos.x * 0.5) * windStrength;

    pos.x += windDir.x * sway * heightFactor;
    pos.z += windDir.y * sway * heightFactor;

    vec4 worldPos = model * vec4(pos, 1.0);
    vec4 viewPos = view * worldPos; // This is the eye-space position
    
    gl_Position = projection * viewPos;

    fPosition = worldPos.xyz;
    fNormal = mat3(model) * vNormal;
    fTexCoords = vTexCoords;
    fragPosView = viewPos.xyz; // Pass eye-space to Fragment shader
    FragPosLightSpace = lightSpaceMatrix * worldPos;
}