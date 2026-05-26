if(glfw3_FOUND) #find package產生的變數 所以要跟find package的名字一樣
    
endif()

FetchContent_Declare(
    glfw3
    URL "https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.zip"
)
FetchContent_MakeAvailable(glfw3)

