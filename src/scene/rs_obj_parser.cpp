#include "rs_obj_parser.h"

/* - - - - */
std::unordered_map<std::string, uint32_t> rs_obj_parser::parse_mtl_names(const std::string& mtlpath, uint32_t base){
    std::unordered_map<std::string, uint32_t> map;
    std::ifstream f(mtlpath);
    std::string line;
    uint32_t idx = base;
    while (std::getline(f, line)) {
        std::istringstream ss(line);
        std::string tok; ss >> tok;
        if (tok == "newmtl") {
            std::string name; ss >> name;
            map[name] = idx++;
        }
    }
    return map;
}
void rs_obj_parser::load_obj_into_rs(const std::string& objpath,const std::string& mtlpath,RenderScene& rs){
    uint32_t mat_base = static_cast<uint32_t>(rs.mat_v.size());
    load_mtl_into_rs(mtlpath, rs);
    auto mat_map = parse_mtl_names(mtlpath, mat_base);

    std::ifstream file(objpath,std::ios::in);
    if(!file.is_open()){
        throw SceneUtilException("Could not load obj from "+objpath);
    }
    std::string line;
    std::vector<Vec3<float>> gpos;
    std::vector<Vec3<float>> gnorm;
    std::vector<Vec2<float>> guv;

    while (std::getline(file,line)){
        std::istringstream line_s(line);
        //todo
    }
}
void rs_obj_parser::load_mtl_into_rs(const std::string& mtlpath,RenderScene& rs){
    std::ifstream file(mtlpath,std::ios::in);
    if(!file.is_open()){
        throw SceneUtilException("Could not load obj from "+mtlpath);
    }
    std::string dir;
    size_t sep = mtlpath.find_last_of("/\\");
    if (sep != std::string::npos){
        dir = mtlpath.substr(0,sep+1);
    }

    Mat curr{};
    curr.uv ={1.f,1.f,0.f,0.f};
    curr.tex={-1,-1,-1,-1};
    bool has_mat = false;
    std::string line;
    while (std::getline(file,line)){
        if (line.empty() || line[0] == '#'){
            continue;
        }
        std::istringstream ss(line);
        std::string tok;
        ss >> tok;
        if (tok == "newmtl") {
            if (has_mat){
                rs.mat_v.push_back(curr);
            }
            curr= Mat{};
            curr.uv = {1.f, 1.f, 0.f, 0.f};
            curr.tex = {-1, -1, -1, -1};
            has_mat = true;
        }
        else if (tok == "Ka") {
            ss >> curr.ambient.x >> curr.ambient.y >> curr.ambient.z;
            curr.ambient.w = 1.f;
        }
        else if (tok == "Kd") {
            ss >> curr.diffuse.x >> curr.diffuse.y >> curr.diffuse.z;
            curr.diffuse.w = 1.f;
        }
        else if (tok == "Ks") {
            ss >> curr.specular.x >> curr.specular.y >> curr.specular.z;
        }
        else if (tok == "Ke") {
            ss >> curr.emission.x >> curr.emission.y >> curr.emission.z;
            curr.emission.w = 1.f;
        }
        else if (tok == "Ns") {
            ss >> curr.specular.w;
        }
        else if (tok == "d") {

            // opacity not implemented yet, for now will store in diffuse[3]
            ss >> curr.diffuse.w;
        }
        else if (tok == "Tr") {
            float tr; ss >> tr;
            curr.diffuse.w = 1.f - tr;
        }
        else if (tok == "map_Kd") {
            std::string next; ss >> next;
            std::string texpath = dir + next;
            curr.tex.x = rs.tex_manager.load_base(texpath);
        }
        else if (tok == "map_Ks") {
            std::string next; ss >> next;
            std::string texpath = dir + next;
            curr.tex.y = rs.tex_manager.load_specular(texpath);
        }
        else if (tok == "map_bump" || tok == "bump") {
            std::string next; ss >> next;
            if (next == "-bm") {
                float bm;
                ss >> bm >> next;
            }
            std::string texpath = dir + next;
            curr.tex.z = rs.tex_manager.load_normal(texpath);
        }
    }
    if (has_mat){
        rs.mat_v.push_back(curr);
    }
}

/* - - - - */
