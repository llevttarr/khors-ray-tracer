#version 460
#extension GL_EXT_ray_tracing : require

#include "structs.glsl"
#include "vk_uniforms.glsl"
#include "shading.glsl"
#include "refl_payload.glsl"

layout(std430,set=1,binding=3) readonly buffer MatBuf {
    Mat mats[];
};
layout(std430,set=1,binding=5) readonly buffer LightBuf {
    Light light_v[];
};
layout(set = 1, binding = 9) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadInEXT ReflPayload payload;

const float GROUND_Y = -4.0;
const uint GROUND_MAT = 1u;

void main() {
    vec3 ro = gl_WorldRayOriginEXT;
    vec3 rd = gl_WorldRayDirectionEXT;
    float denom = rd.y;
    if (abs(denom) > 0.001) {
        float t = (GROUND_Y - ro.y) / denom;
        if (t > 0.001) {
            vec3 pos = ro + t * rd;
            vec3 n = vec3(0.0, 1.0, 0.0);
            vec3 view = -rd;

            Mat mat = mats[0];
            vec2 uv = pos.xz * 0.2;
            vec2 texUV = uv * mat.uv.xy + mat.uv.zw;

            vec3 base = (mat.tex.x != -1) ? texture(baseTexArr, vec3(texUV, mat.tex.x)).rgb : mat.diffuse.rgb;

            vec3 col = mat.ambient.rgb * (0.5 + 0.5 * n.y);
            for (uint li = 0u; li < lightc; ++li) {
                LightSample ls = sampleLight(light_v[li], pos);
                if (!trace_shadow(pos + n * 0.001, ls.dir, ls.dist))
                    col += eval_brdf(pos, n, view, base, mat, texUV, GROUND_MAT, ls);
            }
            col += mat.emission.rgb * mat.emission.w;

            payload.color = col;
            payload.t = t;
            return;
        }
    }

    payload.color = getSky(rd.y);
    payload.t = -1.0;
}
