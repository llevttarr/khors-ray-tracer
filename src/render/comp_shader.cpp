#include "comp_shader.h"


ComputeShader::ComputeShader(const std::string& filename){
    const std::string comp=glsl_util::read_file(filename);
    
    GLuint comp_s=glsl_util::create_shader(GL_COMPUTE_SHADER,comp);
    progr=glsl_util::link_comp(comp_s);
    cache.set_program(progr);

    glDeleteShader(comp_s);
}
ComputeShader::~ComputeShader(){
    if(progr==0){
        return;
    }
    glDeleteProgram(progr);
    progr=0;
}
void ComputeShader::use(){
    if (progr==0){
        throw CShaderUseException("could not use comp shader");
    }
    return glUseProgram(progr);
}
