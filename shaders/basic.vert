#version 410 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoords;

out vec3 fPosition;
out vec3 fNormal;
out vec2 fTexCoords;

out vec3 fragPosView;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 FragPosLightSpace;
uniform mat4 lightSpaceMatrix;


void main()
{
    vec4 worldPos = model * vec4(vPosition, 1.0);
    vec4 viewPos  = view * worldPos;

    gl_Position = projection * viewPos;

    fPosition   = worldPos.xyz;
    fNormal     = mat3(model) * vNormal;
    fTexCoords  = vTexCoords;
    fragPosView = viewPos.xyz;
    FragPosLightSpace = lightSpaceMatrix * worldPos;
}
