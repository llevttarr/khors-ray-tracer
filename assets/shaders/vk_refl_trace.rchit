#version 460
#extension GL_EXT_ray_tracing : require
#include "structs.glsl"
#include "vk_uniforms.glsl"
#include "rt_payload.glsl"
layout(std430,set=1,binding=0) readonly buffer TriBuf {
    Tri tris[];
};
layout(location = 0) rayPayloadInEXT RayPayload payload;

hitAttributeEXT vec2 barycentrics;

void main() {
    Tri tri = tris[uint(gl_PrimitiveID)];

    float w = 1.0 - barycentrics.x - barycentrics.y;
    payload.uv = w * tri.uv0.xy + barycentrics.x * tri.uv1.xy + barycentrics.y * tri.uv2.xy;

    payload.t = gl_HitTEXT;
    payload.data = 0u;
    PAYLOAD_SET_TRIID(payload.data, uint(gl_PrimitiveID));
    PAYLOAD_SET_VALID(payload.data, 1u);
}
