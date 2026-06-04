#version 460
#extension GL_EXT_ray_tracing : require
#include "structs.glsl"
#include "vk_uniforms.glsl"
#include "refl_payload.glsl"
layout(std430,set=1,binding=0) readonly buffer TriBuf {
    Tri tris[];
};
layout(location = 0) rayPayloadInEXT ReflPayload refl_payload;

hitAttributeEXT vec2 barycentrics;

struct HitInfo { vec3 pos, n, tangent, bitangent; vec2 uv; uint matId; };

HitInfo decode_refl(vec3 ro, vec3 rd) {
    HitInfo h;
    h.pos = ro + rd * refl_payload.t;
    if (PAYLOAD_TRIID(refl_payload.data) == GROUND_ID) {
        h.n = vec3(0,1,0); h.tangent = vec3(1,0,0); h.bitangent = vec3(0,0,1);
        h.uv = h.pos.xz * 0.2;  h.matId = 1u;
    } else {
        Tri tri = tris[PAYLOAD_TRIID(refl_payload.data)];
        float u = refl_payload.uv.x, v = refl_payload.uv.y, w = 1.0-u-v;
        h.n = tri.n.xyz;
        h.tangent = tri.t.xyz;
        h.bitangent = tri.b.xyz;
        h.uv = w*tri.uv0 + u*tri.uv1 + v*tri.uv2;
        h.matId = tri.matId;
    }
    return h;
}

vec3 eval_brdf(vec3 pos, vec3 n, vec3 v, vec3 base, Mat mat, vec2 texUV, uint matId, LightSample ls) {
    float ndotl = max(0.0, dot(n, ls.dir));
    float sh = max(1.0, mat.ambient.w);
    float specStr = (brdf_type == 0) ? phongSpecStr(v, ls.dir, n, sh) : blinnPhongSpecStr(v, ls.dir, n, sh);
    float hasSpec = float(matId != 1u && mat.tex.z != -1);
    vec3 specMap = mix(vec3(1.0), texture(specularTexArr, vec3(texUV, max(mat.tex.z,0))).rgb, hasSpec);
    return base * ls.radiance * ndotl + mat.specular.rgb * ls.radiance * specStr * specMap;
}

vec3 shade_reflection(vec3 reflO, vec3 reflDir) {
    HitInfo h = decode_refl(reflO, reflDir);
    Mat mat = mats[h.matId - 1];
    vec3 v = normalize(reflO - h.pos);
    mat3 tbn = mat3(h.tangent, h.bitangent, h.n);
    vec3 n = h.n;
    if (dot(n, v) < 0.0) { n = -n; tbn = -tbn; }

    vec2 texUV = h.uv * mat.uv.xy + mat.uv.zw;
    vec3 base = (mat.tex.x != -1) ? texture(baseTexArr, vec3(texUV, mat.tex.x)).rgb : mat.diffuse.rgb;
    if (h.matId != 1u && mat.tex.y != -1)
        n = normalMapping(texture(normalTexArr, vec3(texUV, mat.tex.y)).rgb, tbn);

    vec3 col = mat.ambient.rgb * (0.5 + 0.5 * n.y);
    for (uint li = 0u; li < lightc; ++li) {
        LightSample ls = sampleLight(light_v[li], h.pos);
        if (!trace_shadow(h.pos + n * 0.001, ls.dir, ls.dist))
            col += eval_brdf(h.pos, n, v, base, mat, texUV, h.matId, ls);
    }
    return col + mat.emission.rgb * mat.emission.w;
}

void main() {
    refl_payload.t = gl_HitTEXT;
    refl_payload.color = shade_reflection(gl_WorldRayOriginEXT,gl_WorldRayDirectionEXT);
}
