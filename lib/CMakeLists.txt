set(BUILD_SHARED_LIBS OFF)

add_subdirectory(glm)

#------------------------------
# GLFW
# Desktop Window Library
#------------------------------
set(GLFW_INSTALL OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(BUILD_SHARED_LIBS OFF)
set(GLFW_USE_HYBRID_HPG ON)
add_subdirectory(glfw)

#------------------------------
# glew
# OpenGL wrapper
# Before build you need to check this: https://github.com/nigels-com/glew/issues/13
#------------------------------
add_subdirectory(glad)