#include "scene_parser.h"

void SceneParser::save_scene(const std::string& fpath){
    std::vector<std::string>& res =rs_to_string();
    glsl_util::out_csv("KHORS_render_scene"+fpath+":",fpath+".khorssc",res);
}
void SceneParser::load_scene(const std::string& fpath){
    std::string f=glsl_util::read_file(fpath+".khorssc");
    string_to_rs(f);
    
}
std::vector<std::string>& SceneParser::rs_to_string(){
}
void SceneParser::string_to_rs(std::string& s){

}