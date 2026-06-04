
layout(set = 0, binding = 0) uniform CameraData {
    vec4 pos;
    vec4 forward;
    vec4 right;
    vec4 up;
    vec4 prev_pos;
    vec4 prev_forward;
    vec4 prev_right;
    vec4 prev_up;
} cam;

#define camPos cam.pos.xyz
#define camFov cam.pos.w
#define camForward cam.forward.xyz
#define camRight cam.right.xyz
#define camUp cam.up.xyz

#define prevCamPos cam.prev_pos.xyz
#define prevCamFov cam.prev_pos.w
#define prevCamForward cam.prev_forward.xyz
#define prevCamRight cam.prev_right.xyz
#define prevCamUp cam.prev_up.xyz
#define prevAspect cam.prev_forward.w
