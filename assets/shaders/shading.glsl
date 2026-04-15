
vec3 getSky(float ndcy){
    float t= ndcy/2.0;
    float tr=max(0.6,0.8+t);
    float tg=max(0.4,0.965+t);
    float tb=max(0.5,1.0+t);
    return vec3(tr,tg,tb);
}
vec3 normalMapping(vec3 normMap,mat3 TBN){
    vec3 mapN=normalize(normMap*2.0-1.0);
    return normalize(TBN*mapN);
}
mat3 getTBNSphere(vec3 n){
    vec3 anyv = abs(n.x)>0.9?vec3(0.0,1.0,0.0):vec3(1.0,0.0,0.0);
    vec3 t=normalize(cross(anyv,n));
    vec3 b = normalize(cross(t,n));
    return mat3(t,b,n);
}
float blinnPhongSpecStr(vec3 v,vec3 ptol,vec3 n,float sh){
    vec3 halfw=v+ptol;
    float blinnAngle=dot(halfw,n);
    float specStr =pow(max(0.0,blinnAngle),sh);
    return specStr;
}
float phongSpecStr(vec3 v,vec3 ptol,vec3 n,float sh){
    vec3 ref=reflect(-ptol,n);
    float rDotV=max(0.0,dot(ref,v));
    float specStr =pow(rDotV,sh);
    return specStr;
}
vec3 brdfShading(vec3 rayOrigin,vec3 rayVector,RayHit hit){
    Mat mat=mats[hit.matId-1];
    vec3 col=mat.ambient.rgb;

    vec2 texUV=hit.uv *mat.uv.xy + mat.uv.zw;
    vec3 base=mat.diffuse.rgb;
    vec3 normMap;
    vec3 specMap;

    if (mat.tex.x!=-1){
        base=texture(baseTexArr,vec3(texUV, mat.tex.x)).rgb;
    }
    if (mat.tex.y!=-1){
        normMap=texture(normalTexArr,vec3(texUV,mat.tex.y)).rgb;
    }
    if (mat.tex.z!=-1){
        specMap=texture(specularTexArr,vec3(texUV,mat.tex.z)).rgb;
    }
    // col=0.2*base + 0.7*mat.ambient.rgb;

    vec3 point=hit.hitPos;
    vec3 v=normalize(rayOrigin-point);
    vec3 n=hit.n;
    mat3 tbn;
    if (hit.type==1){ // sphere
        tbn=getTBNSphere(n);
    }else if (hit.type==0){
        tbn=hit.tbn;
    }
    if(dot(n,v)<0.0){
        n=-n;
        tbn=-tbn;
    }
    if (hit.matId!=1){
        n=normalMapping(normMap,tbn);
    }
    float hemi = 0.5+0.5*n.y;
    col*=hemi;
    float sh=max(1.0,mat.ambient.w);
    for (uint i=0;i<lightc;++i){
        Light l= light_v[i];
        LightSample s = sampleLight(l, point);
        // vec3 diff=base;
        if (traceAny(point,s.dir,s.dist)){
            continue;
        }
        float nDot = max(0.0, dot(n, s.dir));
        vec3 diff = base * s.radiance * nDot;
        float specStr;
        if (brdf_type==0){
            specStr =phongSpecStr(v,s.dir, n, sh);
        }else{
            specStr =blinnPhongSpecStr(v,s.dir,n,sh);
        }
        vec3 spec = mat.specular.rgb*s.radiance * specStr;
        if (hit.matId!=1){
            spec*=specMap;
        }
        col += diff+spec;
    }
    return col;
}
