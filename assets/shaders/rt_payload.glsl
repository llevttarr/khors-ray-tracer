struct RayPayload {
    vec3 hitPos;
    vec3 n;
    vec3 tangent;
    vec3 bitangent;
    vec2 uv;
    float t;
    uint matId;
    uint type;
    int valid;
};
