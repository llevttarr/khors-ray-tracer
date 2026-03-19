#ifndef GLSL_UTIL_H
#define GLSL_UTIL_H

#include <string>
#include <glad/gl.h>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include "../core/math/mat3.h"
#include "../core/math/mat4.h"
#include "../core/math/vec4.h"
#include "../core/math/vec3.h"

struct GLSLUtilException: public std::runtime_error{
    using std::runtime_error::runtime_error;
};
class GLSLUniformCache{
public:
    explicit GLSLUniformCache(GLuint program=0):progr(program){}

    void set_program(GLuint program);

    GLint get_location(const std::string& name);

private:
    GLuint progr = 0;
    std::unordered_map<std::string, GLint> loc_map;
};

namespace glsl_util{
    std::string read_file(const std::string& filename);
    void out_csv(const std::string& firstline,const std::string& filename,const std::vector<std::string>& outp);
    GLuint create_shader(GLenum t,const std::string& s);
    GLuint link_shaders(GLuint vert, GLuint frag);
    GLuint link_comp(GLuint comp);
    void set_int(GLuint program,int i,const std::string& name,GLSLUniformCache& c);
    void set_uint(GLuint program,unsigned i,const std::string& name,GLSLUniformCache& c);
    void set_float(GLuint program,float i,const std::string& name,GLSLUniformCache& c);
    void set_mat3(GLuint program,const Mat3<float>& m,const std::string& name,GLSLUniformCache& c,bool transpose);
    void set_mat4(GLuint program,const Mat4<float>& m,const std::string& name,GLSLUniformCache& c,bool transpose);
    void set_vec3(GLuint program,const Vec3<float>& v,const std::string& name,GLSLUniformCache& c);
    void set_vec4(GLuint program,const Vec4<float>& v,const std::string& name,GLSLUniformCache& c);
}
#endif //GLSL_UTIL_H
