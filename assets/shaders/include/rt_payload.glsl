struct RayPayload {
    // vec3 hitPos;
    // vec3 n;
    // vec3 tangent;
    // vec3 bitangent;
    // vec2 uv;
    // float t;
    // uint matId;
    // uint type;
    // int valid;
    vec2 uv;
    float t;
    uint data;
};
const uint RT_FLAGS = gl_RayFlagsOpaqueEXT;
#define PAYLOAD_VALID(data) ((data)>>(31)&1)
#define PAYLOAD_SET_VALID(data, val) ((data)=(data & ~(1u<<31)) | uint(val)<<31)

#define PAYLOAD_TYPE(data) ((data)>>(30)&1)
#define PAYLOAD_SET_TYPE(data, val) ((data)=(data & ~(1u<<30)) | uint(val)<<30)

#define PAYLOAD_MATID(data) ((data) & 0x3FFFFFFFu)
#define PAYLOAD_SET_MATID(data, val) ((data) = ((data) & 0xC0000000u) | (uint(val) & 0x3FFFFFFFu))