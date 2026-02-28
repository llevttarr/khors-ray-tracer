#include "renderer.h"
#include <glad/gl.h>
#include <iostream>
#include <string>
Renderer::Renderer(int width, int height,EulerCamera& cam)

  : w(width), h(height),camera(cam),
    shader("assets/shaders/vs.vert", "assets/shaders/fs.frag"),
    comp_shader("assets/shaders/cs.comp")
{
    glGenBuffers(1, &tri_ssbo);
    glGenBuffers(1, &sphr_ssbo);
    glGenBuffers(1, &bvh_ssbo);
    glGenBuffers(1, &mat_ssbo);
    glGenBuffers(1, &prim_ssbo);
    glGenBuffers(1, &light_ssbo);
    glGenVertexArrays(1, &vao);
    glGenTextures(1, &cbuff);
    glBindTexture(GL_TEXTURE_2D, cbuff);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w,h,0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}
Renderer::~Renderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1,&tri_ssbo);
    glDeleteBuffers(1,&sphr_ssbo);
    glDeleteBuffers(1,&bvh_ssbo);
    glDeleteBuffers(1,&mat_ssbo);
    glDeleteBuffers(1,&prim_ssbo);
    glDeleteBuffers(1,&light_ssbo);
    glDeleteTextures(1,&cbuff);
}
void Renderer::update_scene(RenderScene& render_scene){
    tric=(uint32_t)render_scene.tri_v.size();
    spherec=(uint32_t)render_scene.sphr_v.size();
    bvhc=(uint32_t)render_scene.bvh_v.size();
    matc=(uint32_t)render_scene.mat_v.size();
    lightc=(uint32_t)render_scene.light_v.size();
    // primc=(uint32_t)render_scene.prim_v.size();

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, tri_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER,render_scene.tri_v.size()*sizeof(RenderTri),render_scene.tri_v.data(),GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, tri_ssbo);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphr_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER,render_scene.sphr_v.size()*sizeof(Sphr),render_scene.sphr_v.data(),GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sphr_ssbo);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvh_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER,render_scene.bvh_v.size()*sizeof(BVH),render_scene.bvh_v.data(),GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bvh_ssbo);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mat_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER,render_scene.mat_v.size()*sizeof(Mat),render_scene.mat_v.data(),GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mat_ssbo);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, prim_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER,render_scene.prim_v.size()*sizeof(uint32_t),render_scene.prim_v.data(),GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, prim_ssbo);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, light_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER,render_scene.light_v.size()*sizeof(Light),render_scene.light_v.data(),GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, light_ssbo);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


}
void Renderer::run(){
    if (camera.get_w()!=w || camera.get_h()!=h){
        resize(camera.get_w(),camera.get_h());
    }

    comp_shader.use();

    glBindImageTexture(0, cbuff, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    comp_shader.set_uint("width",w);
    comp_shader.set_uint("height",h);
    comp_shader.set_uint("tric",tric);
    comp_shader.set_uint("spherec",spherec);
    comp_shader.set_uint("bvhc",bvhc);
    comp_shader.set_uint("matc",matc);

    comp_shader.set_vec3("camPos",camera.get_pos());
    comp_shader.set_vec3("camForward",camera.get_forward());
    comp_shader.set_vec3("camRight",camera.get_right());
    comp_shader.set_vec3("camUp",camera.get_up());
    comp_shader.set_float("camFov",camera.get_fov());
    std::string y=std::to_string(camera.get_yaw());
    std::string p=std::to_string(camera.get_pitch());
    // std::cout<<"w: "+std::to_string(w)+"; h: "+std::to_string(h)<<std::endl;
    const int dx=(w+15)/16;
    const int dy=(h+15)/16;
    glDispatchCompute(dx, dy, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT);

    shader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cbuff);

    shader.set_int("tex",0);

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
