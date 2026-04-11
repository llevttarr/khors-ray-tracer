#ifndef RENDERER_H
#define RENDERER_H

#include <cstdint>
#include <chrono>
#include "shader.h"
#include "comp_shader.h"
#include "../scene/scene.h"
#include "../scene/camera.h"
#include "../scene/textures.h"
#include "../util/benchmark.h"
#include "../core/math/vec3.h"
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

class Renderer {
public:
    explicit Renderer(int width, int height,EulerCamera& camera);
    ~Renderer();
    void run(uint8_t tracing_type=1);
    void run_di();
    void run_rs();
    void get_fps();
    void bind_unif(ComputeShader& cs);
    void bind_stor_buff(int i,size_t size,GLenum glt,GLuint buff, const void * dat);
    void resize(int nw, int nh);
    void update_scene(RenderScene& render_scene);
    void update_mats(RenderScene& render_scene);
    GLuint create_texture_arr(const std::vector<Image>& img_v);
private:
    int w;
    int h;
    Shader shader;
    
    ComputeShader comp_shader;

    ComputeShader cs_res_sampling;
    ComputeShader cs_temp_reuse;
    ComputeShader cs_spat_reuse;
    ComputeShader cs_res_shade;

    EulerCamera& camera;
    CameraSnapshot prev_camera;
    Benchmark benchmark;
    TextureManager tex_manager;

    GLuint vao = 0;
    GLuint cbuff = 0;
    GLuint tri_ssbo = 0;
    GLuint sphr_ssbo = 0;
    GLuint bvh_ssbo = 0;
    GLuint mat_ssbo = 0;
    GLuint prim_ssbo = 0;
    GLuint light_ssbo = 0;
    GLuint base_tex_arr=0;
    GLuint normal_tex_arr=0;
    GLuint specular_tex_arr=0;

    GLuint reservoir_a=0;
    GLuint reservoir_b=0;
    GLuint reservoir_h=0;
    GLuint gbuffer=0;
    
    uint32_t tric = 0;
    uint32_t spherec = 0;
    uint32_t lightc = 0;
    uint32_t framec=0;
    uint32_t init_candidates_restir=32;
    uint32_t bvhc=0;
    uint32_t matc=0;

    uint32_t brdf_type=0;

    uint32_t frame_last_sec=0;
    std::chrono::time_point<std::chrono::steady_clock> time_prev_sec;

    // GLuint vbo = 0;
};
#endif //RENDERER_H
