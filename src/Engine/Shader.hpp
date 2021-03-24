#pragma once

#include <string>
#include "GL.hpp"

namespace RayTracer
{
	class Shader
	{
		GLuint m_ShaderId;
	public:
		explicit Shader(GLenum programType, const std::string& program);
		~Shader();

		[[nodiscard]] inline GLuint ID() const { return m_ShaderId; }
	};
}
