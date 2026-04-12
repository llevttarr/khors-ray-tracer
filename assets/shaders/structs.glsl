
struct Mat{
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 emission;
    vec4 uv;
    ivec4 tex;
};
struct Light{
    vec3 pos;
    // vec4 ambient;
    vec3 diffuse;
    // vec4 specular;
    uint type;
    float range;
    vec3 dir;
    float cosOuter;
    vec3 tangent;
    float halfWidth;
    vec3 bitangent;
    float halfHeight;
};
struct Tri{
    vec4 v0;
    vec4 v1;
    vec4 v2;
    vec4 t;
    vec4 b;
    vec4 n;
    vec2 uv0;
    vec2 uv1;
    vec2 uv2;
    uint matId;
    uint pad0;
};
struct Sphr {
    vec4 c_r;
    uint matId;
    uint pad0, pad1, pad2;
};
struct BVH{
    vec4 mindat;
    vec4 maxdat;
};
uint makePrimDat(uint type,uint id) { return (type<<28)|(id&0x0FFFFFFFu); }
uint primType(uint dat){ return dat >> 28;} /*0-tri;1-sphr;*/
uint primId(uint dat){return dat & 0x0FFFFFFFu;}
bool isLeaf(BVH n){return (floatBitsToUint(n.maxdat.w) & 0x80000000u) != 0u;}
uint lChild(BVH n){return floatBitsToUint(n.mindat.w);}
uint rChild(BVH n){return floatBitsToUint(n.maxdat.w);}
uint leafFirst(BVH n){return floatBitsToUint(n.mindat.w);}
uint leafN(BVH n){return floatBitsToUint(n.maxdat.w)& 0x7fffffffu;}
struct RayHit{
    bool isValid;
    float t;
    uint matId;
    vec3 n;
    vec2 uv;
    vec3 hitPos;
    uint type;
    mat3 tbn;
};
struct Reservoir {
    int sampledLight;
    float wSum;
    int M;
    float W;
};
struct GBufferPixel {
    vec4 pos;
    vec4 norm;
    vec4 diffuse;
    vec2 uv;
    uint matId;
    int valid;
};
