#ifndef VK_SHADERC_H
#define VK_SHADERC_H

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

class VKShaderCompiler {
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    VKShaderCompiler();
    ~VKShaderCompiler() = default;
    VKShaderCompiler(const VKShaderCompiler&) = delete;
    VKShaderCompiler& operator=(const VKShaderCompiler&) = delete;

    void set_optimization(int level);
    void set_generate_debug_info(bool v);
    void add_include_dir(const std::string& dir);
    void add_macro(const std::string& name, const std::string& value = "1");
    std::string compile_file(const std::string& src_path, bool force = false);
 
    std::vector<uint32_t> compile_glsl(const std::string& source,const std::string& src_name,int stage_hint);
    int compile_dir(const std::string& dir,bool recursive = false,bool force= false);
 
    static bool is_glsl_extension(const std::string& ext);
    static int infer_stage(const std::string& path);
};


#endif // VK_SHADERC_h