#ifndef RENDERER_H
#define RENDERER_H

#include <cstdint>
#include "shader.h"
#include "comp_shader.h"
#include "../scene/scene.h"

class Renderer {
public:
    explicit Renderer(int width, int height);
    ~Renderer();
    void run();
    void resize(int nw, int nh);
    void update_scene(RenderScene& render_scene);
private:
    int w;
    int h;
    Shader shader;
    ComputeShader comp_shader;

    GLuint vao = 0;
    GLuint cbuff = 0;
    GLuint tri_ssbo = 0;
    GLuint bvh_ssbo = 0;
    uint32_t tric = 0;
    uint32_t framec=0;
    // GLuint vbo = 0;
};
#endif //RENDERER_H
