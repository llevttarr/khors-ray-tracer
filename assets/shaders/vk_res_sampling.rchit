#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#include "structs.glsl"
#include "uniforms.glsl"
#include "rt_payload.glsl"
layout(set = 1, binding = 0, std430) readonly buffer TriBuf { Tri  tris[];};
layout(set = 1, binding = 3, std430) readonly buffer MatBuf { Mat  mats[];};
layout(location = 0) rayPayloadInEXT RayPayload payload;

hitAttributeEXT vec2 barycentrics;

void main() {
    uint triIdx = uint(gl_PrimitiveID);
    Tri tri = tris[triIdx];
    float u = barycentrics.x;
    float v = barycentrics.y;
    float w = 1.0 - u - v;
    vec3 v0 = tri.v0.xyz;
    vec3 v1 = tri.v1.xyz;
    vec3 v2 = tri.v2.xyz;
    vec3 hitPos = w * v0 + u * v1 + v * v2;
    vec2 uv = w * tri.uv0 + u * tri.uv1 + v * tri.uv2;

    // PAYLOAD FILL

    payload.hitPos = hitPos;
    payload.t = gl_HitTEXT;
    payload.n = tri.n.xyz;
    payload.tangent = tri.t.xyz;
    payload.bitangent = tri.b.xyz;
    payload.matId = tri.matId;
    payload.uv = uv;
    payload.type = 0u;
    payload.valid = 1;
}