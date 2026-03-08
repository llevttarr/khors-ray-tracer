#include "textures.h"

int TextureManager::load_base(std::string& filepath ){
    Image img = image_util::load_image(filepath,true);
    base_img_v.push_back(img);
    return (base_img_v.size()-1);
}
int TextureManager::load_normal(std::string& filepath ){
    Image img = image_util::load_image(filepath,true);
    normal_img_v.push_back(img);
    return (normal_img_v.size()-1);
}
int TextureManager::load_specular(std::string& filepath ){
    Image img = image_util::load_image(filepath,true);
    specular_img_v.push_back(img);
    return (specular_img_v.size()-1);
}
