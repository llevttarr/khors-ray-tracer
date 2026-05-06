struct LightSample {
    vec3 dir;
    float dist;
    vec3 radiance;
};

LightSample sampleLight(Light l, vec3 pos) {
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

    else {
        s.dir = vec3(0);
        s.dist = 1e20;
        s.radiance = vec3(0);
    }

    return s;
}
