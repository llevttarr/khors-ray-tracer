#version 460
#extension GL_EXT_ray_tracing : require

#include "shading.glsl"
#include "refl_payload.glsl"

layout(location = 0) rayPayloadInEXT ReflPayload payload;

void main() {
    payload.color = getSky(gl_WorldRayDirectionEXT.y);
    payload.t = -1.0;
}
