#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec3 fragPosView; // Matching vertex shader
in vec4 FragPosLightSpace;

out vec4 fColor; // Declared as fColor

uniform sampler2D diffuseTexture;
uniform vec3 lightDir;
uniform vec3 lightColor;

uniform bool fogEnabled;
uniform vec3 fogColor;
uniform float fogDensity;

void main()
{
    vec4 texColor = texture(diffuseTexture, fTexCoords);

    if (texColor.a < 0.1)
        discard;

    vec3 normal = normalize(fNormal);
    vec3 lightDirN = normalize(-lightDir);

    float diff = max(dot(normal, lightDirN), 0.0);

    vec3 ambient = 0.4 * lightColor;
    vec3 diffuse = diff * lightColor;

    vec3 color = (ambient + diffuse) * texColor.rgb;

    // FOG COMPUTATION
    if (fogEnabled) {
        float dist = length(fragPosView); // Use fragPosView here
        float fogFactor = exp(-fogDensity * dist);
        
        // LIMIT FOG STRENGTH (To make it lighter/transparent)
        // Change 0.0 to 0.3 if you want the tree to ALWAYS be 30% visible
        fogFactor = clamp(fogFactor, 0.0, 1.0); 
        
        color = mix(fogColor, color, fogFactor);
    }
    
    // Final output assigned to fColor
    fColor = vec4(clamp(color, 0.0, 1.0), texColor.a);
}