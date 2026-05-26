#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Object.h"
#include "Shader.h"
#include "vertex.h"
#include "texture.h"
#include "camera.h"
#include "read_file.h"
#include "UIManager.h"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <math.h>
#include <vector>
#include <algorithm>
#include <thread>
#include <random>

using namespace std;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow *window);

int Create3DVoxelTexture();

// settings
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 750;

const int DOMAIN_WIDTH = 128; // 白色背景大小
const int DOMAIN_HEIGHT = 128;
const int DOMAIN_START_X = 20; // 白色背景offset
const int DOMAIN_START_Y = 20;

// Camera
Camera_c camera(glm::vec3(50.0f, 50.0f, 200.0f));
glm::vec3 lightPos(500.0f, 500.0f, 500.0f);
bool moveObject = 0; // 在移動光源或是相機
bool isDataRefreshing = 0;

// timing
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f;

ReadFile_c rf("../../creditcard");
UIManager UI;

int N = 100;
vector<float> randomSelete(int N) {
    vector<float> input_data(N * rf.dat_file.dimension);
    
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> distrib(0, rf.dat_file.num - 1);

    vector<int> gen_number; // 紀錄隨機生成的編號
    int count = 0;
    while(gen_number.size() < N) {
        int temp = distrib(gen);

        if(find(gen_number.begin(), gen_number.end(), temp) == gen_number.end()) {
            gen_number.push_back(temp);
            for(int i = 0; i < rf.dat_file.dimension; i++) {
                input_data[count++] = rf.dat_file.data[temp * rf.dat_file.dimension + i];
            }
        }
    }
    return input_data;
}

vector<Vertex_c> sammonMapping(vector<float> input_data) {
    vector<Vertex_c> vertex;

    // preprocessing (d'_ij)
    float sum_distance = 0;
    vector<vector<float>> distance_matrix(N, vector<float>(N)); // 算第i筆跟第j筆資料的距離
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            if(i == j) continue;

            float distance = 0;
            for(int k = 0; k < rf.dat_file.dimension - 1; k++) {
                distance += pow(input_data[i * rf.dat_file.dimension + k] - input_data[j * rf.dat_file.dimension + k], 2);
            }
            distance = sqrt(distance);
            distance_matrix[i][j] = distance;
            sum_distance += distance;
        }
    }

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<float> distrib(0, 1);
    
    vector<vector<float>> points(N, vector<float>(2)); // random給mapping後的初始位置
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < 2; j++){
            points[i][j] = distrib(gen);
        }
    }

    // gradient descent
    float threshold = 0.01, error = threshold + 1.0f, last_error = error + 1;
    float learning_rate = 0.3;
    int iter = 0;
    while(error > threshold && abs(last_error - error) > 1e-6) {
        last_error = error;
        error = 0.0f;
        for(int i = 0; i < N; i++) {
            for(int j = 0; j < N; j++) {
                if(i == j)  continue;

                float new_distance = 0;
                
                // 算new distance (d_ij)
                for(int k = 0; k < 2; k++) { 
                    new_distance += pow(points[i][k] - points[j][k], 2);
                }
                new_distance = sqrt(new_distance);
                if(new_distance < 0.0001) new_distance = 0.0001;

                // updata position
                float delta[2];
                for(int k = 0; k < 2; k++) {
                    delta[k] = learning_rate * (distance_matrix[i][j] - new_distance) / new_distance * (points[i][k] - points[j][k]);

                    points[i][k] += delta[k];
                    points[j][k] -= delta[k];
                }

                // 累計error
                error += pow(distance_matrix[i][j] - new_distance, 2) / new_distance;
            }
        }
        error /= sum_distance;
        learning_rate *= 0.95;
        iter++;
    }
    cout<<"iteration: "<<iter<<endl;

    for(int i = 0; i < N; i++) {
        if(input_data[i * rf.dat_file.dimension + rf.dat_file.dimension - 1] == 1)
            vertex.push_back(Vertex_c{{points[i][0] * DOMAIN_WIDTH + DOMAIN_START_X, points[i][1] * DOMAIN_HEIGHT + DOMAIN_START_Y, 1.0}, {1.0f, 0.0f, 0.0f}, {}, {}});
        else
            vertex.push_back(Vertex_c{{points[i][0] * DOMAIN_WIDTH + DOMAIN_START_X, points[i][1] * DOMAIN_HEIGHT + DOMAIN_START_Y, 1.0}, {0.0f, 0.0f, 1.0f}, {}, {}});
    }

    return vertex;
}



int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    // Setup Dear ImGui style  
    ImGui::StyleColorsDark();
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    Shader_c shader("shader/shader.vs", "shader/shader.fs");
    Shader_c light_shader("shader/light_shader.vs", "shader/light_shader.fs");

    vertex.CreateVertices();
    Object_c square_white;
    square_white.CreateObject(vertices[0], {});
    Object_c square_blue;
    square_blue.CreateObject(vertices[1], {});
    Object_c cube;
    cube.CreateObject(vertices[2], {});
    Object_c axis;  
    axis.CreateObject(vertices[3], {});
    Object_c light_cube;
    light_cube.CreateObject(vertices[4], {});
    
    UI.init();
    
    vector<float> input_data = randomSelete(N);
    vector<Vertex_c> data_points = sammonMapping(input_data); 
    Object_c sammon_points;
    sammon_points.CreateObject(data_points, {});
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui::NewFrame();
        UI.render(lightPos, camera.Position);

        if(isDataRefreshing) {
            input_data = randomSelete(N);
            data_points = sammonMapping(input_data); 
            sammon_points.RenewObject(data_points);
            isDataRefreshing = false;
        }
        // create model matrix
        glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        // create view matrix
        glm::mat4 view = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        // create projection matrix
        glm::mat4 projection = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first

        // active shader for shading
        shader.use();

        view = camera.GetViewMatrix();
        projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setVec3("lightPos", lightPos);
        shader.setVec3("viewPos", camera.Position);

        glBindVertexArray(cube.VAO_);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(10.0f, 10.0f, 10.0f));
        model = glm::scale(model, glm::vec3(10, 10, 10));
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, cube.size);


        // active shader for background
        light_shader.use();
        light_shader.setVec3("lightPos", lightPos);
        light_shader.setMat4("view", view);
        light_shader.setMat4("projection", projection);
        
        // draw the white background
        glBindVertexArray(square_white.VAO_);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(DOMAIN_START_X, DOMAIN_START_Y, 0.0f));
        model = glm::scale(model, glm::vec3(DOMAIN_WIDTH, DOMAIN_HEIGHT, 1.0f));
        light_shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, square_white.size);

        // draw the sammon mapping results
        glBindVertexArray(sammon_points.VAO_);
        model = glm::mat4(1.0f);
        light_shader.setMat4("model", model);
        glPointSize(5.0f);
        glDrawArrays(GL_POINTS, 0, sammon_points.size);

        // draw the light
        glBindVertexArray(light_cube.VAO_);
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(10, 10, 10));
        light_shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, light_cube.size);
        
        // draw the 3 axis
        glBindVertexArray(axis.VAO_);
        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(100, 100, 100));
        light_shader.setMat4("model", model);
        glDrawArrays(GL_LINES, 0, axis.size);

        // ImGui render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
        moveObject = !moveObject;
    }
    if(key == GLFW_KEY_F && action == GLFW_PRESS) {
        isDataRefreshing = !isDataRefreshing;
    }
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if(moveObject == 0)
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FRONT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACK, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            camera.ProcessKeyboard(UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
            camera.ProcessKeyboard(DOWN, deltaTime);
    }
    else
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            lightPos.z -= 100.0f * deltaTime; 
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            lightPos.z += 100.0f * deltaTime; 
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            lightPos.x -= 100.0f * deltaTime; 
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            lightPos.x += 100.0f * deltaTime; 
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            lightPos.y += 100.0f * deltaTime; 
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
            lightPos.y -= 100.0f * deltaTime; 
    }

    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        camera.ProcessKeyboard(PITCHUP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
        camera.ProcessKeyboard(PITCHDOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
        camera.ProcessKeyboard(YAWLEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        camera.ProcessKeyboard(YAWRIGHT, deltaTime);

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

int Create3DVoxelTexture(vector<unsigned char> voxel_data)
{
    int x = rf.inf_data.data_resolution[0]; 
    int y = rf.inf_data.data_resolution[1];
    int z = rf.inf_data.data_resolution[2]; 

    unsigned char *texture_data = new unsigned char[x * y * z * 4];

    for(int k = 0; k < z; k++)         
        for(int j = 0; j < y; j++)
            for(int i = 0; i < x; i++)  
            {
                int idx = (k * y * x + j * x + i) * 4;
                
                // gradient x
                if(i == 0)
                    texture_data[idx + 0] = (voxel_data[rf.idx(i + 1, j, k)] - voxel_data[rf.idx(i, j, k)]) / rf.inf_data.voxel_size[0];
                else if(i == x - 1)
                    texture_data[idx + 0] = (voxel_data[rf.idx(i, j, k)] - voxel_data[rf.idx(i - 1, j, k)]) / rf.inf_data.voxel_size[0];
                else 
                    texture_data[idx + 0] = (voxel_data[rf.idx(i + 1, j, k)] - voxel_data[rf.idx(i - 1, j, k)]) / (2 * rf.inf_data.voxel_size[0]);

                // gradient y
                if(j == 0)
                    texture_data[idx + 1] = (voxel_data[rf.idx(i, j + 1, k)] - voxel_data[rf.idx(i, j, k)]) / rf.inf_data.voxel_size[1];
                else if(j == y - 1)
                    texture_data[idx + 1] = (voxel_data[rf.idx(i, j, k)] - voxel_data[rf.idx(i, j - 1, k)]) / rf.inf_data.voxel_size[1];
                else 
                    texture_data[idx + 1] = (voxel_data[rf.idx(i, j + 1, k)] - voxel_data[rf.idx(i, j - 1, k)]) / (2 * rf.inf_data.voxel_size[1]);

                // gradient z 
                if(k == 0)
                    texture_data[idx + 2] = (voxel_data[rf.idx(i, j, k + 1)] - voxel_data[rf.idx(i, j, k)]) / rf.inf_data.voxel_size[2];
                else if(k == z - 1)
                    texture_data[idx + 2] = (voxel_data[rf.idx(i, j, k)] - voxel_data[rf.idx(i, j, k - 1)]) / rf.inf_data.voxel_size[2];
                else 
                    texture_data[idx + 2] = (voxel_data[rf.idx(i, j, k + 1)] - voxel_data[rf.idx(i, j, k - 1)]) / (2 * rf.inf_data.voxel_size[2]);

                // intensity
                texture_data[idx + 3] = voxel_data[rf.idx(i, j, k)];
            }

    unsigned int texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_3D, texID);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 注意這裡 x 和 z 換位後也要改
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, x, y, z, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);

    return texID;
}