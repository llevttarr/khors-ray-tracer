#pragma once
#include <vector>
#include <string>

struct Image{
    int w;
    int h;
    std::vector<unsigned char> data;
    int channels;
};
namespace image_util{
    Image load_image(const std::string& filepath);
}
