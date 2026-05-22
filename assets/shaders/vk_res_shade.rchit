#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

#include "structs.glsl"
#include "vk_uniforms.glsl"
#include "rt_payload.glsl"

layout(set = 1, binding = 0, std430) readonly buffer TriBuf { Tri tris[]; };
layout(set = 1, binding = 3, std430) readonly buffer MatBuf { Mat  mats[];};

layout(location = 0) rayPayloadInEXT RayPayload payload;
hitAttributeEXT vec2 barycentrics;

void main() {
    uint triIdx = uint(gl_PrimitiveID);
    float u = barycentrics.x;
    float v = barycentrics.y;
    float w = 1.0 - u - v;
    vec2 uv = w * tri.uv0 + u * tri.uv1 + v * tri.uv2;

    // PAYLOAD FILL
    payload.uv = uv;
    payload.t = gl_HitTEXT;
    PAYLOAD_SET_VALID(payload,1);
    PAYLOAD_SET_TYPE(payload,0);
    PAYLOAD_SET_TRIID(payload, triIdx);
}
