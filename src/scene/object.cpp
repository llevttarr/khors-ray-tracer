#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include "object.h"
namespace obj_util{
    Mesh create_sphere_tri(float radius,uint16_t detail_lvl){
        Mesh res{};
        double pi=std::numbers::pi;

        uint16_t st=detail_lvl*2;
        uint16_t sl=detail_lvl*4;

        res.pos.reserve((sl+1)*(st+1));
        for(size_t i=0;i<=st;++i){
            float phi=pi* (float(i)/float(st));
            float y=radius*std::cos(phi);
            float sinp=std::sin(phi);
            for(size_t j=0;j<=sl;++j){
                float theta=pi*2.0f*(float(j)/float(sl));
                float x=radius*std::cos(theta)*sinp;
                float z=radius*std::sin(theta)*sinp;
                res.pos.push_back(Vec3<float>(x,y,z));
            }
        }
        res.ind.reserve(sl*st*6);

        for (size_t i=0;i<st;++i){
            for(size_t j=0;j<sl;++j){
                uint32_t i0=i*(sl+1)+j;
                uint32_t i1=(i+1)*(sl+1)+j;
                uint32_t i2=(i+1)*(sl+1)+(j+1);
                uint32_t i3=i*(sl+1)+(j+1);

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
    static uint32_t parse_face_ind(const std::string& t, size_t n){
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
    Mesh load_mesh_obj(const std::string& file_path){
        std::ifstream file(file_path,std::ios::in);
        if(!file.is_open()){
            throw MeshLoadException("Could not load mesh from "+file_path);
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
                continue;
            }else if (sym=="vt"){
                // VERT TEXTURE - todo
                continue;
            }
        }
        file.close();
        return res;
    }
}