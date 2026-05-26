cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project("Practice")
find_package(OpenGL REQUIRED)
add_executable(Practice)
add_subdirectory(extern)


set_target_properties( Practice PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_sources( Practice PRIVATE
    "src/main.cpp"
    "src/image.cpp"
    "src/vertex.cpp"
    "src/UIManager.cpp"
)

target_include_directories( Practice PRIVATE
    "include/"
)

target_link_libraries( Practice PRIVATE
    glad # include(BuildGlad.cmake)
    imgui # include(BuildImgui.cmake)
    glm::glm # add_subdirectory(ext/glm)
    glfw  # find_package(glfw3 REQUIRED)
    OpenGL::GL # find_package(OpenGL REQUIRED)
)

add_custom_command( TARGET Practice POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_CURRENT_SOURCE_DIR}/shader"
        "$<TARGET_FILE_DIR:Practice>/shader"
    VERBATIM
)

add_custom_command( TARGET Practice POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_CURRENT_SOURCE_DIR}/picture"
        "$<TARGET_FILE_DIR:Practice>/picture"
    VERBATIM
)