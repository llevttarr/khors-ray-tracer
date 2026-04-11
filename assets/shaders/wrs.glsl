const vec3 LUMINANCE=vec3(0.2126, 0.7152, 0.0722); 
void wrs_update(inout Reservoir r, int candidate, float w, float xi) {
    r.wSum += w;
    r.M += 1;
    if (xi * r.wSum < w)
        r.y = candidate;
}
float targetPDF(int lightIdx, vec3 pos, vec3 n, vec3 diffuse) {
    Light l = light_v[lightIdx];
    vec3  ptol = l.pos.xyz - pos;
    float dist = length(ptol);
    float ndotl = max(0.0, dot(n, ptol / dist));
    vec3  contrib = diffuse *l.diffuse.rgb * ndotl/(dist*dist);
    return dot(contrib,LUMINANCE);
}
void finalize_reservoir(inout Reservoir r, vec3 pos, vec3 n, vec3 diffuse) {
    if (r.y < 0 || r.M == 0) { 
        r.W = 0.0; 
        return;
    }
    float p_hat=targetPDF(r.y, pos, n, diffuse);
    r.W = (p_hat > 0.0) ? (r.wSum / (float(r.M) * p_hat)) : 0.0;
}
