if (glm_FOUND)
    return()
endif()

FetchContent_Declare(
    glm
    URL "https://github.com/g-truc/glm/releases/download/1.0.3/glm-1.0.3.7z"
)
FetchContent_MakeAvailable(glm)