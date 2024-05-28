cmake_minimum_required(VERSION 3.2.0...3.5.0)
project(phi VERSION 0.3.2)

# Find OpenGL
set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)

# Add cmake project folders
add_subdirectory(thirdparty/glfw)
add_subdirectory(thirdparty/glm)
add_subdirectory(thirdparty/glew/build/cmake)
add_subdirectory(thirdparty/yaml-cpp)

# Add all project-wide includes
include_directories(
    ${CMAKE_SOURCE_DIR}
    thirdparty
    thirdparty/glfw/include
    thirdparty/glm
    thirdparty/glew/include
    thirdparty/imgui
    ${OPENGL_INCLUDE_DIRS})

# Add ImGui
set(IMGUI_PATH ${CMAKE_SOURCE_DIR}/thirdparty/imgui/)
file(GLOB_RECURSE IMGUI_SOURCES ${IMGUI_PATH}/*.cpp)
add_library(ImGui STATIC ${IMGUI_SOURCES})

# Glob source for engine
file(GLOB_RECURSE PHI_SOURCE ${CMAKE_SOURCE_DIR}/phi/*.c ${CMAKE_SOURCE_DIR}/phi/*.cpp)
file(GLOB_RECURSE PHI_HEADERS ${CMAKE_SOURCE_DIR}/phi/*.h ${CMAKE_SOURCE_DIR}/phi/*.hpp)


# TOOL APPS


# Particle effect editor
set(PARTICLE_EFFECT_EDITOR_SOURCE ${CMAKE_SOURCE_DIR}/tools/particle_effect_editor.cpp)
set(PARTICLE_EFFECT_EDITOR_HEADER ${CMAKE_SOURCE_DIR}/tools/particle_effect_editor.hpp)
add_executable(particle_effect_editor ${PHI_SOURCE} ${PHI_HEADERS} ${IMGUI_SOURCES} ${PARTICLE_EFFECT_EDITOR_SOURCE} ${PARTICLE_EFFECT_EDITOR_HEADER})
target_link_libraries(particle_effect_editor yaml-cpp::yaml-cpp glfw glew ${OPENGL_LIBRARIES} ${GLFW_LIBRARIES})

# PBR material editor
set(PBR_MATERIAL_EDITOR_SOURCE ${CMAKE_SOURCE_DIR}/tools/pbr_material_editor.cpp)
set(PBR_MATERIAL_EDITOR_HEADER ${CMAKE_SOURCE_DIR}/tools/pbr_material_editor.hpp)
add_executable(pbr_material_editor ${PHI_SOURCE} ${PHI_HEADERS} ${IMGUI_SOURCES} ${PBR_MATERIAL_EDITOR_SOURCE} ${PBR_MATERIAL_EDITOR_HEADER})
target_link_libraries(pbr_material_editor yaml-cpp::yaml-cpp glfw glew ${OPENGL_LIBRARIES} ${GLFW_LIBRARIES})

# Voxel map editor
set(VOXEL_MAP_EDITOR_SOURCE ${CMAKE_SOURCE_DIR}/tools/voxel_map_editor.cpp)
set(VOXEL_MAP_EDITOR_HEADER ${CMAKE_SOURCE_DIR}/tools/voxel_map_editor.hpp)
add_executable(voxel_map_editor ${PHI_SOURCE} ${PHI_HEADERS} ${IMGUI_SOURCES} ${VOXEL_MAP_EDITOR_SOURCE} ${VOXEL_MAP_EDITOR_HEADER})
target_link_libraries(voxel_map_editor yaml-cpp::yaml-cpp glfw glew ${OPENGL_LIBRARIES} ${GLFW_LIBRARIES})

# Voxel object editor
set(VOXEL_OBJECT_EDITOR_SOURCE ${CMAKE_SOURCE_DIR}/tools/voxel_object_editor.cpp)
set(VOXEL_OBJECT_EDITOR_HEADER ${CMAKE_SOURCE_DIR}/tools/voxel_object_editor.hpp)
add_executable(voxel_object_editor ${PHI_SOURCE} ${PHI_HEADERS} ${IMGUI_SOURCES} ${VOXEL_OBJECT_EDITOR_SOURCE} ${VOXEL_OBJECT_EDITOR_HEADER})
target_link_libraries(voxel_object_editor yaml-cpp::yaml-cpp glfw glew ${OPENGL_LIBRARIES} ${GLFW_LIBRARIES})


# TEMPLATES


# Template app
set(TEMPLATE_APP_SOURCE ${CMAKE_SOURCE_DIR}/templates/new_app.cpp)
set(TEMPLATE_APP_HEADER ${CMAKE_SOURCE_DIR}/templates/new_app.hpp)
add_executable(new_app ${PHI_SOURCE} ${PHI_HEADERS} ${IMGUI_SOURCES} ${TEMPLATE_APP_SOURCE} ${TEMPLATE_APP_HEADER})
target_link_libraries(new_app yaml-cpp::yaml-cpp glfw glew ${OPENGL_LIBRARIES} ${GLFW_LIBRARIES})

# CPack
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
