#version 460

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vColor;
layout(location = 2) in vec2 vUV;

layout(location = 0) out vec4 fragColor;

layout(set = 1, binding = 0) uniform sampler2D texAlbedo;
layout(set = 1, binding = 1) uniform sampler2D texNormal;

//const vec3 gLightDir = vec3(0.577, -0.577, 0.577);
const vec3 gLightDir = vec3(-1, 2, 3);

void main()
{
    // TODO: norm/tan/bitan transformation so that transformations don't mess with the normals etc

    float diffuseValue = dot(vNormal, normalize(gLightDir));
    float halfLambert = diffuseValue * 0.5 + 0.5;
    vec3 texColor = texture(texAlbedo, vUV).xyz;
    fragColor = vec4(texColor * halfLambert, 1.0);
}
