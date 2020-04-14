#ifndef SHADER_H_
#define SHADER_H_

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <glad/glad.h>
#include <glm/glm.hpp>


namespace render {

class ShaderProgram {
public:
    unsigned int ID;
    int Success;

    explicit ShaderProgram(const char* vex_path, const char* frag_path) {
        Success = 1;

        std::string vex_code;
        std::string frag_code;

        std::ifstream vex_file;
        std::ifstream frag_file;

        vex_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        frag_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            vex_file.open(vex_path);
            frag_file.open(frag_path);

            std::stringstream vex_stream, frag_stream;
            vex_stream << vex_file.rdbuf();
            frag_stream << frag_file.rdbuf();

            vex_file.close();
            frag_file.close();

            vex_code = std::move(vex_stream).str();
            frag_code = std::move(frag_stream).str();
        } catch (std::ifstream::failure& e) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }
        unsigned int vex_shader, frag_shader;

        vex_shader = glCreateShader(GL_VERTEX_SHADER);
        const char* vex_code_ptr = vex_code.c_str();
        glShaderSource(vex_shader, 1, &vex_code_ptr, NULL);
        glCompileShader(vex_shader);
        CheckCompileError(vex_shader, "Vertex");

        frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
        const char* frag_code_ptr = frag_code.c_str();
        glShaderSource(frag_shader, 1, &frag_code_ptr, NULL);
        glCompileShader(frag_shader);
        CheckCompileError(frag_shader, "Fragment");
        
        ID = glCreateProgram();
        glAttachShader(ID, vex_shader);
        glAttachShader(ID, frag_shader);

        glLinkProgram(ID);
        CheckLinkError(ID);

        glDeleteShader(vex_shader);
        glDeleteShader(frag_shader);
    }
    virtual ~ShaderProgram() {}

    void Use() {
        glUseProgram(ID);
    }

    void setVec3(const std::string& name, const glm::vec3& value) const { 
        glUniform3fv(GetUniformLocation(name), 1, &value[0]); 
    }

    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
    }
private:
    int GetUniformLocation(const std::string& name) const {
        int location = glGetUniformLocation(ID, name.c_str());
        if (location < 0) {
            std::cout << "ERROR::NOT_FOUND uniform " << name << std::endl;
        }
        return location;
    }

    void CheckCompileError(unsigned int shader, const std::string& name) {
        if (!Success) return;
        char infoLog[1024];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &Success);
        if(!Success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR " << name << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }

    void CheckLinkError(unsigned int program) {
        if (!Success) return;
        char infoLog[1024];
        glGetProgramiv(program, GL_LINK_STATUS, &Success);
        if(!Success) {
            glGetProgramInfoLog(program, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
};


}  // namespace render

#endif  // SHADER_H_