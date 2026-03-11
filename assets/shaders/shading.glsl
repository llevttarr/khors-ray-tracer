
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
vec3 phongShading(vec3 rayOrigin,vec3 rayVector,RayHit hit){
    Mat mat=mats[hit.matId-1];
    vec3 col=mat.ambient.rgb;

    vec2 texUV=hit.uv *mat.uv.xy + mat.uv.zw;
    vec3 base=mat.diffuse.rgb;
    vec3 normMap;
    vec3 specMap;
    if (hit.matId==1){
        // skip textures 
    }else{
        base=texture(baseTexArr,vec3(texUV, mat.tex.x)).rgb;
        normMap=texture(normalTexArr,vec3(texUV,mat.tex.y)).rgb;
        specMap=texture(specularTexArr,vec3(texUV,mat.tex.z)).rgb;
        col=0.2*base + 0.7*mat.ambient.rgb;
    }

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

        vec3 ptol=l.pos.xyz-point;
        float dist=length(ptol);
        ptol=ptol/dist;
        float nDotPtol=max(0.0,dot(n,ptol));
        vec3 diff=base*l.diffuse.rgb*nDotPtol;
        // vec3 diff=base;
        if(traceAny(point,ptol,dist)){
            continue;
        }
        vec3 ref=reflect(-ptol,n);
        float rDotV=max(0.0,dot(ref,v));
        float specStr =pow(rDotV,sh);
        vec3 spec = mat.specular.rgb*l.specular.rgb*specStr;
        if (hit.matId!=1){
            spec*=specMap;
        }
        vec3 amb = mat.ambient.rgb * l.ambient.rgb*hemi;

        col += amb+diff+spec;
    }
    return col;
}
