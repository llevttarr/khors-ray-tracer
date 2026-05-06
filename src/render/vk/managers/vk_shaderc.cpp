#include "vk_shaderc.h"

#include <shaderc/shaderc.hpp>

class GlslIncluder : public shaderc::CompileOptions::IncluderInterface {
private:
    std::vector<std::string> include_dirs;
public:
    explicit GlslIncluder(std::vector<std::string> dirs): include_dirs(std::move(dirs)) {}
 
    shaderc_include_result* GetInclude(const char* requested_source,shaderc_include_type,const char* requesting_source,size_t df) override{
        auto* result = new shaderc_include_result{};

        std::vector<std::filesystem::path> search;
        search.push_back(std::filesystem::path(requesting_source).parent_path());
        for (const auto& d : include_dirs){
            search.emplace_back(d);
        }
        for (const auto& base : search) {
            std::filesystem::path candidate = base / requested_source;
            if (std::filesystem::exists(candidate)) {
                std::ifstream f(candidate, std::ios::binary);
                if (!f){
                    continue;
                }
                std::string* content = new std::string((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
                std::string* name = new std::string(candidate.string());
 
                result->source_name= name->c_str();
                result->source_name_length = name->size();
                result->content= content->c_str();
                result->content_length = content->size();
                result->user_data = new std::pair<std::string*, std::string*>(content, name);
                return result;
            }
        }
        const std::string msg = "include file not found";
        result->source_name = "";
        result->source_name_length = 0;
        result->content= msg.c_str();
        result->content_length=msg.size();
        result->user_data = nullptr;
        return result;
    }
 
    void ReleaseInclude(shaderc_include_result* data) override {
        if (data->user_data) {
            auto* p = static_cast<std::pair<std::string*, std::string*>*>(data->user_data);
            delete p->first;
            delete p->second;
            delete p;
        }
        delete data;
    }

};

struct VKShaderCompiler::Impl {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    std::vector<std::string> include_dirs;
    Impl() {
        options.SetTargetEnvironment(shaderc_target_env_vulkan,shaderc_env_version_vulkan_1_3);
        options.SetOptimizationLevel(shaderc_optimization_level_zero);
        options.AddMacroDefinition("VULKAN", "1");
    }
    void rebuild_includer() {
        options.SetIncluder(
            std::make_unique<GlslIncluder>(include_dirs));
    }
};
 
VKShaderCompiler::VKShaderCompiler(): impl(std::make_unique<Impl>()) {}

static const std::unordered_map<std::string, shaderc_shader_kind> EXT_MAP = {
    { ".vert", shaderc_glsl_vertex_shader},
    { ".frag", shaderc_glsl_fragment_shader},
    { ".comp", shaderc_glsl_compute_shader},
    { ".rgen", shaderc_glsl_raygen_shader },
    { ".rmiss", shaderc_glsl_miss_shader },
    { ".rchit", shaderc_glsl_closesthit_shader },
    { ".rahit",shaderc_glsl_anyhit_shader },
    { ".rint",shaderc_glsl_intersection_shader },
    { ".rcall",shaderc_glsl_callable_shader },
    { ".tesc", shaderc_glsl_tess_control_shader },
    { ".tese", shaderc_glsl_tess_evaluation_shader},
    { ".geom", shaderc_glsl_geometry_shader},
};
 
void VKShaderCompiler::set_optimization(int level) {
    impl->options.SetOptimizationLevel(
        static_cast<shaderc_optimization_level>(level));
}
 
void VKShaderCompiler::set_generate_debug_info(bool v) {
    if (v) impl->options.SetGenerateDebugInfo();
}
 
void VKShaderCompiler::add_include_dir(const std::string& dir) {
    impl->include_dirs.push_back(dir);
    impl->rebuild_includer();
}
 
void VKShaderCompiler::add_macro(const std::string& name, const std::string& value) {
    impl->options.AddMacroDefinition(name, value);
}

bool VKShaderCompiler::is_glsl_extension(const std::string& ext) {
    return EXT_MAP.count(ext) > 0;
}
 
int VKShaderCompiler::infer_stage(const std::string& path) {
    std::filesystem::path p(path);
    if (p.extension() == ".spv")
        p = p.stem();
 
    const std::string ext = p.extension().string();
    auto it = EXT_MAP.find(ext);
    if (it != EXT_MAP.end()) return static_cast<int>(it->second);
    return static_cast<int>(shaderc_glsl_infer_from_source);
}
// std::string VKShaderCompiler::process_imports(const std::string& source,const std::filesystem::path& base_path,std::unordered_set<std::filesystem::path>& included_files) {
//     std::stringstream input(source);
//     std::stringstream output;
//     std::string line;

//     while (std::getline(input, line)) {
//         if (line.find("//#import") != std::string::npos) {
//             size_t start = line.find("\"");
//             size_t end = line.find("\"", start + 1);

//             if (start == std::string::npos || end == std::string::npos) {
//                 throw std::runtime_error("err at //#import line: " + line);
//             }

//             std::string file = line.substr(start + 1, end-start- 1);

//             std::filesystem::path full_path = base_path / file;
//             std::string full_path_str = full_path.string();
//             if (included_files.find(full_path_str) != included_files.end())
//                 continue;
//             included_files.insert(full_path_str);

//             std::ifstream f(full_path);
//             if (!f) {
//                 throw std::runtime_error("cannot open import: " + full_path_str);
//             }

//             std::string imported((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
//             output << process_imports(imported, full_path.parent_path(), included_files) << std::endl;
//         }
//         else {
//             output << line << std::endl;
//         }
//     }

//     return output.str();
// }
std::string VKShaderCompiler::compile_file(const std::string& src_path, bool force) {
    const std::string out_path = src_path + ".spv";
    if (!force && std::filesystem::exists(out_path)) {
        const auto src_time = std::filesystem::last_write_time(src_path);
        const auto spv_time = std::filesystem::last_write_time(out_path);
        if (spv_time >= src_time) {
            return out_path;
        }
    }

    std::ifstream f(src_path, std::ios::binary);
    if (!f){
        throw std::runtime_error("shader compiler: cannot open " + src_path);
    }
    std::string source((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());

    // std::unordered_set<std::filesystem::path> included_files;
    // std::string source = process_imports(raw_source,std::filesystem::path(src_path).parent_path(),included_files);
    const auto stage = static_cast<shaderc_shader_kind>(infer_stage(src_path));
    if (stage == static_cast<shaderc_shader_kind>(shaderc_glsl_infer_from_source)) {
        throw std::runtime_error("shaderC: unknown extension for " + src_path);
    }
 
    const auto spv_words = compile_glsl(source, src_path, static_cast<int>(stage));

    std::ofstream out(out_path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("shader compiler: cannot write " + out_path);
    out.write(reinterpret_cast<const char*>(spv_words.data()),spv_words.size() * sizeof(uint32_t));
    return out_path;
}
 
std::vector<uint32_t> VKShaderCompiler::compile_glsl(const std::string& source,const std::string& src_name,int stage_hint) {
    const auto stage = static_cast<shaderc_shader_kind>(stage_hint);
 
    const shaderc::SpvCompilationResult result =impl->compiler.CompileGlslToSpv(source, stage, src_name.c_str(), impl->options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        throw std::runtime_error(std::string("Compilation failed (") + src_name + "):=" +result.GetErrorMessage() +"\n");
    }
    if (result.GetNumWarnings() > 0) {
        std::cerr << "shadeer compiler: warnings for " << src_name<< ":="<<result.GetErrorMessage() <<std::endl;
    }
    return { result.cbegin(), result.cend() };
}
 
int VKShaderCompiler::compile_dir(const std::string& dir, bool recursive, bool force) {
    int compiled = 0;
    auto process = [&](const std::filesystem::directory_entry& entry) {
        if (!entry.is_regular_file()){
            return;
        }
 
        const std::filesystem::path p = entry.path();
        if (p.extension() == ".spv"){
            return;
        }

        if (!is_glsl_extension(p.extension().string())) return;
 
        try {
            const std::string compiled_path = compile_file(p.string(), force);
            std::cout << "compiled " << p.filename().string()<< " to " << std::filesystem::path(compiled_path).filename().string() << std::endl;
            ++compiled;
        } catch (const std::exception& e) {
            std::cerr << "shader compiler err:" << e.what() <<std::endl;
        }
    };
 
    if (recursive) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
            process(entry);
    } else {
        for (const auto& entry : std::filesystem::directory_iterator(dir))
            process(entry);
    } 
    return compiled;
}
