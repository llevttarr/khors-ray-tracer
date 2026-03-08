#pragma once
#include <vector>
#include <string>
#include <stdexcept>
#include "stb_image.h"

struct ImageUtilException: public std::runtime_error{
    using std::runtime_error::runtime_error;
};
struct Image{
    int w;
    int h;
    std::vector<unsigned char> data;
    int channels;
};
namespace image_util{
    Image load_image(const std::string& filepath, int flipflag);
    Image load_image_stb(const std::string& filepath, int flipflag);
    Image load_image_svg(const std::string& filepath, int flipflag);
}
