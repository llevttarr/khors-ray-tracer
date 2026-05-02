#ifndef CONFIG_H
#define CONFIG_H
#include <string>
#include <vector>
#include <cstdint>

struct Config{
    // - GENERAL -
    int h=720;
    int w=1080;
    bool using_vsync=true;
    bool debug_mode = true;
    // - TRACING - 
    uint8_t tracing_type=0; // 0 - direct illumination, 1 - reservoir sampling
    uint8_t local_size_x=16; // *note: warp - 32px; wavefront - 64/32px
    uint8_t local_size_y=4;
    uint8_t M=32; // reservoir sampling only
    // - SCENE -
    bool auto_scene_load =false;
    std::string auto_scene_path= "demo_scene";
    uint8_t bvh_leaf_size=4;
};

class ConfigParser{
    static Config parse(std::istream& conf_file);
};
#endif
