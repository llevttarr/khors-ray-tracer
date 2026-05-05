#ifndef RENDERER_H
#define RENDERER_H

enum class GraphicsAPI { OpenGL, Vulkan };

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