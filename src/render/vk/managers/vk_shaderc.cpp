#include "vk_shaderc.h"

#include <shaderc/shaderc.hpp>

class GlslIncluder : public shaderc::CompileOptions::IncluderInterface {
private:
public:
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

VKShaderCompiler::VKShaderCompiler(){

}