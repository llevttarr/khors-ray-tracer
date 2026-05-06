#include "scene.h"
#include <bit>
#include <iostream>
#include <fstream>
#include <sstream>
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
    
    std::cout<<"to rs..."<<std::endl;
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
            Vec4<float> v0=Vec4<float>::matvec_mul(t,Vec4<float>::from_v3(p0));
            Vec4<float> v1=Vec4<float>::matvec_mul(t,Vec4<float>::from_v3(p0));
            Vec4<float> v2=Vec4<float>::matvec_mul(t,Vec4<float>::from_v3(p0));
            Vec2<float> uv0=mesh.uv[i0];
            Vec2<float> uv1=mesh.uv[i1];
            Vec2<float> uv2=mesh.uv[i2];
            //cross(v1-v0 v2-v0)
            Vec3<float> v1mv0{v1.x-v0.x,v1.y-v0.y,v1.z-v0.z};
            Vec3<float> v2mv0{v2.x-v0.x,v2.y-v0.y,v2.z-v0.z};
            Vec3<float> n_v3=Vec3<float>::cross(v1mv0,v2mv0);
            Vec4<float> n =Vec4<float>::from_v3(Vec3<float>::normalize(n_v3));
            
            Vec2<float> duv0=uv1-uv0;
            Vec2<float> duv1=uv2-uv0;
            float det=duv0.x*duv1.y-duv1.x*duv0.y;
            float f=1.0/det; // avoid div by zero ?
            Vec3<float> t_inn=v1mv0*duv1.y+v2mv0*(-duv0.y);
            Vec3<float> b_inn=v1mv0*(-duv1.x)+v2mv0*duv0.x;
            Vec4<float> t=Vec4<float>::from_v3(t_inn*f);
            Vec4<float> b=Vec4<float>::from_v3(b_inn*f);
            RenderTri r{v0,v1,v2,n,t,b,uv0,uv1,uv2,obj.matid};
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
    std::cout<<"building bvh..."<<std::endl;
    scene_util::build_bvh(prims,0,prims.size(),res.prim_v,res.bvh_v);
    res.mat_v=mat_v;
    res.light_v=light_v;
    res.tex_manager=tex_manager;
    return res;
}

Scene::Scene(){
    test_scene_init();
}

Scene::~Scene(){

}
uint32_t Scene::add_mat(Mat m){
    mat_v.push_back(std::move(m));
    return (uint32_t)(mat_v.size()-1);
}
uint32_t Scene::add_light(Light l){
    light_v.push_back(std::move(l));
    return (uint32_t)(light_v.size()-1);
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
void Scene::gen_random_mats(size_t n,int basei,int normali,int speculari){
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 engine(seed);
    std::uniform_real_distribution<float> light_diff(0.1f,0.7f);
    std::uniform_real_distribution<float> light_diff_small(0.02f,0.15f);
    mat_v.clear();
    for (size_t i=0;i<n;++i){
        Vec4<float> amb=scene_util::rand_vec(engine,light_diff_small);
        // Vec4<float> diff={amb.x+0.01f,amb.y+0.01f,amb.z+0.01f,amb.w+0.01f};
        Vec4<float> diff=scene_util::rand_vec(engine,light_diff);
        Vec4<float> spec=scene_util::rand_vec(engine,light_diff);
        if (i==0){
            spec.w=0.0f;
        }else{
            spec.w=0.0f;
        }
        // Vec4<float> diff{0.7f,0.5f,0.8f,1.0f};
        // Vec4<float> spec{0.7f,0.5f,0.8f,0.2f};
        Vec4<float> emis{0.0f,0.0f,0.0f,0.0f};
        Vec4<int32_t> tex={i!=0?basei:-1,i!=0?normali:-1,i!=0?speculari:-1,1};
        Vec4<float> uv={1.0,1.0,0.0,0.0};
        Mat m{amb,diff,spec,emis,uv,tex};
        mat_v.push_back(m);
    }
}

void Scene::test_scene_init(){
    Mesh tri_sphere_mesh=obj_util::create_sphere_tri(3,4);

    uint32_t tri_sphere_id = add_mesh(std::move(tri_sphere_mesh));
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 engine(seed);
    std::uniform_real_distribution<float> dist(-50.0, 50.0);
    std::uniform_int_distribution<uint32_t> mat_dist(2,5);
    
    std::vector<uint32_t> objs{};
    objs.reserve(11);
    Mat4<float> identity{};
    Mat4<float> temp{};
    float r0;
    float r1;
    float r2;
    uint32_t obj;
    for (size_t i=0;i<8;++i){
        r0=dist(engine);
        r1=6.0+dist(engine)/10.0;
        r2=dist(engine);
        temp=identity.translate(r0,r1,r2);
        uint32_t matid=1;
        obj=add_object(Object{tri_sphere_id,temp,matid});
    }
    Sphr sp=Sphr{};
    sp.cx=100.0;
    sp.cy=100.0;
    sp.cz=100.0;
    sp.r=1.25f;
    sp.matid=mat_dist(engine);
    add_sphere(sp);
    sphere_v.reserve(100);

    int n_spheres = 1028;
    float eps=0.001f;
    int spheres_per_side = static_cast<int>(std::ceil(std::sqrt(n_spheres)));

    // for(size_t i=0;i<spheres_per_side;++i){
    //     for (size_t j=0;j<spheres_per_side;++j){
    //         if (i ==spheres_per_side/2&& j==spheres_per_side/2){
    //             continue;
    //         }
    //         sp=Sphr{};
    //         sp.cx=4.0*(i-spheres_per_side/2.0);
    //         sp.cy=-2.5f;
    //         sp.cz=4.0*(j-spheres_per_side/2.0);
    //         sp.r=1.25f;
    //         sp.matid=mat_dist(engine);
    //         add_sphere(sp);
    //     }
    // }
    

    // for (size_t i=0;i<128;++i){
    //     sp=Sphr{};
    //     sp.cx=dist(engine)*2.0;
    //     sp.cy=8.0-dist(engine)/4.0;
    //     sp.cz=dist(engine)*2.0;
    //     sp.r=3.5;
    //     sp.matid=mat_dist(engine);
    //     add_sphere(sp);

    // }
    std::uniform_real_distribution<float> light_diff(0.1f,0.7f);

    // for (size_t i=0;i<5;++i){
    //     Vec4<float> pos=scene_util::rand_vec(engine,dist);
    //     pos.w=4;
    //     Vec4<float> amb=scene_util::rand_vec(engine,light_diff);
    //     Vec4<float> diff=scene_util::rand_vec(engine,light_diff);
    //     Vec4<float> spec=scene_util::rand_vec(engine,light_diff);
    //     Light l{pos,amb,diff,spec};
    //     light_v.push_back(l);
    // }
    Light dirl{};
    dirl.pos = {0.0f, 0.0f, 0.0f, 0.0f};
    dirl.diffuse = {0.7f, 0.7f, 0.7f, 0.0f};
    Vec3<float> dir = Vec3<float>::normalize(Vec3<float>{-0.5f, -1.0f, -0.3f});
    light_util::set_dir(dirl, dir);
    light_util::set_type(dirl, LIGHT_DIRECTION);
    dirl.params1 = {0.0f, 0.0f, 0.0f, 0.0f};
    light_v.push_back(dirl);

    Light areal{};
    areal.pos = {0.0f, 5.0f, 0.0f, 0.0f};
    areal.diffuse = {3.0f,3.0f,3.0f, 0.0f};
    Vec3<float> area_dir = Vec3<float>::normalize(Vec3<float>{0.0f, -1.0f, 0.0f});
    light_util::set_dir(areal, area_dir);
    light_util::set_type(areal, LIGHT_AREA);
    Vec3<float> tangent = Vec3<float>::normalize(Vec3<float>{1.0f, 0.0f, 0.0f});
    Vec3<float> bitangent = Vec3<float>::normalize(Vec3<float>{0.0f, 0.0f, 1.0f});
    areal.tangent = {tangent.x, tangent.y, tangent.z, 0.0f};
    areal.bitangent = {bitangent.x, bitangent.y, bitangent.z, 0.0f};
    areal.params1 = {30.0f, 0.0f, 1.5f, 1.0f}; 

    light_v.push_back(areal);
    TextureManager texman;
    std::uniform_real_distribution<float> light_diff_small(0.02f,0.15f);
    Vec4<float> gamb=Vec4<float>{0.4f,0.6f,0.6f,0.1f};
    Vec4<float> gdiff=Vec4<float>{0.9f,0.9f,0.9f,0.9f};
    Vec4<float> gemis{0.0f,0.0f,0.0f,0.0f};
    Mat mg{gamb,gdiff,gdiff,gemis};

    std::string base_path="assets/textures/test1_base.png";
    std::string normal_path="assets/textures/test1_normal.png";
    std::string specular_path="assets/textures/test1_specular.png";
    int basei=texman.load_base(base_path);
    int normali=texman.load_normal(normal_path);
    int speculari=texman.load_specular(specular_path);
    Scene::gen_random_mats(5,0,0,0);
    tex_manager=texman;
}
void Scene::load_obj(const std::string& fpath){
    load_obj_mtl(fpath);
    Mesh m= obj_util::load_mesh_obj(fpath+".obj");
    // unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    // std::mt19937 engine(seed);
    // std::uniform_real_distribution<float> dist(-50.0, 50.0);
    // std::uniform_int_distribution<uint32_t> mat_dist(2,5);
    // std::uniform_real_distribution<float> light_diff(0.1f,0.7f);
    // std::uniform_real_distribution<float> light_diff_small(0.05f,0.3f);
    uint32_t mid=add_mesh(m);

    // Vec4<float> amb=scene_util::rand_vec(engine,light_diff_small);
    // Vec4<float> diff=scene_util::rand_vec(engine,light_diff);
    // Vec4<float> spec=scene_util::rand_vec(engine,light_diff);
    // Vec4<float> emis{0.0f,0.0f,0.0f,0.0f};
    // int i=mat_v.size();
    // Vec4<int32_t> tex={i,i,i,1};
    // Vec4<float> uv={1.0,1.0,0.0,0.0};
    // Mat mat{amb,diff,spec,emis,uv,tex};

    Mat4<float> identity{};

    // uint32_t matid=add_mat(mat);
    Object o={mid,identity,1};
    uint32_t objid=add_object(o);
}
void Scene::load_obj_mtl(const std::string& fpath){
    std::ifstream file(fpath+".mtl",std::ios::in);
    if(!file.is_open()){
        throw MeshLoadException("Could not load mesh from "+fpath+".obj");
    }
    std::string line;
    while (std::getline(file,line)){
        std::istringstream line_s(line);
        std::string sym;
        line_s>>sym;
        if (sym== "newmtl"){ //new mat

        }else if (sym=="Ka"){ // ambient

        }else if (sym=="Kd"){ // diffuse

        }else if (sym=="Ks"){ // specular

        }else if (sym=="Ns"){ // diffuse[3]

        }else if (sym=="d"){ //dissolve - skip(?)

        }else if (sym=="map_Kd"){// text base

        }else if (sym=="map_Ks"){//text spec

        }else if (sym=="map_bump"||sym=="bump"){ //text norm

        }
    }
    file.close();
}
void Scene::change_mat(Mat& m, uint32_t matid){
    mat_v[matid]=m;
}
void Scene::change_light(Light& l, uint32_t lightid){
    light_v[lightid]=l;
}
