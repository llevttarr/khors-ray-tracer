#include "renderer.h"
#include <glad/gl.h> 
Renderer::Renderer(int width, int height)

  : w(width), h(height),
    shader("assets/shaders/vs.vert", "assets/shaders/fs.frag"),
    comp_shader("assets/cs.comp")
{
    glGenBuffers(1, &tri_ssbo);
    glGenVertexArrays(1, &vao);
    glGenTextures(1, &cbuff);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w,h,0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}
Renderer::~Renderer() {
    glDeleteVertexArrays(1, &vao);
}
void Renderer::update_scene(RenderScene& render_scene){
    tric=(uint32_t)render_scene.tri_v.size();

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, tri_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER,render_scene.tri_v.size()*sizeof(RenderTri),render_scene.tri_v.data(),GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, tri_ssbo);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

}
void Renderer::run(){
    comp_shader.use();

    glBindImageTexture(0, cbuff, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    comp_shader.set_uint("width",w);
    comp_shader.set_uint("height",h);
    comp_shader.set_uint("tric",tric);

    const int dx=(w+15)/16;
    const int dy=(h+15)/16;
    glDispatchCompute(dx, dy, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT);

    shader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cbuff);

    shader.set_uint("tex",0);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}
void Renderer::resize(int nw, int nh){
    h=nh;
    w=nw;

    glBindTexture(GL_TEXTURE_2D, cbuff);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, nw,nh,0, GL_RGBA, GL_FLOAT, nullptr);

    comp_shader.set_uint("width",nw);
    comp_shader.set_uint("height",nh);
    glBindTexture(GL_TEXTURE_2D, 0);
}
