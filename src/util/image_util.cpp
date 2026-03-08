#include "image_util.h"
#include "iostream"
#include "fstream"

Image image_util::load_image(const std::string& filepath, int flipflag){
    size_t path_size=filepath.length();
    if (path_size<5){
        throw ImageUtilException("Invalid filepath");

    }
    std::string file_ext=filepath.substr(path_size-4,4);
    if (file_ext==".svg"){
        // return image_util::load_image_svg(filepath);
    }
    return image_util::load_image_stb(filepath,flipflag);
}
Image image_util::load_image_stb(const std::string& filepath, int flipflag){
    int width;
    int height;
    int chan=4;
    // stbi_set_flip_vertically_on_load(flipflag);
    const char* filename=filepath.c_str();
    unsigned char* imgdat = stbi_load(filename,&width,&height,&chan,STBI_rgb_alpha);
    if (imgdat == nullptr) {
        throw ImageUtilException("Invalid filepath");
    }
    size_t datsize=width*height*chan;
    std::vector<unsigned char> res(imgdat,imgdat+datsize);
    stbi_image_free(imgdat);
    return Image{width,height,res,chan};
}
