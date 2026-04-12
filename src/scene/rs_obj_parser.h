#ifndef RS_OBJ_PARSER_H
#define RS_OBJ_PARSER_H
#include <unordered_map> 
#include <string> 
#include <iostream> 
#include <sstream> 
#include <fstream> 
#include <scene.h> 
struct LocalMesh{
    std::vector<Vec3<float>> pos;
    std::vector<Vec3<float>> norm_v;
    std::vector<Vec3<float>> tang_v;
    std::vector<Vec3<float>> bitang_v;
    std::vector<Vec2<float>> uv;
    std::vector<uint32_t> ind;
    uint32_t mat_id = 0;
};
namespace rs_obj_parser{
    void load_obj_into_rs(const std::string& objpath,const std::string& mtlpath,RenderScene& rs);
    void load_mtl_into_rs(const std::string& mtlpath,RenderScene& rs);
    std::unordered_map<std::string, uint32_t> parse_mtl_names(const std::string& mtlpath, uint32_t base);
};

#endif