
bool intersectsGround(vec3 rayOrigin,vec3 rayVector,out float res){
    vec3 groundNormal=vec3(0.0,1.0,0.0);
    vec3 groundPoint=vec3(0.0,-4.0,0.0);
    float d=dot(groundNormal,rayVector);
    float eps=0.001;
    if (abs(d)<eps){
        return false;
    }
    res=dot(groundNormal,(groundPoint-rayOrigin))/d;
    return res>0.0;
}

bool intersects(vec3 rayOrigin,vec3 rayVector,Tri tri, out float res, out vec2 uv,out vec3 hitPos){
    float epsilon=0.000001;
    vec3 v0=tri.v0.xyz;
    vec3 v1=tri.v1.xyz;
    vec3 v2=tri.v2.xyz;
    
    vec3 edge0=v1-v0;
    vec3 edge1=v2-v0;
    vec3 rayCrossE1=cross(rayVector,edge1);
    float det=dot(edge0,rayCrossE1);
    if(det>-epsilon&&det<epsilon){
        return false;
    }
    float invDet=1.0/det;
    vec3 s=rayOrigin-v0;
    float u=invDet*dot(s,rayCrossE1);
    if ( (u<0&&abs(u)>epsilon) || (u>1 && abs(u-1)>epsilon)){
        return false;
    }
    vec3 sCrossE0=cross(s,edge0);
    float v=invDet*dot(rayVector,sCrossE0);
    if ( (v<0&&abs(v)>epsilon) || (u+v>1 && abs(u+v-1)>epsilon)){
        return false;
    }
    res=invDet*dot(edge1,sCrossE0);
    if(res>epsilon){
        hitPos=rayOrigin+res*rayVector;
        float w = 1.0-u-v;
        uv=w*tri.uv0+u*tri.uv1+v*tri.uv2;
        return true;
    }
    return false;
}
bool intersectsAABB(vec3 rayOrigin,vec3 invRayVec,vec3 minv,vec3 maxv,float tMax){
    vec3 t0=(minv-rayOrigin)*invRayVec;
    vec3 t1=(maxv-rayOrigin)*invRayVec;
    vec3 tmin=min(t0,t1);
    vec3 tmax=max(t0,t1);
    float lo = max(max(tmin.x,tmin.y),max(tmin.z,0.0));
    float hi = min(min(tmax.x,tmax.y),min(tmax.z,tMax));
    return hi >= lo;
}
bool intersectsSphere(vec3 rayOrigin,vec3 rayVector,Sphr sphere,out float res,out vec2 uv,out vec3 hitPos){
    float epsilon=0.000001;
    vec3 cv=sphere.c_r.xyz;
    float r=sphere.c_r.w;
    vec3 L=cv-rayOrigin;
    float tca=dot(L,rayVector);
    if(tca<-epsilon){
        return false;
    }
    float sc=dot(L,L);
    float d2=sc-tca*tca;
    if(d2>r*r){
        return false;
    }
    float thc=sqrt(r*r-d2);
    float t0=tca-thc;
    float t1=tca+thc;
    float t;
    if (t0>epsilon){
        t=t0;
    }else if (t1>epsilon){
        t=t1;
    }else{
        t=-1.0;
    }
    if (t<-epsilon){
        return false;
    }
    res=t;
    hitPos=rayOrigin+t*rayVector;
    vec3 p = normalize(hitPos-sphere.c_r.xyz);
    float u = 0.5+atan(p.z, p.x)/(2.0*3.14159);
    float v = 0.5-asin(p.y)/3.14159;
    uv=vec2(u,v);
    return true;
}
bool traceAny(vec3 rayOrigin,vec3 rayVector,float tMax){
    vec3 invR=1.0/rayVector;

    uint stp=1;
    uint stack[64];
    stack[0]=0;
    float eps=0.0001;
    while(stp>0){
        uint i=stack[stp-1];
        --stp;
        BVH bvh=bvh_v[i];
        if (!intersectsAABB(rayOrigin,invR,bvh.mindat.xyz,bvh.maxdat.xyz,tMax)){
            continue;
        }
        bool isLeafNode=isLeaf(bvh);
        uint datmin= floatBitsToUint(bvh.mindat.w);
        uint datmax= floatBitsToUint(bvh.maxdat.w);
        if(!isLeafNode){
            stack[stp]=datmin;
            ++stp;
            stack[stp]=datmax;
            ++stp;
            continue;
        }
        uint lN=leafN(bvh);
        for (uint j=0;j<lN;++j){
            uint primDat=prims[datmin+j];
            uint type=primType(primDat);
            uint id=primId(primDat);
            vec3 hitPos;
            vec2 uv;
            if (type==0){
                Tri tri=tris[id];
                float t;
                if(intersects(rayOrigin,rayVector,tri,t,uv,hitPos)){
                    if (t>eps&&t<tMax){
                        return true;
                    }
                }
            }else if (type==1){
                Sphr sp=spheres[id];
                float t;
                if (intersectsSphere(rayOrigin,rayVector,sp,t,uv,hitPos)){
                    if (t>eps&&t<tMax){
                        return true;
                    }
                }
            }
        }
    }
    float tg;
    if (intersectsGround(rayOrigin,rayVector,tg)){
        if (tg>eps){
            return true;
        }
    }
    return false;
}

RayHit trace(vec3 rayOrigin,vec3 rayVector){
    RayHit res;
    res.isValid=false;
    res.t=1e30;
    vec3 invR= 1.0/rayVector;
    
    uint stp=1;
    uint stack[64];
    stack[0]=0;
    while(stp>0){
        uint i=stack[stp-1];
        --stp;
        BVH bvh=bvh_v[i];
        float tMax=1e30;
        if (res.isValid){
            tMax=res.t;
        }
        if (!intersectsAABB(rayOrigin,invR,bvh.mindat.xyz,bvh.maxdat.xyz,tMax)){
            continue;
        }
        bool isLeafNode=isLeaf(bvh);
        uint datmin= floatBitsToUint(bvh.mindat.w);
        uint datmax= floatBitsToUint(bvh.maxdat.w);
        if(!isLeafNode){
            stack[stp]=datmin;
            ++stp;
            stack[stp]=datmax;
            ++stp;
            continue;
        }
        uint lN=leafN(bvh);
        for (uint j=0;j<lN;++j){
            uint primDat=prims[datmin+j];
            uint type=primType(primDat);
            uint id=primId(primDat);
            vec2 uv;
            vec3 hitPos;
            if (type==0){
                Tri tri=tris[id];
                float t;
                if(intersects(rayOrigin,rayVector,tri,t,uv,hitPos)){
                    if (!res.isValid){
                        res.isValid=true;
                    }
                    else if (res.t<t){
                        continue;
                    }
                    res.t=t;
                    res.matId=tri.matId;
                    res.hitPos=hitPos;
                    res.uv=uv;

                    // vec3 v0=tri.v0.xyz;
                    // vec3 v1=tri.v1.xyz;
                    // vec3 v2=tri.v2.xyz;
                    // res.n=normalize(cross(v1-v0,v2-v0));
                    res.n=tri.n.xyz;
                    res.tbn=mat3(tri.t.xyz,tri.b.xyz,tri.n.xyz);
                    res.type=0;
                }
            }else if (type==1){
                Sphr sp=spheres[id];
                float t;
                if (intersectsSphere(rayOrigin,rayVector,sp,t,uv,hitPos)){
                    if (!res.isValid){
                        res.isValid=true;
                    }
                    else if (res.t<t){
                        continue;
                    }
                    res.t=t;
                    res.matId=sp.matId;
                    res.hitPos=hitPos;
                    res.uv=uv;

                    vec3 cr=sp.c_r.xyz;
                    vec3 p = rayOrigin+t*rayVector;
                    res.n = normalize(p-cr);
                    res.type=1;
                }
            }
        }
    }
    float tg;
    if (intersectsGround(rayOrigin,rayVector,tg)){
        if (!res.isValid){
            res.isValid=true;
        }
        else if (res.t<tg){
            return res;
        }
        res.t=tg;
        res.matId=1;
        res.n=vec3(0.0,1.0,0.0);
        res.hitPos = rayOrigin+tg*rayVector;
        res.uv = res.hitPos.xz*0.25;
    }
    return res;

}
