#include "shader.h"


Shader::Shader(const std::string& vert_path, const std::string& frag_path){
    const std::string vert=glsl_util::read_file(vert_path);
    const std::string frag=glsl_util::read_file(frag_path);
    
    GLuint vert_s=glsl_util::create_shader(GL_VERTEX_SHADER,vert);
    GLuint frag_s=glsl_util::create_shader(GL_FRAGMENT_SHADER,frag);
    progr=glsl_util::link_shaders(vert_s,frag_s);

    cache.set_program(progr);
    glDeleteShader(vert_s);
    glDeleteShader(frag_s);
}
Shader::~Shader(){
    if(progr==0){
        return;
    }
    glDeleteProgram(progr);
    progr=0;
}
void Shader::use(){
    if (progr==0){
        throw ShaderUseException("could not use shader");
    }
    return glUseProgram(progr);
}
