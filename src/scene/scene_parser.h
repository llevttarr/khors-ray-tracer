#ifndef SCENE_PARSER_H
#define SCENE_PARSER_H

#include <string>
#include <iostream>
#include <fstream>
#include "scene.h"
#include "../util/glsl_util.h"

class SceneParser{
public:
    SceneParser(RenderScene& rs): render_scene(rs) {}
    void save_scene(const std::string& fpath);
    void load_scene(const std::string& fpath);
private:
    RenderScene& render_scene;
    std::vector<std::string>& rs_to_string();
    void string_to_rs(std::string& s);
};

#endif