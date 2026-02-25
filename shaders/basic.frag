#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec3 fragPosView;
in vec4 FragPosLightSpace;

out vec4 fColor;

// matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;

// directional lighting
uniform vec3 lightDir;
uniform vec3 lightColor;

// point lighting
uniform vec3 pointLightPos;
uniform vec3 pointLightColor;
uniform vec3 pointLightPos2;
uniform vec3 pointLightColor2;

// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

// shadow mapping
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;

// fog
uniform bool fogEnabled;
uniform vec3 fogColor;
uniform float fogDensity;

// components
vec3 ambient;
vec3 diffuse;
vec3 specular;

const float ambientStrength = 0.2f;
const float specularStrength = 0.5f;
const float shininess = 32.0f;

void computeDirLight(vec3 fragPosViewIn, vec3 fNormalIn) {
    // compute eye space coordinates
    vec3 normalEye = normalize(normalMatrix * fNormalIn);
    
    // normalize light direction
    vec3 lightDirN = normalize((view * vec4(lightDir, 0.0)).xyz);
    
    // compute view direction
    vec3 viewDir = normalize(-fragPosViewIn);
    
    // compute ambient light
    ambient = ambientStrength * lightColor;
    
    // compute diffuse light
    diffuse = max(dot(normalEye, lightDirN), 0.0) * lightColor;
    
    // compute specular light
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    specular = specularStrength * specCoeff * lightColor;
}

vec3 PointLight(vec3 normalEye, vec3 fragPosEye, vec3 viewDir, 
                vec3 lightPosWorld, vec3 lightCol) 
{
    vec3 lightPosEye = (view * vec4(lightPosWorld, 1.0)).xyz;
    vec3 L = normalize(lightPosEye - fragPosEye);
    
    float distance = length(lightPosEye - fragPosEye);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    
    // ambient light
    vec3 ambientPt = ambientStrength * lightCol;
    
    // diffuse light
    float diff = max(dot(normalEye, L), 0.0);
    vec3 diffusePt = diff * lightCol;
    
    // specular light
    vec3 halfVector = normalize(L + viewDir);
    float spec = pow(max(dot(normalEye, halfVector), 0.0), shininess);
    vec3 specularPt = specularStrength * spec * lightCol;
    
    return attenuation * (ambientPt + diffusePt + specularPt);
}

float computeShadow()
{
    vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = max(0.005 * (1.0 - dot(normalize(fNormal),
                                       normalize(lightDir))), 0.001);

    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    return shadow * 0.6;
}

void main() 
{
    vec3 fragPosEye = fragPosView;
    vec3 viewDirEye = normalize(-fragPosEye);
    
    computeDirLight(fragPosEye, fNormal);
    vec3 dirLight = ambient + diffuse + specular;
    
    vec3 normalEye = normalize(normalMatrix * fNormal);
    
    vec3 point1 = PointLight(normalEye, fragPosEye, viewDirEye, pointLightPos, pointLightColor);
    vec3 point2 = PointLight(normalEye, fragPosEye, viewDirEye, pointLightPos2, pointLightColor2);
    
    vec3 lightTotal = dirLight + point1 + point2;
    
    float shadow = computeShadow();
    lightTotal *= (1.0 - shadow);
    
    vec3 diffuseTex = texture(diffuseTexture, fTexCoords).rgb;
    vec3 specularTex = texture(specularTexture, fTexCoords).rgb;
    
    vec3 color = lightTotal * diffuseTex;
    color += specularTex * 0.12;
    
    if (fogEnabled) {
        float distance = length(fragPosEye);
        float fogFactor = exp(-fogDensity * distance);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        color = mix(fogColor, color, fogFactor);
    }
    
    fColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
