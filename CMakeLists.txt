cmake_minimum_required(VERSION 3.1)
project(sdf-studio)

# source and library directories
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor")

# sources and headers
file(GLOB_RECURSE HEADERS "${SOURCE_DIR}/*.h")
file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cpp")

# our executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PRIVATE "${SOURCE_DIR}")
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
add_definitions(-DPROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\" "-w")

# GLM
set (GLM_DIR "${LIB_DIR}/glm")
file(GLOB_RECURSE GLM_SOURCES "${GLM_DIR}/*.cpp")
file(GLOB_RECURSE GLM_HEADERS "${GLM_DIR}/*.hpp")
add_library(glm ${GLM_SOURCES} ${GLM_HEADERS})
target_include_directories(glm PRIVATE "${GLM_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${GLM_DIR}")
target_link_libraries(${PROJECT_NAME} glm)

# Glad
set(GLAD_DIR "${LIB_DIR}/glad")
add_library(glad "${GLAD_DIR}/src/glad.c")
target_include_directories(glad PRIVATE "${GLAD_DIR}/include")
target_include_directories(${PROJECT_NAME} PRIVATE "${GLAD_DIR}/include")
target_link_libraries(${PROJECT_NAME} glad "${CMAKE_DL_LIBS}")

# GLFW
set(GLFW_DIR "${LIB_DIR}/glfw")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
add_subdirectory("${GLFW_DIR}")
target_link_libraries(${PROJECT_NAME} "glfw" "${GLFW_LIBRARIES}")
target_include_directories(${PROJECT_NAME} PRIVATE "${GLFW_DIR}/include")
target_compile_definitions(${PROJECT_NAME} PRIVATE "GLFW_INCLUDE_NONE")

# IMGUI
set(IMGUI_DIR "${LIB_DIR}/imgui")
file(GLOB_RECURSE IMGUI_SOURCES "${IMGUI_DIR}/*.cpp")
file(GLOB_RECURSE IMGUI_HEADERS "${IMGUI_DIR}/*.h")
add_library(imgui ${IMGUI_HEADERS} ${IMGUI_SOURCES})
target_include_directories(imgui PRIVATE "${IMGUI_DIR}")
target_include_directories(imgui PRIVATE "${GLAD_DIR}/include")
target_link_libraries(imgui glfw glad)
target_include_directories(${PROJECT_NAME} PRIVATE "${IMGUI_DIR}")
target_link_libraries(${PROJECT_NAME} imgui)

# ImGui File Dialog
set(IFD_DIR "${LIB_DIR}/ImGuiFileDialog")
file(GLOB_RECURSE IFD_SOURCES "${IFD_DIR}/*.cpp")
file(GLOB_RECURSE IFD_HEADERS "${IFD_DIR}/*.h")
add_library(imgui_filedialog ${IFD_SOURCES} ${IFD_HEADERS})
target_include_directories(imgui_filedialog PRIVATE "${IFD_DIR}")
target_include_directories(imgui_filedialog PRIVATE "${IMGUI_DIR}")
target_link_libraries(imgui_filedialog imgui)
target_include_directories(${PROJECT_NAME} PRIVATE "${IFD_DIR}")
target_link_libraries(${PROJECT_NAME} imgui_filedialog)

# ImGui Editor
set(ITE_DIR "${LIB_DIR}/ImGuiColorTextEdit")
file(GLOB_RECURSE ITE_SOURCES "${ITE_DIR}/*.cpp")
file(GLOB_RECURSE ITE_HEADERS "${ITE_DIR}/*.h")
add_library(imgui_texteditor ${ITE_SOURCES} ${ITE_HEADERS})
target_include_directories(imgui_texteditor PRIVATE "${ITE_DIR}")
target_include_directories(imgui_texteditor PRIVATE "${IMGUI_DIR}")
target_link_libraries(imgui_texteditor imgui)
target_include_directories(${PROJECT_NAME} PRIVATE "${ITE_DIR}")
target_link_libraries(${PROJECT_NAME} imgui_texteditor)

#STB Image Loader
set(STB_DIR "${LIB_DIR}/stb")
target_include_directories(${PROJECT_NAME} PRIVATE "${STB_DIR}")

