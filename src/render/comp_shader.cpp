#include "comp_shader.h"
#include <sstream>
#include <iostream>

ComputeShader::ComputeShader(const std::string& path,const std::string& filename){
    const std::string comp_pre=glsl_util::read_file(filename);
    const std::string comp= process_imports(comp_pre,path);
    // std::cout << "FINAL SHADER:\n" << comp << std::endl;
    GLuint comp_s=glsl_util::create_shader(GL_COMPUTE_SHADER,comp);
    progr=glsl_util::link_comp(comp_s);
    cache.set_program(progr);

    glDeleteShader(comp_s);
}
std::string ComputeShader::process_imports(const std::string& str,const std::string& path){
    std::stringstream res;
    std::stringstream input(str);
    std::string line;
    while (std::getline(input, line)){
        if (line.find("//#import") != std::string::npos){
            std::string file = line.substr(line.find("\"")+1);
            file = file.substr(0, file.find("\""));
            res<< glsl_util::read_file(path+ file) << "\n";}
        else{
            res<< line << "\n";
        }
    }
    return res.str();
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
