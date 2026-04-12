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
    load_mtl_into_rs(mtlpath,rs);
    std::ifstream file(objpath,std::ios::in);
    if(!file.is_open()){
        throw SceneUtilException("Could not load obj from "+objpath);
    }
    std::string line;
    while (std::getline(file,line)){
        std::istringstream line_s(line);
        //todo
    }
    //obj_util::parse_face_ind(const std::string& t, size_t n);
}
void rs_obj_parser::load_mtl_into_rs(const std::string& mtlpath,RenderScene& rs){
    std::ifstream file(mtlpath,std::ios::in);
    if(!file.is_open()){
        throw SceneUtilException("Could not load obj from "+mtlpath);
    }
    std::string line;
    while (std::getline(file,line)){
        std::istringstream line_s(line);
        //todo
    }
}

/* - - - - */
