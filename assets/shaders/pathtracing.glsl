const float REFL_T_THRESHOLD=100.0;

vec3 getReservoirTracingResult(vec3 rayOrigin,vec3 rayVector,float ndcy){
    RayHit hit=trace(rayOrigin,rayVector);
    vec3 sky=getSky(ndcy);
    if (!hit.isValid){
        return sky;
    }
    // TODO: change this code
    vec3 base=brdfShading(rayOrigin,rayVector,hit);
    
    Mat mat = mats[hit.matId-1];
    float reflStr = mat.specular.w;
    if (hit.t>REFL_T_THRESHOLD || reflStr<= 0.01) {
        return base;
    }

    vec3 p = rayOrigin+rayVector*hit.t;
    vec3 n = normalize(hit.n);
    vec3 reflO = p +n* 0.001;
    vec3 reflDir = normalize(reflect(rayVector, n));

    RayHit reflHit = trace(reflO,reflDir);
    vec3 reflCol = sky;
    if (reflHit.isValid) {
        reflCol = brdfShading(reflO,reflDir,reflHit);
    }
    return mix(base,reflCol, reflStr);
}
