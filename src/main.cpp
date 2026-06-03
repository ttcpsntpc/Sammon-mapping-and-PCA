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

ReadFile_c rf("../../Iris");
UIManager UI;

bool isAdd = false;
int N = 100;

// SOM parameters
bool start = false;
thread t1;
int total_iteration_count = 200000;
int iteration_count = 0;
int resolution = 5;
double original_radius = resolution / 2.0, original_learning_rate = 0.005; // 鄰域半徑和學習率
double neighbor_radius = original_radius, learning_rate = original_learning_rate;
// 一個resolution * resolution大小的vector，裡面每個元素都是大小為dimension的vector<float>，代表該點的weight
vector<vector<vector<float>>> square_weight(resolution, vector<vector<float>>(resolution, vector<float>(rf.dat_file.dimension)));

vector<float> randomSelete(int N) {
    int N_node = 0;
    if(isAdd == true) N_node = resolution * resolution;

    vector<float> input_data((N + N_node) * rf.dat_file.dimension);
    
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
    if(isAdd == true) {
        for(int i = 0; i < resolution; i++) {
            for(int j = 0; j < resolution; j++) {
                for(int k = 0; k < rf.dat_file.dimension; k++) {
                    input_data[count++] = square_weight[i][j][k];
                }
            }
        }
    }
    return input_data;
}

vector<Vertex_c> sammonMapping(vector<float> input_data, int N) {
    vector<Vertex_c> vertex;

    if(isAdd == true) N += resolution * resolution;

    // preprocessing (d'_ij)
    int zero_count = 0;
    float sum_distance = 0;
    vector<vector<float>> origin_distance(N, vector<float>(N)); // 算第i筆跟第j筆資料的距離
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N, j != i; j++) {
            float distance = 0;
            for(int k = 0; k < rf.dat_file.dimension - 1; k++) { // 2-norm
                distance += pow(input_data[i * rf.dat_file.dimension + k] - input_data[j * rf.dat_file.dimension + k], 2);
            }
            distance = sqrt(distance);
            if(distance == 0) {
                zero_count++;
                distance += 1e-6;
            }
            origin_distance[i][j] = distance;
            sum_distance += distance;
        }
    }

    if(zero_count > 0) cout<<"original distance has "<<zero_count<<" zero distance\n";
    zero_count = 0;

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
    float max_iter = 1000, threshold = 1e-6, error = threshold + 1.0f, last_error = error + 1.0f;
    float learning_rate = 0.3;
    int iter = 0;
    float min[2] = {FLT_MAX, FLT_MAX}, max[2] = {-FLT_MAX, -FLT_MAX};
    
    while(iter < max_iter && abs(last_error - error) / last_error > threshold) {
        last_error = error;
        error = 0.0f;
        for(int i = 0; i < N; i++) {
            for(int j = 0; j < N, j != i; j++) {

                float new_distance = 0;
                
                // 算new distance (d_ij)
                for(int k = 0; k < 2; k++) { 
                    new_distance += pow(points[i][k] - points[j][k], 2);
                }
                new_distance = sqrt(new_distance);
                if(new_distance < 1e-6) {
                    new_distance = 1e-6;
                    zero_count++;
                }

                // updata position
                float delta[2];
                for(int k = 0; k < 2; k++) {
                    delta[k] = learning_rate * (origin_distance[i][j] - new_distance) / new_distance * (points[i][k] - points[j][k]);

                    points[i][k] += delta[k];
                    points[j][k] -= delta[k];
                }

                // 累計error
                error += pow(origin_distance[i][j] - new_distance, 2) / origin_distance[i][j];
            }
        }
        error /= sum_distance;
        learning_rate *= 0.95;
        iter++;
    }
    cout<<"iteration: "<<iter<<endl;
    if(zero_count > 0) cout<<"in the iteration, new distance has "<<zero_count<<" zero distance\n";

    for(int i = 0; i < N; i++) {
        for(int k = 0; k < 2; k++) {
            if(points[i][k] < min[k]) min[k] = points[i][k];
            if(points[i][k] > max[k]) max[k] = points[i][k];
        }
    }

    for(int i = 0; i < N; i++) {
        points[i][0] = (points[i][0] - min[0]) / (max[0] - min[0]) * 0.9 + 0.05;
        points[i][1] = (points[i][1] - min[1]) / (max[1] - min[1]) * 0.9 + 0.05;

        if(input_data[i * rf.dat_file.dimension + rf.dat_file.dimension - 1] == 1)
            vertex.push_back(Vertex_c{{points[i][0], points[i][1], 1.0}, {1.0f, 0.0f, 0.0f}, {}, {}});
        else if(input_data[i * rf.dat_file.dimension + rf.dat_file.dimension - 1] == 0)
            vertex.push_back(Vertex_c{{points[i][0], points[i][1], 1.0}, {0.0f, 0.0f, 1.0f}, {}, {}});
        else if(input_data[i * rf.dat_file.dimension + rf.dat_file.dimension - 1] == 2)
            vertex.push_back(Vertex_c{{points[i][0], points[i][1], 1.0}, {0.0f, 1.0f, 0.0f}, {}, {}});
        else if(input_data[i * rf.dat_file.dimension + rf.dat_file.dimension - 1] == 100)
            vertex.push_back(Vertex_c{{points[i][0], points[i][1], 1.0}, {0.0f, 0.0f, 0.0f}, {}, {}});
        else 
            cout<<"wrong classification type: "<<input_data[i * rf.dat_file.dimension + rf.dat_file.dimension - 1]<<endl;

    }
    return vertex;
}

void initialSquareMeshWeight() {
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<float> distrib(0, 1);

    for(int i = 0; i < resolution; i++) {
        for(int j = 0; j < resolution; j++) {
            for(int k = 0; k < rf.dat_file.dimension - 1; k++) {
                square_weight[i][j][k] = distrib(gen);
            }
            square_weight[i][j][rf.dat_file.dimension - 1] = 100; // 用第一百類代表用SOM算出的點
        }
    }
}

glm::ivec2 FindBMU(vector<float> input_vector) {
    int k = 1;
    float minimum_distance = std::numeric_limits<float>::max();
    glm::ivec2 winner(0, 0);
    for(int i = 0; i < resolution; ++i)
    {
        for(int j = 0; j < resolution; ++j)
        {
            float distance = 0;
            
            for(int k = 0; k < rf.dat_file.dimension - 1; k++) { // 2-norm
                distance += pow(input_vector[i * rf.dat_file.dimension + k] - input_vector[k], 2);
            }
            if (distance < minimum_distance)
            {
                k = 1;
                minimum_distance = distance;
                winner.x = i;
                winner.y = j;
            }
            else if (distance == minimum_distance)
            {
                k++;
                // random number between 0 and 1
                // !!! for c++ 11
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_real_distribution<> dis(0.0, 1.0);
                double random_value = dis(gen);
                if(random_value < 1.0 / k)
                {
                    winner.x = i;
                    winner.y = j;
                }
            }
        }
    }
    return winner;
}

double UpdateNeighbourhoodRadius(double original_radius, int iteration_count, int total_iteration_count) {
    return original_radius * exp( (double)-iteration_count * log(original_radius) / (double)total_iteration_count );
}

double UpdateLearningRate(double original_learning_rate, int iteration_count, int total_iteration_count) {
    return original_learning_rate * exp( -(double)iteration_count / (double)total_iteration_count );
}

double CalculateInfluence(glm::vec2 BMU, glm::vec2 now_point, int resolution, double neighbourhood_radius) {
    double distance = (BMU.x - now_point.x) * (BMU.x - now_point.x) + (BMU.y - now_point.y) * (BMU.y - now_point.y);
    if (distance > neighbourhood_radius * neighbourhood_radius)
        return 0.0f;
    return exp(-(distance / (2 * neighbourhood_radius * neighbourhood_radius)));
}

void RunSOM() {
     // 1. pick a random input vector
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, rf.dat_file.num - 1);
    int random_index = dis(gen);    
    vector<float> input_vector = vector<float>(rf.dat_file.data.begin() + random_index * rf.dat_file.dimension, rf.dat_file.data.begin() + (random_index + 1) * rf.dat_file.dimension);

    // 2. find the best matching unit (BMU)
    glm::ivec2 BMU = FindBMU(input_vector);
    
    // 3. update the radius and learning rate
    neighbor_radius = UpdateNeighbourhoodRadius(original_radius, iteration_count, total_iteration_count);
    learning_rate = UpdateLearningRate(original_learning_rate, iteration_count, total_iteration_count);
    
    for(int j = 0; j < resolution; ++j)
    {
        for(int k = 0; k < resolution; ++k)
        {
            glm::ivec2 now_point(j, k);
            double influence = CalculateInfluence(BMU, now_point, resolution, neighbor_radius);
            glm::ivec2 distance = BMU - now_point;
            // if( influence != 0.0){
            //     cout<<" cylinderical_mesh["<<j<<"]["<<k<<"] influence: "<<influence;
            //     cout<<"  BMU: "<<BMU.x<<", "<<BMU.y<<endl;
            // }
            for(int l = 0; l < rf.dat_file.dimension - 1; l++) {
                square_weight[j][k][l] += (float)influence * (float)learning_rate * (input_vector[l] - square_weight[j][k][l]);
            }
        }
    }
}

void CreateThread() {
    if(t1.joinable())
        t1.join();

    t1 = thread(RunSOM);
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
    
    initialSquareMeshWeight();

    vector<float> input_data = randomSelete(N);
    vector<Vertex_c> data_points = sammonMapping(input_data, N); 
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
        ImGui::Begin("Multi-Dimension Data");
        if(ImGui::InputInt("Resolution", &resolution)) {
            isAdd = false;
            start = false;
            original_radius = resolution / 2.0; 
            neighbor_radius = original_radius;
            original_learning_rate = 0.005;
            learning_rate = original_learning_rate;
            iteration_count = 0;
            square_weight.clear();
            square_weight.resize(resolution, vector<vector<float>>(resolution, vector<float>(rf.dat_file.dimension)));
            initialSquareMeshWeight();
        }
        if (ImGui::Button("Run SOM")) {
            start = true;
            CreateThread();
        }
        ImGui::SameLine();
        ImGui::Text("i: %d", iteration_count);
        ImGui::Text("radius: %f", neighbor_radius);
        ImGui::Text("learning: %f", learning_rate);
        if (ImGui::Button("add SOM nodes to sammon mapping")) {
            isAdd = true;
        }
        ImGui::InputInt("N", &N);
        ImGui::End();

        if (start && iteration_count < total_iteration_count) {
            for(int i = 0; i < 100; ++i) {
                RunSOM();
                iteration_count++;
            }
        }

        if(isDataRefreshing) {
            input_data = randomSelete(N);
            data_points = sammonMapping(input_data, N); 
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
        model = glm::translate(model, glm::vec3(DOMAIN_START_X, DOMAIN_START_Y, 0.0f));
        model = glm::scale(model, glm::vec3(DOMAIN_WIDTH, DOMAIN_HEIGHT, 1.0f));
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