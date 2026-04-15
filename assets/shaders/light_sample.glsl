struct LightSample {
    vec3 dir;
    float dist;
    vec3 radiance;
};

LightSample sampleLight(Light l, vec3 pos) {
    LightSample s;
    if (l.type == 0) {//LIGHT_POINT
        vec3 ptol = l.pos - pos;
        float distSq = max(dot(ptol, ptol), 0.001);
        s.dist = sqrt(distSq);
        s.dir = ptol / s.dist;
        float att = 1.0 / sqrt(distSq);

        s.radiance = l.diffuse * att;
    }

    else if (l.type ==3) {//LIGHT_DIRECTION
        s.dir = normalize(-l.dir);
        s.dist = 1e20;

        s.radiance = l.diffuse;
    }

    else if (l.type == 1) {//LIGHT_SPOT
        vec3 ptol = l.pos - pos;
        float distSq = max(dot(ptol, ptol), 0.001);
        s.dist = sqrt(distSq);
        s.dir = ptol / s.dist;

        float theta = dot(s.dir, normalize(-l.dir));

        float spot = smoothstep(l.cosOuter, l.cosOuter + 0.05, theta);

        float att = spot / sqrt(distSq);

        s.radiance = l.diffuse * att;
    }

    else if (l.type == 2) {//LIGHT_AREA
        vec3 ptol = l.pos - pos;
        float distSq = max(dot(ptol, ptol), 0.001);
        s.dist = sqrt(distSq);
        s.dir = ptol / s.dist;

        float att = 1.0 / sqrt(distSq);

        s.radiance = l.diffuse * att;
    }

    else {
        s.dir = vec3(0);
        s.dist = 1e20;
        s.radiance = vec3(0);
    }

    return s;
}
