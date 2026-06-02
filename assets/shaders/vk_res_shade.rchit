#version 460
#extension GL_EXT_ray_tracing : require
#include "rt_payload.glsl"
#include "rt_shadow_payload.glsl"
layout(location = 0) rayPayloadInEXT ShadowPayload shadowPayload;
void main() {
    shadowPayload.shadowed = 1;
}
