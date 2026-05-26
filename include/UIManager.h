#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <vector>

// --- 共用資料結構 ---
struct ControlPoint {
    ImVec2 pos; 
    bool operator<(const ControlPoint& other) const { 
        return pos.x < other.pos.x;
    }
};

class UIManager {
public:
    UIManager();
    ~UIManager();

    bool isFileUpdata = false;
    char filename[50] = "../../vector";

    void init();
    
    // 主渲染入口，每次迴圈呼叫這一個就好
    void render(glm::vec3 light_pos, glm::vec3 camera_pos);

    // --- 給外部主程式取得資料的介面 ---
    float* getXRange() { return xRange; } // ray casting顯示範圍
    float* getYRange() { return yRange; }
    float* getZRange() { return zRange; }
    int getNormalizationMode() { return data_normalization; } // 0: raw data, 1: CDE, 2: CLAHE 
    unsigned int getTFTextureID() const { return m_TFTextureID; }

    // --- Transfer Function ---
    float evaluateChannel(int channelIndex, float x);

private:
    // --- UI 繪製區塊 (把不同功能拆成獨立函式) ---
    void drawMainMenuBar();
    void drawTransferFunctionEditor();
    void drawOtherInfo();

    // --- Transfer Function 內部邏輯 ---
    void initTF();
    void updateTFTexture();

private:
    // --- 視窗開關狀態 ---
    bool m_showTFEditor = false;
    bool m_showDemoWindow = false;
    bool m_showOtherInfo = true;

    // --- Transfer Function 變數 ---
    float xRange[2] = {0.0f, 1.0f};
    float yRange[2] = {0.0f, 1.0f};
    float zRange[2] = {0.0f, 1.0f};
    int data_normalization = 0; 

    std::vector<ControlPoint> m_channels[4];
    unsigned int m_TFTextureID = 0;
    int m_currentChannel = 3; 
    int m_draggingIdx = -1; // 記錄目前正在拖拉哪個點

    // --- 其他全域狀態 (可以用指標或參考從 Render 傳入，這裡暫存用) ---
    glm::vec3 light_pos, camera_pos;
};