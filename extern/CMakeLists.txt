include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(FETCHCONTENT_QUIET FALSE)

add_subdirectory("glad")
add_subdirectory("imgui")
add_subdirectory("glm")
add_subdirectory("glfw")