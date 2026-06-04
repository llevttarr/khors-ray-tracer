#version 460
#extension GL_EXT_ray_tracing : require
#include "structs.glsl"
#include "vk_uniforms.glsl"
#include "shading.glsl"
#include "refl_payload.glsl"

layout(std430,set=1,binding=0) readonly buffer TriBuf {
    Tri tris[];
};
layout(std430,set=1,binding=3) readonly buffer MatBuf {
    Mat mats[];
};
layout(std430,set=1,binding=5) readonly buffer LightBuf {
    Light light_v[];
};
layout(set = 1, binding = 6) uniform sampler2DArray baseTexArr;
layout(set = 1, binding = 7) uniform sampler2DArray normalTexArr;
layout(set = 1, binding = 8) uniform sampler2DArray specularTexArr;
layout(set = 1, binding = 9) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadInEXT ReflPayload refl_payload;
hitAttributeEXT vec2 barycentrics;

void main() {
    Tri tri = tris[gl_PrimitiveID];
    float u = bary.x, v = bary.y, w = 1.0 - u - v;
    vec3 pos = gl_WorldRayOriginEXT + gl_HitTEXT * gl_WorldRayDirectionEXT;
    vec3 view = normalize(gl_WorldRayOriginEXT - pos);

    vec3 n = normalize(tri.n.xyz);
    vec3 tangent = tri.t.xyz;
    vec3 bitangent = tri.b.xyz;
    mat3 tbn = mat3(tangent, bitangent, n);
    vec2 uv = w * tri.uv0 + u * tri.uv1 + v * tri.uv2;

    uint matId = tri.matId;
    Mat mat = mats[matId - 1];

    if (dot(n, view) < 0.0) {
        n = -n;
        tbn = mat3(tangent, -bitangent, n);
    }

    vec2 texUV = uv * mat.uv.xy + mat.uv.zw;

    vec3 base = (mat.tex.x != -1) ? texture(baseTexArr, vec3(texUV, mat.tex.x)).rgb : mat.diffuse.rgb;

    if (mat.tex.y != -1){
        n = normalMapping(texture(normalTexArr, vec3(texUV, mat.tex.y)).rgb, tbn);
    }
    vec3 col = mat.ambient.rgb * (0.5 + 0.5 * n.y);
    for (uint li = 0u; li < lightc; ++li) {
        LightSample ls = sampleLight(light_v[li], pos);
        if (!trace_shadow(pos + n * 0.001, ls.dir, ls.dist))
            col += eval_brdf(pos, n, view, base, mat, texUV, matId, ls);
    }
    col += mat.emission.rgb * mat.emission.w;

    payload.color = col;
    payload.t = gl_HitTEXT;
}
