#include "scene.h"
#include <bit>
#include <iostream>
#include <algorithm>
const uint BVH_LEAF_SIZE=4;
constexpr uint32_t IS_LEAF_FLAG=0x80000000u;

Vec4<float> scene_util::rand_vec(std::mt19937& engine, std::uniform_real_distribution<float>& dist){
    float x=dist(engine);
    float y=dist(engine);
    float  z=dist(engine);
    float w=dist(engine);
    return {x,y,z,w};
}
uint32_t scene_util::get_type_id(uint32_t type, uint32_t id) {
    return (type << 28)|(id & 0x0FFFFFFF);
}
AABB scene_util::tri_to_aabb(const RenderTri& t){
    Vec3<float> mn{std::min({t.v0.x,t.v1.x,t.v2.x}),std::min({t.v0.y,t.v1.y,t.v2.y}),std::min({t.v0.z,t.v1.z,t.v2.z})};
    Vec3<float> mx{std::max({t.v0.x,t.v1.x,t.v2.x}),std::max({t.v0.y,t.v1.y,t.v2.y}),std::max({t.v0.z,t.v1.z,t.v2.z})};
    return {mn,mx};
}
AABB scene_util::sphr_to_aabb(const Sphr& s){
    Vec3<float> mn{s.cx-s.r,s.cy-s.r,s.cz-s.r};
    Vec3<float> mx{s.cx+s.r,s.cy+s.r,s.cz+s.r};
    return {mn,mx};
}
AABB scene_util::merge_aabb(const AABB& a, const AABB& b){
    return{
        Vec3<float>{std::min(a.minv.x,b.minv.x),std::min(a.minv.y,b.minv.y), std::min(a.minv.z,b.minv.z)},
        Vec3<float>{std::max(a.maxv.x,b.maxv.x), std::max(a.maxv.y,b.maxv.y), std::max(a.maxv.z,b.maxv.z)}
    };
}
Vec3<float> scene_util::get_c_aabb(const AABB& a){
    return (a.minv+a.maxv)*0.5f;
}
AABB scene_util::prims_vec_aabb(std::vector<Prim>& prims,size_t l,size_t r){
    AABB a =prims[l].aabb;
    for (size_t i=l+1;i<r;++i){
        a=merge_aabb(a,prims[i].aabb);
    }
    return a;
}
AABB scene_util::c_prims_aabb(std::vector<Prim>& prims,size_t l,size_t r){
    AABB a{prims[l].c,prims[l].c};
    for(size_t i=l+1;i<r;++i){
        Vec3<float> c=prims[i].c;
        a.minv.x=std::min(a.minv.x,c.x);
        a.minv.y=std::min(a.minv.y,c.y);
        a.minv.z=std::min(a.minv.z,c.z);
        a.maxv.x=std::max(a.maxv.x,c.x);
        a.maxv.y=std::max(a.maxv.y,c.y);
        a.maxv.z=std::max(a.maxv.z,c.z);
    }
    return a;
}
uint16_t scene_util::max_axis(const AABB& a){
    Vec3<float> r=a.maxv-a.minv;
    if(r.x>=r.y && r.x>=r.z){
        return 0;
    }
    if (r.y>=r.x && r.y>=r.z){
        return 1;
    }
    return 2;
}

uint32_t scene_util::build_bvh(std::vector<Prim>& prims,size_t l,size_t r, 
   /* output */ std::vector<uint32_t>& prim_v,std::vector<BVH>& bvh_v){
    AABB p_aabb = prims_vec_aabb(prims,l,r);
    uint32_t curr_i=bvh_v.size();
    bvh_v.push_back(BVH{});
    size_t n=r-l;
    if(n<=BVH_LEAF_SIZE){
        uint32_t first=prim_v.size();
        for (size_t i=l;i<r;++i){
            prim_v.push_back(prims[i].dat);
        }
        Vec4<float> mndt{p_aabb.minv.x,p_aabb.minv.y,p_aabb.minv.z,std::bit_cast<float>(first)};
        Vec4<float> mxdt{p_aabb.maxv.x,p_aabb.maxv.y,p_aabb.maxv.z,std::bit_cast<float>((uint32_t)n|IS_LEAF_FLAG)};
        BVH bvh{mndt,mxdt};
        bvh_v[curr_i]=bvh;
        return curr_i;
    }
    AABB c_aabb=c_prims_aabb(prims,l,r);
    int maxis=max_axis(c_aabb);
    size_t mid=(r+l)/2;


    auto cmp_axis=[maxis](const Prim& a,const Prim& b){
        if(maxis==0)/*x*/{
            return a.c.x<b.c.x;
        }else if(maxis==1){
            return a.c.y<b.c.y;   
        }
        return a.c.z<b.c.z;
    };
    std::nth_element(prims.begin()+l,prims.begin()+mid,prims.begin()+r,cmp_axis);
    uint32_t l_tree=build_bvh(prims,l,mid,prim_v,bvh_v);
    uint32_t r_tree=build_bvh(prims,mid,r,prim_v,bvh_v);
    Vec4<float> mndt{p_aabb.minv.x,p_aabb.minv.y,p_aabb.minv.z,std::bit_cast<float>(l_tree)};
    Vec4<float> mxdt{p_aabb.maxv.x,p_aabb.maxv.y,p_aabb.maxv.z,std::bit_cast<float>(r_tree)};
    BVH bvh{mndt,mxdt};
    bvh_v[curr_i]=bvh;
    return curr_i;
}

RenderScene Scene::to_render_scene() const{
    RenderScene res;
    res.tri_v.reserve(1000);
    std::vector<Prim> prims;
    prims.reserve(res.tri_v.size()+sphere_v.size());
    
    for (const Object& obj : obj_v){
        const Mesh& mesh=mesh_v[obj.mesh_id];
        Mat4<float> t= obj.transform;
        for(size_t i=0;i<mesh.ind.size()-2;i+=3){
            uint32_t i0=mesh.ind[i];
            uint32_t i1=mesh.ind[i+1];
            uint32_t i2=mesh.ind[i+2];
            Vec3<float> p0=mesh.pos[i0];
            Vec3<float> p1=mesh.pos[i1];
            Vec3<float> p2=mesh.pos[i2];
            Vec4<float> v0=Vec4<float>::matvec_mul(t,Vec4<float>(p0.x,p0.y,p0.z,1.0));
            Vec4<float> v1=Vec4<float>::matvec_mul(t,Vec4<float>(p1.x,p1.y,p1.z,1.0));
            Vec4<float> v2=Vec4<float>::matvec_mul(t,Vec4<float>(p2.x,p2.y,p2.z,1.0));
            RenderTri r{v0,v1,v2,obj.matid};
            res.tri_v.push_back(r);
        }
    }
    res.sphr_v=sphere_v;

    for (size_t i=0;i<res.tri_v.size();++i){
        AABB a=scene_util::tri_to_aabb(res.tri_v[i]);
        prims.push_back(Prim{a,scene_util::get_c_aabb(a),scene_util::get_type_id(0,i)});
    }
    for (size_t i=0;i<res.sphr_v.size();++i){
        AABB a=scene_util::sphr_to_aabb(res.sphr_v[i]);
        prims.push_back(Prim{a,scene_util::get_c_aabb(a),scene_util::get_type_id(1,i)});
    }
    res.bvh_v.reserve(999);
    res.prim_v.reserve(999);
    // std::cout<<"building bvh...";
    scene_util::build_bvh(prims,0,prims.size(),res.prim_v,res.bvh_v);
    res.mat_v=mat_v;
    res.light_v=light_v; 
    return res;
}

Scene::Scene(){
    test_scene_init();
}

Scene::~Scene(){

}
uint32_t Scene::add_mesh(Mesh m){
    mesh_v.push_back(std::move(m));
    return (uint32_t)(mesh_v.size()-1);
}
uint32_t Scene::add_object(Object o){
    obj_v.push_back(std::move(o));
    return (uint32_t)(obj_v.size()-1);
}
uint32_t Scene::add_sphere(Sphr s){
    sphere_v.push_back(std::move(s));
    return (uint32_t)(sphere_v.size()-1);
}

void Scene::test_scene_init(){
    Mesh tri_sphere_mesh=obj_util::create_sphere_tri(3,4);

    uint32_t tri_sphere_id = add_mesh(std::move(tri_sphere_mesh));
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 engine(seed);
    std::uniform_real_distribution<float> dist(-50.0, 7.0);
    std::uniform_int_distribution<uint32_t> mat_dist(1,5);
    
    std::vector<uint32_t> objs{};
    objs.reserve(11);
    Mat4<float> identity{};
    Mat4<float> temp{};
    float r0;
    float r1;
    float r2;
    uint32_t obj;
    Sphr sp;
    sphere_v.reserve(100);

    for(float i=-100.0f;i<100.1f;i+=5){
        for (float j=-100.0f;j<100.1f;j+=5){
            if (i>=-0.1f && i<=0.1f && j>=-0.1f && j<=0.1f){
                continue;
            }
            sp=Sphr{};
            sp.cx=i;
            sp.cy=-2.5f;
            sp.cz=j;
            sp.r=1.25f;
            sp.matid=mat_dist(engine);
            add_sphere(sp);
        }
    }

    // for (size_t i=0;i<32;++i){
    //     sp=Sphr{};
    //     sp.cx=dist(engine);
    //     sp.cy=dist(engine)/5.0;
    //     sp.cz=dist(engine);
    //     sp.r=2.5;
    //     sp.matid=mat_dist(engine);
    //     add_sphere(sp);

    // }
    std::uniform_real_distribution<float> light_diff(0.1f,0.7f);

    // for (size_t i=0;i<2;++i){
    //     Vec4<float> pos=scene_util::rand_vec(engine,dist);
    //     pos.w=4;
    //     Vec4<float> amb=scene_util::rand_vec(engine,light_diff);
    //     Vec4<float> diff=scene_util::rand_vec(engine,light_diff);
    //     Vec4<float> spec=scene_util::rand_vec(engine,light_diff);
    //     Light l{pos,amb,diff,spec};
    //     light_v.push_back(l);
    // }
    Light l;
    l.pos = {0.2f, 0.f,0.2f,200.f};
    l.ambient = {0.02f,0.02f,0.02f,1.f};
    l.diffuse = {0.7f,0.7f,0.7f,0.6f};
    l.specular = {0.8f,0.8f,0.7f,0.7f};
    light_v.push_back(l);
    std::uniform_real_distribution<float> light_diff_small(0.02f,0.15f);
    for (size_t i=0;i<5;++i){
        Vec4<float> amb=scene_util::rand_vec(engine,light_diff_small);
        // Vec4<float> diff={amb.x+0.01f,amb.y+0.01f,amb.z+0.01f,amb.w+0.01f};
        Vec4<float> diff=scene_util::rand_vec(engine,light_diff);
        Vec4<float> spec=scene_util::rand_vec(engine,light_diff);
        // Vec4<float> diff{0.7f,0.5f,0.8f,1.0f};
        // Vec4<float> spec{0.7f,0.5f,0.8f,0.2f};
        Vec4<float> emis{0.0f,0.0f,0.0f,0.0f};
        Mat m{amb,diff,spec,emis};
        mat_v.push_back(m);
    }
}
