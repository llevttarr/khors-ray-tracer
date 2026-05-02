#include "renderer.h"
#include <glad/gl.h>
#include <iostream>
#include <string>

constexpr int LOCAL_SIZE_X = 16;
constexpr int LOCAL_SIZE_Y = 4;
Renderer::Renderer(int width, int height,EulerCamera& cam)

  : w(width), h(height),camera(cam),
    shader("assets/shaders/vs.vert", "assets/shaders/fs.frag"),
    comp_shader("assets/shaders/", "assets/shaders/cs.comp"),
    cs_res_sampling("assets/shaders/", "assets/shaders/res_sampling.comp"),
    cs_temp_reuse("assets/shaders/", "assets/shaders/temp_reuse.comp"),
    cs_spat_reuse("assets/shaders/", "assets/shaders/spat_reuse.comp"),
    cs_res_shade("assets/shaders/", "assets/shaders/res_shade.comp"),
    restir_timers(4)
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
    glGenBuffers(1, &gbuffer);
    glGenBuffers(1, &gbuffer_h);
    glGenVertexArrays(1, &vao);
    glGenTextures(1, &cbuff);
    glBindTexture(GL_TEXTURE_2D, cbuff);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w,h,0, GL_RGBA, GL_FLOAT, nullptr);

    glGenTextures(1, &accum_tex);
    glBindTexture(GL_TEXTURE_2D, accum_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w,h,0, GL_RGBA, GL_FLOAT, nullptr);

    glGenTextures(1, &refl_accum_tex);
    glBindTexture(GL_TEXTURE_2D, refl_accum_tex);
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
    glDeleteBuffers(1,&reservoir_a);
    glDeleteBuffers(1,&reservoir_b);
    glDeleteBuffers(1,&reservoir_h);
    glDeleteBuffers(1,&gbuffer);
    glDeleteBuffers(1,&gbuffer_h);
    glDeleteTextures(1,&cbuff);
    glDeleteTextures(1,&accum_tex);
    glDeleteTextures(1,&refl_accum_tex);
}
bool Renderer::camera_moved() {
    return prev_camera.pos!= camera.get_pos()||prev_camera.forward != camera.get_forward()||prev_camera.fov != camera.get_fov();
}
void Renderer::cout_data(){
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "GPU Vendor: " << vendor << std::endl;
    std::cout << "GPU Renderer: " << renderer << std::endl;
    std::cout << "OpenGL Version: " << version << std::endl;
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
    int i=1;
    bind_stor_buff(i++,render_scene.tri_v.size()*sizeof(RenderTri),GL_STATIC_DRAW,tri_ssbo,render_scene.tri_v.data());
    bind_stor_buff(i++,render_scene.sphr_v.size()*sizeof(Sphr),GL_STATIC_DRAW,sphr_ssbo,render_scene.sphr_v.data());
    bind_stor_buff(i++,render_scene.bvh_v.size()*sizeof(BVH),GL_STATIC_DRAW,bvh_ssbo,render_scene.bvh_v.data());
    bind_stor_buff(i++,render_scene.mat_v.size()*sizeof(Mat),GL_STATIC_DRAW,mat_ssbo,render_scene.mat_v.data());
    bind_stor_buff(i++,render_scene.prim_v.size()*sizeof(uint32_t),GL_STATIC_DRAW,prim_ssbo,render_scene.prim_v.data());
    bind_stor_buff(i++,render_scene.light_v.size()*sizeof(Light),GL_STATIC_DRAW,light_ssbo,render_scene.light_v.data());
    
    bind_stor_buff(i++,w* h * sizeof(Reservoir),GL_DYNAMIC_COPY,reservoir_a,nullptr);
    bind_stor_buff(i++,w* h * sizeof(Reservoir),GL_DYNAMIC_COPY,reservoir_b,nullptr);
    bind_stor_buff(i++,w* h * sizeof(Reservoir),GL_DYNAMIC_COPY,reservoir_h,nullptr);
    bind_stor_buff(i++,w* h * sizeof(GBufferPixel),GL_DYNAMIC_COPY,gbuffer,nullptr);
    bind_stor_buff(i++,w* h * sizeof(GBufferPixel),GL_DYNAMIC_COPY,gbuffer_h,nullptr);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    base_tex_arr= create_texture_arr(tex_manager.get_base());
    normal_tex_arr =create_texture_arr(tex_manager.get_normal());
    specular_tex_arr = create_texture_arr(tex_manager.get_specular());
}
void Renderer::bind_stor_buff(int i,size_t size,GLenum glt,GLuint buff, const void * dat){
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buff);
    glBufferData(GL_SHADER_STORAGE_BUFFER,size,dat,glt);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,i, buff);
}
void Renderer::update_mats(RenderScene& render_scene){
    framec=0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mat_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER,render_scene.mat_v.size()*sizeof(Mat),render_scene.mat_v.data(),GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mat_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
void Renderer::update_lights(RenderScene& render_scene){
    framec=0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, light_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER,render_scene.light_v.size()*sizeof(Light),render_scene.light_v.data(),GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, light_ssbo);
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
    if (tracing_type==0){
        run_di();
    }else{
        run_rs();
    }
}
void Renderer::run_rs(){
    if (camera.get_w()!=w || camera.get_h()!=h){
        resize(camera.get_w(),camera.get_h());
    }
    if (camera_moved()){
        // std::cout<<"camera moved"<<std::endl;
        framec = 0;
    }
    const int dx=(w+LOCAL_SIZE_X-1)/LOCAL_SIZE_X;
    const int dy=(h+LOCAL_SIZE_Y-1)/LOCAL_SIZE_Y;
    {
        // - stage 1:= reservoir_a, gbuffer -
        GpuTimerScope scope(restir_timers[0].get_id());

        bind_unif(cs_res_sampling);
        glDispatchCompute(dx, dy, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
    {
    // - stage 2:= reservoir_a write, reservoir_b read, reservoir_h read -
        GpuTimerScope scope(restir_timers[1].get_id());
    
        if (framec == 0) {
            glBindBuffer(GL_COPY_READ_BUFFER, gbuffer);
            glBindBuffer(GL_COPY_WRITE_BUFFER, gbuffer_h);
            glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, w * h * sizeof(GBufferPixel));
        }
        bind_unif(cs_temp_reuse); // todo: prev view proj
        cs_temp_reuse.set_vec3("prevCamPos",prev_camera.pos);
        cs_temp_reuse.set_vec3("prevCamForward",prev_camera.forward);
        cs_temp_reuse.set_vec3("prevCamRight",prev_camera.right);
        cs_temp_reuse.set_vec3("prevCamUp",prev_camera.up);
        cs_temp_reuse.set_float("prevCamFov",prev_camera.fov);
        cs_temp_reuse.set_float("prevAspect",prev_camera.aspect);
        glDispatchCompute(dx, dy, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
    {
        // - stage 3:= reservoir_a read, reservoir_b write -
        GpuTimerScope scope(restir_timers[2].get_id());
        bind_unif(cs_spat_reuse);
        glDispatchCompute(dx, dy, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
    {
        // - stage 4:= reservoir_a read, reservoir_b write -
        GpuTimerScope scope(restir_timers[3].get_id());
        bind_unif(cs_res_shade);
        glBindImageTexture(0, cbuff, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glBindImageTexture(1, accum_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(2, refl_accum_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glDispatchCompute(dx, dy, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
    // postprocess
    framec++;
    prev_camera = { camera.get_pos(), camera.get_forward(),camera.get_right(), camera.get_up(),camera.get_fov(), float(w)/float(h)};
    // todo: prev proj view
    std::swap(reservoir_a, reservoir_h);
    std::swap(gbuffer_h, gbuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, reservoir_a);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, reservoir_h);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, gbuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, gbuffer_h);

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
    glFinish();
    get_timer_results();
}
void Renderer::get_timer_results(){
    if (timerc < 60){
        ++timerc;
        return;
    }
    timerc = 0;

    for (size_t i = 0; i < restir_timers.size(); ++i) {
        GLuint available = 0;
        glGetQueryObjectuiv(restir_timers[i].get_id(), GL_QUERY_RESULT_AVAILABLE, &available);
        
        if (available) {
            GLuint64 time_elapsed = 0;
            glGetQueryObjectui64v(restir_timers[i].get_id(), GL_QUERY_RESULT, &time_elapsed);
            double ms = time_elapsed / 1000000.0;
            std::cout << "Compute Shader " << i + 1 << " took: " << ms << " ms"<<std::endl;
        }
    }
}
void Renderer::run_di(){
    if (camera.get_w()!=w || camera.get_h()!=h){
        resize(camera.get_w(),camera.get_h());
    }
    bind_unif(comp_shader);
    glBindImageTexture(0, cbuff, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

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
void Renderer::switch_brdf(){
    if (brdf_type==0){
        brdf_type=1;
    }else{
        brdf_type=0;
    }
}
void Renderer::switch_tt(){
    if (tracing_type==0){
        tracing_type=1;
    }else{
        tracing_type=0;
    }
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
    framec=0;
    h=nh;
    w=nw;

    glBindTexture(GL_TEXTURE_2D, cbuff);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, nw,nh,0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, accum_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, nw,nh,0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, refl_accum_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, nw,nh,0, GL_RGBA, GL_FLOAT, nullptr);

    comp_shader.set_uint("width",nw);
    comp_shader.set_uint("height",nh);
    size_t pixel_count = w * h;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gbuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 
                 pixel_count * sizeof(GBufferPixel), nullptr, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gbuffer_h);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                pixel_count * sizeof(GBufferPixel), nullptr, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, reservoir_a);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 
                 pixel_count * sizeof(Reservoir), nullptr, GL_DYNAMIC_COPY);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, reservoir_b);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 
                 pixel_count * sizeof(Reservoir), nullptr, GL_DYNAMIC_COPY);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, reservoir_h);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 
                 pixel_count * sizeof(Reservoir), nullptr, GL_DYNAMIC_COPY);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
void Renderer::bind_unif(ComputeShader& sh){
    sh.use();
    sh.set_uint("width",w);
    sh.set_uint("height",h);
    sh.set_uint("tric", tric);
    sh.set_uint("spherec",spherec);
    sh.set_uint("bvhc", bvhc);
    sh.set_uint("matc", matc);
    sh.set_uint("lightc", lightc);
    sh.set_uint("framec", framec);
    sh.set_uint("init_candidates_restir", init_candidates_restir);
    sh.set_vec3("camPos", camera.get_pos());
    sh.set_vec3("camForward",camera.get_forward());
    sh.set_vec3("camRight", camera.get_right());
    sh.set_vec3("camUp",camera.get_up());
    sh.set_float("camFov",camera.get_fov());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, base_tex_arr);
    sh.set_int("baseTexArr", 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D_ARRAY, normal_tex_arr);
    sh.set_int("normalTexArr", 2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D_ARRAY, specular_tex_arr);
    sh.set_int("specularTexArr", 3);
}
