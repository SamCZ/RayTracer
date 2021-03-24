#include <iostream>
#include <filesystem>
#include <fstream>

#define GLM_EXT_INCLUDED
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <lib/glm/glm/gtx/hash.hpp>

#include <lib/glm/glm/glm.hpp>
#include <lib/glm/glm/gtx/transform.hpp>
#include <lib/glm/glm/gtc/matrix_access.hpp>
#include <lib/glm/glm/gtx/quaternion.hpp>
#include <lib/glm/glm/gtc/type_ptr.hpp>
#include <lib/glm/glm/gtx/string_cast.hpp>

#include "Engine/GL.hpp"
#include <GLFW/glfw3.h>

#include "Engine/Shader.hpp"
#include "Engine/ShaderProgram.hpp"

using namespace RayTracer;

std::string ReadShader(const std::filesystem::path& path)
{
	std::ifstream stream(path.string());
	std::string line;
	std::string content;

	if (stream.is_open())
	{
		while (std::getline(stream, line))
		{
			content += line + "\n";
		}
		stream.close();
	}

	return content;
}

GLFWwindow* InitWindow(int w, int h)
{
	glfwInit();

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	auto windowHandle = glfwCreateWindow(w, h, "Ray Tracer - OpenGL", nullptr, nullptr);

	glfwMakeContextCurrent(windowHandle);

	// This is gonna break after second window is created !
	if(!gladLoadGL()) {
		printf("Something went wrong!\n");
		exit(-1);
	}
	printf("OpenGL %d.%d\n", GLVersion.major, GLVersion.minor);

	glfwSwapInterval(0);

	glfwShowWindow(windowHandle);

	return windowHandle;
}

int main()
{
	auto windowHandle = InitWindow(1280, 720);

	Shader ray_shader(GL_COMPUTE_SHADER, ReadShader("../res/compute.glsl"));
	auto ray_program = ShaderProgram::Create({&ray_shader});

	///
	int tex_w = 1280, tex_h = 720;
	GLuint tex_output;
	glGenTextures(1, &tex_output);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_output);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	/////

	auto projViewUniformId = glGetUniformLocation(ray_program->ID(), "ProjectionViewMatrix");

	auto projectionView = glm::perspective<float>(80, (float)1280 / (float)720, 0.1, 1000);

	float rotation = 0;

	auto PrevTime = glfwGetTime();
	auto lastFpsTime = PrevTime;
	int frameCount = 0;

	while(!glfwWindowShouldClose(windowHandle)) {
		auto CurrTime    = glfwGetTime();
		auto ElapsedTime = CurrTime - PrevTime;
		PrevTime         = CurrTime;

		frameCount++;
		if(CurrTime - lastFpsTime >= 1.0) {
			{
				std::stringstream fpsCounterSS;
				fpsCounterSS << "Ray Tracer - OpenGL" << " - " << (ElapsedTime * 1000.0);
				fpsCounterSS << " ms (" << frameCount << " fps)";

				glfwSetWindowTitle(windowHandle, fpsCounterSS.str().c_str());
			}

			frameCount = 0;
			lastFpsTime += 1.0;
		}

		glfwPollEvents();

		rotation += 0.1;

		auto viewMatrix = glm::rotate<float>(glm::radians<float>(rotation), glm::vec3(0, 1, 0));

		{ // launch compute shaders!
			glUseProgram(ray_program->ID());

			glUniformMatrix4fv(projViewUniformId, 1, GL_FALSE, glm::value_ptr(projectionView * viewMatrix));

			glDispatchCompute((GLuint)tex_w / 32, (GLuint)tex_h / 32, 1);
		}

		// make sure writing to image has finished before read
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		{ // normal drawing pass
			glClear(GL_COLOR_BUFFER_BIT);

			glEnable(GL_TEXTURE_2D);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex_output);

			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2f(-1.0f, -1.0f);
			glTexCoord2f(0, 1); glVertex2f(-1.0f, 1.0f);
			glTexCoord2f(1, 1); glVertex2f(1.0f, 1.0f);
			glTexCoord2f(1, 0); glVertex2f(1.0f, -1.0f);
			glEnd();
		}



		glfwSwapBuffers(windowHandle);
	}

	std::cout << "Hello, World!" << std::endl;

	glfwDestroyWindow(windowHandle);
	glfwTerminate();
	return 0;
}
