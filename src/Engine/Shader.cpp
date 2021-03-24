#include "Shader.hpp"
#include <iostream>

namespace RayTracer
{
	Shader::Shader(GLenum programType, const std::string &program) : m_ShaderId(0)
	{
		m_ShaderId = glCreateShader(programType);

		const GLchar* p[1];
		p[0] = program.c_str();
		GLint lengths[1];
		lengths[0] = program.length();
		glShaderSource(m_ShaderId, 1, p, lengths);
		glCompileShader(m_ShaderId);

		GLint success = 0;
		GLchar error[1024] = { 0 };

		glGetShaderiv(m_ShaderId, GL_COMPILE_STATUS, &success);

		if (success == GL_FALSE) {
			glGetShaderInfoLog(m_ShaderId, sizeof(error), nullptr, error);
			std::cerr << "Cannot compile shader: " << m_ShaderId << std::endl << error << std::endl;
		} else {
			std::cout << "Created new shader: " << m_ShaderId << std::endl;
		}
	}

	Shader::~Shader()
	{
		if(m_ShaderId > 0) {
			glDeleteShader(m_ShaderId);
			std::cout << "Destroyed shader: " << m_ShaderId << std::endl;
		}
	}
}