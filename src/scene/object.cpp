#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include "object.h"
namespace obj_util{
    Mesh create_sphere_tri(float radius, uint16_t detail_lvl){
        Mesh res{};
        double pi = std::numbers::pi;

        uint16_t st = detail_lvl * 2;
        uint16_t sl = detail_lvl * 4;

        size_t vertex_count = (sl + 1) * (st + 1);
        res.pos.reserve(vertex_count);
        res.norm_v.reserve(vertex_count);
        res.tang_v.reserve(vertex_count);
        res.bitang_v.reserve(vertex_count);
        res.uv.reserve(vertex_count);
        res.ind.reserve(sl * st * 6);

        for (size_t i = 0; i <= st; ++i) {
            float phi = pi * (float(i) / float(st));
            float y = radius * std::cos(phi);
            float sinp = std::sin(phi);
            
            float v = float(i) / float(st);
            
            for (size_t j = 0; j <= sl; ++j) {
                float theta = 2.0f * pi * (float(j) / float(sl));
                float x = radius * std::cos(theta) * sinp;
                float z = radius * std::sin(theta) * sinp;
                
                Vec3<float> pos(x, y, z);
                res.pos.push_back(pos);
                
                Vec3<float> norm = Vec3<float>::normalize(pos);
                res.norm_v.push_back(norm);
                
                float u = float(j) / float(sl);
                res.uv.push_back(Vec2<float>(u, v));
                
                Vec3<float> tangent, bitangent;
                
                float next_theta = 2.0f * pi * (float(j + 1) / float(sl));
                float next_x = radius * std::cos(next_theta) * sinp;
                float next_z = radius * std::sin(next_theta) * sinp;
                Vec3<float> next_pos(next_x, y, next_z);
                tangent = Vec3<float>::normalize((next_pos - pos));
                
                if (std::abs(sinp) < 0.0001f) {
                    tangent = Vec3<float>(1.0f, 0.0f, 0.0f);
                }
                
                bitangent = Vec3<float>::cross(norm, tangent);
                
                res.tang_v.push_back(tangent);
                res.bitang_v.push_back(bitangent);
            }
        }

        for (size_t i = 0; i < st; ++i) {
            for (size_t j = 0; j < sl; ++j) {
                uint32_t i0 = (uint32_t)(i * (sl + 1) + j);
                uint32_t i1 = (uint32_t)((i + 1) * (sl + 1) + j);
                uint32_t i2 = (uint32_t)((i + 1) * (sl + 1) + (j + 1));
                uint32_t i3 = (uint32_t)(i * (sl + 1) + (j + 1));

                res.ind.push_back(i0);
                res.ind.push_back(i1);
                res.ind.push_back(i2);
                res.ind.push_back(i0);
                res.ind.push_back(i2);
                res.ind.push_back(i3);
            }
        }

        return res;
    }
    uint32_t parse_face_ind(const std::string& t, size_t n){
        size_t s=t.find("/");
        std::string res=t;
        if (s!=std::string::npos){
            res=t.substr(0,s);
        }
        int i = std::stoi(res);
        if (i>0){
            return static_cast<uint32_t>(i-1);
        }
        return static_cast<uint32_t>(static_cast<int>(n)+i);
    }
    void gen_tbn(Mesh& mesh){
        
        for(size_t i=0; i<mesh.ind.size()-2;i+=3){
            uint32_t i0=mesh.ind[i];
            uint32_t i1=mesh.ind[i+1];
            uint32_t i2=mesh.ind[i+2];
            Vec3<float> p0=mesh.pos[i0];
            Vec3<float> p1=mesh.pos[i1];
            Vec3<float> p2=mesh.pos[i2];
            Vec2<float> uv0=mesh.uv[i0];
            Vec2<float> uv1=mesh.uv[i1];
            Vec2<float> uv2=mesh.uv[i2];
            
            Vec3<float> dp1=p1-p0;
            Vec3<float> dp2=p2-p0;
            Vec2<float> duv1=uv1-uv0;
            Vec2<float> duv2=uv2-uv0;
            float d=duv1.x*duv2.y-duv1.y*duv2.x;
            float r=0.0f;
            if(std::abs(d)>0.0001f){
                r=1.0f/d;
            }
            Vec3<float> t=(dp1*duv2.y-dp2*duv1.y)*r;
            Vec3<float> b=(dp2*duv1.x-dp1*duv2.x)*r;
            mesh.tang_v[i0]=mesh.tang_v[i0]+t;
            mesh.tang_v[i1]=mesh.tang_v[i1]+t;
            mesh.tang_v[i2]=mesh.tang_v[i2]+t;
            mesh.bitang_v[i0]=mesh.bitang_v[i0]+b;
            mesh.bitang_v[i1]=mesh.bitang_v[i1]+b;
            mesh.bitang_v[i2]=mesh.bitang_v[i2]+b;
        }
        for(size_t i=0; i<mesh.pos.size(); ++i){
            Vec3<float> n = Vec3<float>::normalize(mesh.norm_v[i]);
            Vec3<float> t = Vec3<float>::normalize(mesh.tang_v[i]-n* Vec3<float>::dot(n,mesh.tang_v[i]));
            Vec3<float> b = (Vec3<float>::dot(Vec3<float>::cross(n,t),mesh.bitang_v[i])<0.0f) ? Vec3<float>::cross(n,t)*(-1.0f) : Vec3<float>::cross(n,t);
            mesh.tang_v[i]=t;
            mesh.bitang_v[i]=b;
            mesh.norm_v[i]=n;
        }
    }
    Mesh load_mesh_obj(const std::string& file_path){
        std::ifstream file(file_path+".obj",std::ios::in);
        if(!file.is_open()){
            throw MeshLoadException("Could not load mesh from "+file_path+".obj");
        }
        // TODO: change to loading object in the future
        Mesh res{};
        std::string line;
        while (std::getline(file,line)){
            std::istringstream line_s(line);
            std::string sym;
            line_s>>sym;
            if (sym== "v"){
                // VERTEX
                Vec3<float> v;
                line_s>>v.x>>v.y>>v.z;
                res.pos.push_back(v);

            }else if (sym=="f"){
                // FACE INDICES
                std::vector<uint32_t> f;
                std::string temp;
                while(line_s>>temp){
                    uint32_t vi= parse_face_ind(temp,res.pos.size());
                    f.push_back(vi);
                }
                for(size_t i=1;i<f.size();++i){
                    res.ind.push_back(f[0]);
                    res.ind.push_back(f[i]);
                    res.ind.push_back(f[i+1]);
                }
            }else if (sym=="vn"){
                // VERT NORMAL - todo
                Vec3<float> vn;
                line_s>>vn.x>>vn.y>>vn.z;
                res.norm_v.push_back(vn);
                continue;
            }else if (sym=="vt"){
                // VERT TEXTURE - todo
                Vec2<float> vt;
                line_s>>vt.x>>vt.y;
                res.uv.push_back(vt); 
                continue;
            }
        }
        file.close();
        obj_util::gen_tbn(res);
        return res;
    }
}