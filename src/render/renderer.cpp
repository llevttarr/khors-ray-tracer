#include "renderer.h"
#include <glad/gl.h>
#include <iostream>
#include <string>

constexpr int LOCAL_SIZE_X = 16;
constexpr int LOCAL_SIZE_Y = 4;
Renderer::Renderer(int width, int height,EulerCamera& cam)

  : w(width), h(height),camera(cam),
    shader("assets/shaders/vs.vert", "assets/shaders/fs.frag"),
    comp_shader("assets/shaders/", "assets/shaders/cs.comp")
{
    glGenBuffers(1, &tri_ssbo);
    glGenBuffers(1, &sphr_ssbo);
    glGenBuffers(1, &bvh_ssbo);
    glGenBuffers(1, &mat_ssbo);
    glGenBuffers(1, &prim_ssbo);
    glGenBuffers(1, &light_ssbo);
    glGenBuffers(1, &reservoir_a);
    glGenBuffers(1, &reservoir_b);
    glGenBuffers(1, &reservoir_h);
    glGenVertexArrays(1, &vao);
    glGenTextures(1, &cbuff);
    glBindTexture(GL_TEXTURE_2D, cbuff);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w,h,0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
    time_prev_sec=std::chrono::steady_clock::now();
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
    glDeleteTextures(1, &base_tex_arr);
    glDeleteTextures(1, &normal_tex_arr);
    glDeleteTextures(1, &specular_tex_arr);
    tric=(uint32_t)render_scene.tri_v.size();
    spherec=(uint32_t)render_scene.sphr_v.size();
    bvhc=(uint32_t)render_scene.bvh_v.size();
    matc=(uint32_t)render_scene.mat_v.size();
    lightc=(uint32_t)render_scene.light_v.size();
    tex_manager=render_scene.tex_manager;
    // primc=(uint32_t)render_scene.prim_v.size();


    for (size_t i =0;i<lightc;++i){
        std::cout<<"light "+std::to_string(i)<<":"<<std::endl;
        Light l =render_scene.light_v[i];
        std::cout<<std::to_string(l.pos.x)<<"; "<<std::to_string(l.pos.y)<<"; "<<std::to_string(l.pos.z)<<std::endl;
    }
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
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, reservoir_a);
    glBufferData(GL_SHADER_STORAGE_BUFFER, w* h * sizeof(Reservoir),nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, reservoir_a);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, reservoir_b);
    glBufferData(GL_SHADER_STORAGE_BUFFER, w* h * sizeof(Reservoir),nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, reservoir_b);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, reservoir_h);
    glBufferData(GL_SHADER_STORAGE_BUFFER, w* h * sizeof(Reservoir),nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, reservoir_h);
    
    base_tex_arr= create_texture_arr(tex_manager.get_base());
    normal_tex_arr =create_texture_arr(tex_manager.get_normal());
    specular_tex_arr = create_texture_arr(tex_manager.get_specular());
}
void Renderer::update_mats(RenderScene& render_scene){
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mat_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER,render_scene.mat_v.size()*sizeof(Mat),render_scene.mat_v.data(),GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mat_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
GLuint Renderer::create_texture_arr(const std::vector<Image>& img_v){
    GLuint tex=0;
    glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D_ARRAY,tex);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,GL_REPEAT);
    if (img_v.empty()) {
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 1, 1, 1);
        uint32_t px = 0x00000000;
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, 1, 1, 1,GL_RGBA, GL_UNSIGNED_BYTE, &px);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        return tex;
    }
    int w=img_v.at(0).w;
    int h=img_v.at(0).h;
    int layers=img_v.size();
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8,w,h,layers);

    for (size_t i=0; i<layers;++i) {
        const Image& img = img_v[i];
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY,0,0,0,i,w,h,1,GL_RGBA,GL_UNSIGNED_BYTE,img.data.data());
    }
    glBindTexture(GL_TEXTURE_2D_ARRAY,0);
    return tex;
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
    comp_shader.set_uint("lightc",lightc);

    comp_shader.set_uint("brdf_type",brdf_type);

    comp_shader.set_vec3("camPos",camera.get_pos());
    comp_shader.set_vec3("camForward",camera.get_forward());
    comp_shader.set_vec3("camRight",camera.get_right());
    comp_shader.set_vec3("camUp",camera.get_up());
    comp_shader.set_float("camFov",camera.get_fov());
    std::string y=std::to_string(camera.get_yaw());
    std::string p=std::to_string(camera.get_pitch());
    // std::cout<<"w: "+std::to_string(w)+"; h: "+std::to_string(h)<<std::endl;
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY,base_tex_arr);
    comp_shader.set_int("baseTexArr",1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D_ARRAY,normal_tex_arr);
    comp_shader.set_int("normalTexArr", 2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D_ARRAY, specular_tex_arr);
    comp_shader.set_int("specularTexArr", 3);
    const int dx=(w+LOCAL_SIZE_X-1)/LOCAL_SIZE_X;
    const int dy=(h+LOCAL_SIZE_Y-1)/LOCAL_SIZE_Y;
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
    get_fps();
}
void Renderer::get_fps(){
    auto curr_time=std::chrono::steady_clock::now();
    std::chrono::duration<double> diff=curr_time-time_prev_sec;
    ++frame_last_sec;
    if (diff.count()>=1.0){
        auto fps=frame_last_sec/diff.count();
        std::string f=std::to_string(fps);
        benchmark.update(f);
        std::cout<<"fps: "+f<<std::endl;
        frame_last_sec=0;
        time_prev_sec=curr_time;
    }
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
