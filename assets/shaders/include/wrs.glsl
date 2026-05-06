const vec3 LUMINANCE=vec3(0.2126, 0.7152, 0.0722); 

void wrs_update(inout Reservoir r, int candidate, float w, float xi) {
    r.wSum += w;
    r.M += 1;
    if (xi * r.wSum < w)
        r.sampledLight = candidate;
}
float targetPDF(int lightIdx, vec3 pos, vec3 n, vec3 diffuse) {
    Light l = light_v[lightIdx];
    LightSample s = sampleLight(l, pos);
    float ndotl = max(0.0, dot(n, s.dir));
    vec3 contrib = (diffuse + vec3(0.05)) * l.diffuse.rgb * ndotl /*/ distSq*/;
    return dot(contrib,LUMINANCE);
}
void finalize_reservoir(inout Reservoir r, vec3 pos, vec3 n, vec3 diffuse) {
    if (r.sampledLight < 0 || r.M == 0) { 
        r.W = 0.0; 
        return;
    }
    float p_hat=targetPDF(r.sampledLight, pos, n, diffuse);
    r.W = (p_hat > 0.0) ? (r.wSum / (float(r.M) * p_hat)) : 0.0;
}
