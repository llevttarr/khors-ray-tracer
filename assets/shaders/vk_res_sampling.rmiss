#version 460
#extension GL_EXT_ray_tracing : require

//#import "rt_payload.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    payload.valid = -1;
    payload.t = 1e30;
}
