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

#define STB_IMAGE_IMPLEMENTATION
#include "Engine/stb_image.h"

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

int GetIndex(int x, int y, int z)
{
	return (x * 16 + z) * 16 + y;
}

int main()
{
	auto windowHandle = InitWindow(1280, 720);

	Shader ray_shader(GL_COMPUTE_SHADER, ReadShader("../res/compute.glsl"));
	auto ray_program = ShaderProgram::Create({&ray_shader});

	int width, height, nrChannels;
	unsigned char *data = stbi_load("../res/wood_planks.png", &width, &height, &nrChannels, 0);

	if(!data) {
		std::cerr << "Cannot load image !" << std::endl;
	}

	std::cout << width << ", " << height << ", " << nrChannels << std::endl;

	GLuint tex_input;
	glGenTextures(1, &tex_input);
	glBindTexture(GL_TEXTURE_2D, tex_input);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	if(data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	stbi_image_free(data);
	/////
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

	int blocks[16 * 16 * 16];

	for(int x = 0; x < 16; x++) {
		for(int y = 0; y < 16; y++) {
			for(int z = 0; z < 16; z++) {

				blocks[GetIndex(x, y, z)] = 1;
			}
		}
	}

	GLuint ssbo;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(blocks), blocks, GL_READ_ONLY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

	////

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

			glActiveTexture(GL_TEXTURE0);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, tex_output);
			glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

			glActiveTexture(GL_TEXTURE0 + 1);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, tex_input);

			glDispatchCompute((GLuint)tex_w / 16, (GLuint)tex_h / 16, 1);
		}

		// make sure writing to image has finished before read
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		for (int i = 0; i < 6; ++i) {
			glActiveTexture(GL_TEXTURE0+i);
			glDisable(GL_TEXTURE_2D);
		}

		{ // normal drawing pass
			glClear(GL_COLOR_BUFFER_BIT);

			glActiveTexture(GL_TEXTURE0);
			glEnable(GL_TEXTURE_2D);
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
