#ifndef RENDERER_H
#define RENDERER_H

enum class GraphicsAPI { OpenGL, Vulkan };

class Renderer{
public:
    virtual ~Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    
    virtual bool init() = 0;
    virtual void beginf() = 0;
    virtual void endf() = 0;
};
#endif // RENDERER_H