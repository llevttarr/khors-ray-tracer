#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <glad/gl.h>
#include <stdexcept>

#include "glsl_util.h"

struct ShaderUseException: public std::runtime_error{
    using std::runtime_error::runtime_error;
};

class Shader{
public:
    Shader() = default;
    explicit Shader(const std::string& frag_path,const std::string& vert_path);
    ~Shader();

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
    // std::string read_file(const std::string& filename);
    // GLuint create_shader(GLenum t,const std::string& s);
    // GLuint create_shader_program(GLuint vert, GLuint frag);
};

#endif //SHADER_H