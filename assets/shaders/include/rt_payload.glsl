struct RayPayload {
    vec2 uv;
    float t;
    uint data;
};
const uint RT_FLAGS = gl_RayFlagsOpaqueEXT;
#define PAYLOAD_VALID(data) ((data)>>(31)&1)
#define PAYLOAD_SET_VALID(data, val) ((data)=(data & ~(1u<<31)) | uint(val)<<31)

// #define PAYLOAD_TYPE(data) ((data)>>(30)&1)
// #define PAYLOAD_SET_TYPE(data, val) ((data)=(data & ~(1u<<30)) | uint(val)<<30)

#define PAYLOAD_TRIID(data) ((data) & 0x3FFFFFFFu)
#define PAYLOAD_SET_TRIID(data, val) ((data) = ((data) & 0xC0000000u) | (uint(val) & 0x3FFFFFFFu))