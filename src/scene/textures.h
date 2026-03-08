#pragma once

#include "../util/image_util.h"
#include <vector>
#include <string>

class TextureManager{
public:
    // TextureManager();
    int load_base(std::string& filepath);
    int load_normal(std::string& filepath);
    int load_specular(std::string& filepath);

    std::vector<Image>& get_base(){return base_img_v;}
    std::vector<Image>& get_normal(){return normal_img_v;}
    std::vector<Image>& get_specular(){return specular_img_v;} 
private:
    std::vector<Image> base_img_v;
    std::vector<Image> specular_img_v;
    std::vector<Image> normal_img_v;
};
