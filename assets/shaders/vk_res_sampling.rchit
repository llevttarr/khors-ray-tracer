#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#include "structs.glsl"
#include "vk_uniforms.glsl"
#include "rt_payload.glsl"
layout(set = 1, binding = 0, std430) readonly buffer TriBuf { Tri  tris[];};
layout(set = 1, binding = 3, std430) readonly buffer MatBuf { Mat  mats[];};
layout(location = 0) rayPayloadInEXT RayPayload payload;

hitAttributeEXT vec2 barycentrics;

void main() {
    payload.uv = barycentrics;
    payload.t = gl_HitTEXT;
    payload.data = 0u;
    PAYLOAD_SET_VALID(payload.data, 1u);
    PAYLOAD_SET_TRIID(payload.data, uint(gl_PrimitiveID));
}