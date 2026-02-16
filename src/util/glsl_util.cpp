
#include <fstream>
#include <sstream>
#include "glsl_util.h"
void GLSLUniformCache::set_program(GLuint program) {
    progr = program;
    loc_map.clear();
}
GLint GLSLUniformCache::get_location(const std::string& name) {
        if (progr==0){
            throw GLSLUtilException("no progr");
        }
        auto it = loc_map.find(name);
        if (it != loc_map.end()){
            return it->second;
        }
        GLint l = glGetUniformLocation(progr, name.c_str());
        loc_map.emplace(name, l);
        return l;
    }

namespace glsl_util{

    std::string read_file(const std::string& filename){
        std::ifstream instr(filename);
        if(!instr){
            throw GLSLUtilException("Could not read file");
        }
        std::stringstream ss;
        ss << instr.rdbuf();
        return ss.str();
    }
    GLuint create_shader(GLenum t,const std::string& s){
        const char* ch=s.c_str();
        GLuint shader=glCreateShader(t);
        glShaderSource(shader,1,&ch,nullptr);
        glCompileShader(shader);
        GLint status=0;
        glGetShaderiv(shader,GL_COMPILE_STATUS,&status);
        if(status==0){
            throw GLSLUtilException("could not create shader ");
        }
        return shader;
    }

    GLuint link_shaders(GLuint vert, GLuint frag){
        GLuint program =glCreateProgram();
        glAttachShader(program,vert);
        glAttachShader(program,frag); 
        glLinkProgram(program);
        GLint status=0;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if(status==0){
            throw GLSLUtilException("could not link shaders ");
        }
        return program;
    }GLuint link_comp(GLuint comp){
        GLuint program =glCreateProgram();
        glAttachShader(program,comp); 
        glLinkProgram(program);
        GLint status=0;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if(status==0){
            throw GLSLUtilException("could not link comp shader");
        }
        return program;
    }
    void set_int(GLuint program,int i,const std::string& name,GLSLUniformCache& c){
        GLint loc = c.get_location(name);
        if(loc==0){
            return;
        }
        glProgramUniform1i(program, loc,i);
    }
    void set_uint(GLuint program,unsigned i,const std::string& name,GLSLUniformCache& c){
        GLint loc = c.get_location(name);
        if(loc==0){
            return;
        }
        glProgramUniform1ui(program, loc,i);
    }
    void set_float(GLuint program,float i,const std::string& name,GLSLUniformCache& c){
        GLint loc = c.get_location(name);
        if(loc==0){
            return;
        }
        glProgramUniform1f(program, loc,i);
    }
    void set_mat3(GLuint program,const Mat3<float>& m,const std::string& name,GLSLUniformCache& c,bool transpose){
        GLint loc = c.get_location(name);
        if(loc==0){
            return;
        }
        if (transpose){
            glProgramUniformMatrix3fv(program,loc,1, GL_TRUE, m.data());
            return;
        }
        glProgramUniformMatrix3fv(program,loc,1, GL_FALSE, m.data());
    }
    void set_mat4(GLuint program,const Mat4<float>& m,const std::string& name,GLSLUniformCache& c,bool transpose){
        GLint loc = c.get_location(name);
        if(loc==0){
            return;
        }
        if (transpose){
            glProgramUniformMatrix4fv(program,loc,1, GL_TRUE, m.data());
            return;
        }
        glProgramUniformMatrix4fv(program,loc,1, GL_FALSE, m.data());

    }
    void set_vec3(GLuint program,const Vec3<float>& v,const std::string& name,GLSLUniformCache& c){
        GLint loc = c.get_location(name);
        if(loc==0){
            return;
        }
        glProgramUniform3f(program, loc, v.x, v.y, v.z);
    }
    void set_vec4(GLuint program,const Vec4<float>& v,const std::string& name,GLSLUniformCache& c){
        GLint loc = c.get_location(name);
        if(loc==0){
            return;
        }
        glProgramUniform4f(program, loc, v.x, v.y, v.z, v.w);
    }
}