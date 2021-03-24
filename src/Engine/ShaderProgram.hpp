#pragma once

#include <vector>
#include <memory>
#include "Shader.hpp"

namespace RayTracer
{
	class ShaderProgram
	{
	private:
		GLuint m_ProgramId;
	public:
		inline explicit ShaderProgram(GLuint programId) : m_ProgramId(programId) { }

		inline ~ShaderProgram()
		{
			if(m_ProgramId > 0) {
				glDeleteProgram(m_ProgramId);
				std::cout << "Destroyed program: " << m_ProgramId << std::endl;
			}
		}

		[[nodiscard]] inline GLuint ID() const { return m_ProgramId; }

		static std::shared_ptr<ShaderProgram> Create(const std::vector<Shader*>& shaders)
		{
			GLuint program = glCreateProgram();

			for(Shader* shader : shaders) {
				glAttachShader(program, shader->ID());
				glLinkProgram(program);

				{
					GLint success = 0;
					GLchar error[1024] = { 0 };

					glGetProgramiv(program, GL_LINK_STATUS, &success);

					if (success == GL_FALSE) {
						glGetProgramInfoLog(program, sizeof(error), nullptr, error);
						std::cerr << error << std::endl;
					}
				}
			}

			glValidateProgram(program);

			{
				GLint success = 0;
				GLchar error[1024] = { 0 };

				glGetProgramiv(program, GL_VALIDATE_STATUS, &success);

				if (success == GL_FALSE) {
					glGetProgramInfoLog(program, sizeof(error), nullptr, error);
					std::cerr << error << std::endl;
				}
			}

			std::cout << "Created program: " << program << std::endl;

			return std::make_shared<ShaderProgram>(program);
		}
	};
}
