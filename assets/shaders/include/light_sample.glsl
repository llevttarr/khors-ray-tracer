struct LightSample {
    vec3 dir;
    float dist;
    vec3 radiance;
};

LightSample sampleLight(Light l, vec3 pos, vec2 xi) {
    LightSample s;
    int type= int(l.dir_type.w);
    vec3 dir= l.dir_type.xyz;
    float range = l.params1.x;
    float cosOuter = l.params1.y;
    float halfW = l.params1.z;
    float halfH  = l.params1.w;
    if (type == 0) {//LIGHT_POINT
        vec3 ptol = l.pos.xyz - pos;
        float distSq = max(dot(ptol, ptol), 0.001);
        s.dist = sqrt(distSq);
        s.dir = ptol / s.dist;
        float att = 1.0 / sqrt(distSq);

        s.radiance = l.diffuse.xyz * att;
    }

    else if (type ==3) {//LIGHT_DIRECTION
        s.dir = normalize(-dir);
        s.dist = 1e20;

        s.radiance = l.diffuse.xyz;
    }

    else if (type == 1) {//LIGHT_SPOT
        vec3 ptol = l.pos.xyz - pos;
        float distSq = max(dot(ptol, ptol), 0.001);
        s.dist = sqrt(distSq);
        s.dir = ptol / s.dist;

        float theta = dot(s.dir, normalize(-dir));

        float spot = smoothstep(cosOuter,cosOuter + 0.05, theta);

        float att = spot / sqrt(distSq);

        s.radiance = l.diffuse.xyz * att;
    }

    else if (type == 2) {//LIGHT_AREA
        vec3 ptol = l.pos.xyz - pos;
        float distSq = max(dot(ptol, ptol), 0.001);
        s.dist = sqrt(distSq);
        s.dir = ptol / s.dist;

        float att = 1.0 / sqrt(distSq);

        s.radiance = l.diffuse.xyz * att;
    }
    else if (type == 4) { // LIGHT_TRIANGLE
        vec3 e0 = l.tangent.xyz;
        vec3 e1 = l.bitangent.xyz;
        vec3 v0 = l.pos.xyz - (e0 + e1) / 3.0;
        float r1 = xi.x, r2 = xi.y;
        if (r1 + r2 > 1.0) { 
            r1 = 1.0 - r1; 
            r2 = 1.0 - r2; 
        }
        vec3 sampleP = v0 + r1 * e0 + r2 * e1;

        vec3 toLight = sampleP - pos;
        float distSq = max(dot(toLight, toLight), 1e-6);
        s.dist = sqrt(distSq);
        s.dir = toLight / s.dist;

        float cosL = dot(-s.dir, l.dir_type.xyz);
        if (cosL > 0.0) {
            vec3 Le = l.diffuse.xyz * l.diffuse.w;
            float area = l.params1.x; 
            s.radiance = Le * (cosL * area / distSq);
        } else {
            s.radiance = vec3(0.0);
        }
    }

    else {
        s.dir = vec3(0);
        s.dist = 1e20;
        s.radiance = vec3(0);
    }

    return s;
}