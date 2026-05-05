#ifndef RENDERER_H
#define RENDERER_H

#include "vec4.h"
#include "vec3.h"
#include "vec2.h"

enum class GraphicsAPI { OpenGL, Vulkan };

struct Reservoir{
    int y;
    float w_sum;
    int m;
    float w;
};
struct GBufferPixel{
    Vec4<float> pos;
    Vec4<float> norm;
    Vec4<float> diff;
    Vec2<float> uv;
    uint32_t matid;
    int valid;
};

class Renderer{
public:
    Renderer() = default;
    virtual ~Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    
    // virtual bool init() = 0;
    // virtual void beginf() = 0;
    // virtual void endf() = 0;
    virtual void update_scene(RenderScene& scene) = 0;
};
#endif // RENDERER_H