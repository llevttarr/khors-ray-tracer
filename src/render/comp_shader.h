#ifndef COMP_SHADER_H
#define COMP_SHADER_H

#include <string>
#include <glad/gl.h>
#include <stdexcept>
#include "../util/glsl_util.h"

struct CShaderUseException: public std::runtime_error{
    using std::runtime_error::runtime_error;
};

class ComputeShader{
public:
    ComputeShader() = default;
    explicit ComputeShader(const std::string& path,const std::string& filename);
    ~ComputeShader();

    GLuint id() const{return progr; }
    void use();
    void set_int(const std::string& name, int v){ glsl_util::set_int(progr, v, name, cache); }
    void set_uint(const std::string& name, unsigned v){ glsl_util::set_uint(progr, v, name, cache); }
    void set_float(const std::string& name, float v){ glsl_util::set_float(progr, v, name, cache); }
    void set_vec3(const std::string& name, const Vec3<float>& v){ glsl_util::set_vec3(progr, v, name, cache);}
    void set_vec4(const std::string& name, const Vec4<float>& v){ glsl_util::set_vec4(progr, v, name, cache);}
    void set_mat3(const std::string& name, const Mat3<float>& v,bool transpose){ glsl_util::set_mat3(progr, v, name, cache, transpose);}
    void set_mat4(const std::string& name, const Mat4<float>& v,bool transpose){ glsl_util::set_mat4(progr, v, name, cache,transpose);}
private:
    GLuint progr=0;
    GLSLUniformCache cache;
    std::string process_imports(const std::string& str,const std::string& path);
};

#endif //COMPSHADER_H