#include "UIManager.h"
#include <algorithm>
// #include <GL/glew.h>

UIManager::UIManager() {}
UIManager::~UIManager() {
    // if (m_TFTextureID != 0) glDeleteTextures(1, &m_TFTextureID);
}

void UIManager::init() {
    initTF();
}

void UIManager::initTF() {
    // 為四個通道都加上起點和終點 (避免空白)
    for (int i = 0; i < 4; ++i) {
        m_channels[i].push_back({ImVec2(0.0f, 0.0f)});
        m_channels[i].push_back({ImVec2(1.0f, 1.0f)});
    }

    glGenTextures(1, &m_TFTextureID);
    glBindTexture(GL_TEXTURE_1D, m_TFTextureID);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

// 這是主程式唯一需要呼叫的 UI 函式
void UIManager::render(glm::vec3 light_pos, glm::vec3 camera_pos) {
    // 綁定外部變數供內部面板使用
    this->light_pos = light_pos;
    this->camera_pos = camera_pos;

    drawMainMenuBar();

    if (m_showTFEditor) {
        drawTransferFunctionEditor();
    }

    if(m_showOtherInfo) {
        drawOtherInfo();
    }

    if (m_showDemoWindow) {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }
}

void UIManager::drawMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Windows")) {
            ImGui::MenuItem("Transfer Function", NULL, &m_showTFEditor);
            ImGui::MenuItem("Other info", NULL, &m_showOtherInfo);
            ImGui::MenuItem("ImGui Demo", NULL, &m_showDemoWindow);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void UIManager::drawOtherInfo() {
    ImGui::Begin("Other Information", &m_showOtherInfo);
    ImGui::InputText("File Path", filename, IM_ARRAYSIZE(filename));
    if(ImGui::Button("load file")) {
        isFileUpdata = true;
    }
    // 使用指標來讀寫光線位置
    ImGui::Text("Light position: (%.1f, %.1f, %.1f)", light_pos.x, light_pos.y, light_pos.z);
    ImGui::Text("Eye position: (%.1f, %.1f, %.1f)", camera_pos.x, camera_pos.y, camera_pos.z);
    ImGui::RadioButton("Original", &data_normalization, 0); ImGui::SameLine();
    ImGui::RadioButton("CDE", &data_normalization, 1); ImGui::SameLine();
    ImGui::RadioButton("AHE", &data_normalization, 2); ImGui::SameLine();
    ImGui::RadioButton("CLAHE", &data_normalization, 3);

    ImGui::End();
}

void UIManager::drawTransferFunctionEditor() {
    ImGui::Begin("Transfer Function Editor", &m_showTFEditor); // 傳入開關指標，右上角會有 'X' 關閉按鈕

    // 設定目前正在編輯哪個通道
    ImGui::RadioButton("Red", &m_currentChannel, 0); ImGui::SameLine();
    ImGui::RadioButton("Green", &m_currentChannel, 1); ImGui::SameLine();
    ImGui::RadioButton("Blue", &m_currentChannel, 2); ImGui::SameLine();
    ImGui::RadioButton("Alpha", &m_currentChannel, 3);

    // 定義畫布大小
    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
    ImVec2 canvas_sz(400, 256);
    ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    // 畫畫布背景 (深灰色)
    draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(40, 40, 40, 255));
    draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

    // 建立一個隱形的按鈕覆蓋在畫布上，用來捕捉滑鼠事件
    ImGui::InvisibleButton("canvas", canvas_sz);
    bool is_hovered = ImGui::IsItemHovered(); // 是否停在物件上
    bool is_active = ImGui::IsItemActive();   // 是否在操作物件(按住)
    ImVec2 mouse_pos = ImGui::GetIO().MousePos;

    // 將 ImVec2(0~1, 0~1) 轉換為折線於螢幕的實際座標
    auto PointToScreen = [&](ImVec2 p) { return ImVec2(canvas_p0.x + p.x * canvas_sz.x, canvas_p1.y - p.y * canvas_sz.y); };
    // 將螢幕實際座標轉換回 ImVec2(0~1, 0~1)
    auto ScreenToPoint = [&](ImVec2 p) { return ImVec2((p.x - canvas_p0.x) / canvas_sz.x, (canvas_p1.y - p.y) / canvas_sz.y); };

    auto& pts = m_channels[m_currentChannel];
    bool modified = false;

    // 左鍵新增點或準備拖曳點
    if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        int hovered_idx = -1;
        for (int i = 0; i < pts.size(); ++i) {
            ImVec2 sp = PointToScreen(pts[i].pos);
            if ((mouse_pos.x - sp.x)*(mouse_pos.x - sp.x) + (mouse_pos.y - sp.y)*(mouse_pos.y - sp.y) < 64.0f) { hovered_idx = i; break; }
        }
        if (hovered_idx != -1) m_draggingIdx = hovered_idx;
        else { pts.push_back({ScreenToPoint(mouse_pos)}); std::sort(pts.begin(), pts.end()); modified = true; }
    }

    // 右鍵刪除點
    if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        for (int i = 0; i < pts.size(); ++i) {
            ImVec2 sp = PointToScreen(pts[i].pos);
            if ((mouse_pos.x - sp.x)*(mouse_pos.x - sp.x) + (mouse_pos.y - sp.y)*(mouse_pos.y - sp.y) < 64.0f) {
                if (pts.size() > 2) { pts.erase(pts.begin() + i); modified = true; }
                break;
            }
        }
    }

    // 正在拖曳點
    if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && m_draggingIdx != -1) {
        ImVec2 new_pos = ScreenToPoint(mouse_pos);
        if(m_draggingIdx == 0) new_pos.x = std::max(0.0f, std::min(pts[m_draggingIdx + 1].pos.x, new_pos.x));
        else if(m_draggingIdx == pts.size() - 1) new_pos.x = std::min(1.0f, std::max(pts[m_draggingIdx - 1].pos.x, new_pos.x));
        else new_pos.x = std::max(pts[m_draggingIdx - 1].pos.x, std::min(pts[m_draggingIdx + 1].pos.x, new_pos.x));
        new_pos.y = std::max(0.0f, std::min(1.0f, new_pos.y));
        pts[m_draggingIdx].pos = new_pos;
        std::sort(pts.begin(), pts.end());
        modified = true;
    }

    // 放開滑鼠取消拖曳
    if (!ImGui::IsMouseDown(0)) m_draggingIdx = -1;

     // --- 開始畫圖 ---
    ImU32 colors[4] = { // 用一個unsigned int 存 RGBA
        IM_COL32(255, 50, 50, 255), // Red
        IM_COL32(50, 255, 50, 255), // Green
        IM_COL32(50, 50, 255, 255), // Blue
        IM_COL32(200, 200, 200, 255)// Alpha
    };

    // 畫出四個通道的折線 (不活耀的通道畫暗一點，活耀的畫亮一點)
    for (int c = 0; c < 4; ++c) {
        ImU32 line_color = colors[c];
        if (c != m_currentChannel) {
            ImVec4 float_color = ImGui::ColorConvertU32ToFloat4(line_color);
            float_color.w *= 0.7f;
            line_color = ImGui::ColorConvertFloat4ToU32(float_color);
        } 
        for (size_t i = 0; i < m_channels[c].size() - 1; ++i) {
            draw_list->AddLine(PointToScreen(m_channels[c][i].pos), PointToScreen(m_channels[c][i+1].pos), line_color, (c == m_currentChannel) ? 2.0f : 1.0f);
        }
    }

    // 畫出目前選擇通道的控制點
    for (size_t i = 0; i < pts.size(); ++i) {
        ImVec2 p = PointToScreen(pts[i].pos);
        // 拖曳的點畫黃色，其他點的畫該通道顏色
        ImU32 circle_color = (m_draggingIdx == i) ? IM_COL32(255, 255, 0, 255) : colors[m_currentChannel];
        draw_list->AddCircleFilled(p, 4.0f, circle_color);
        draw_list->AddCircle(p, 5.0f, IM_COL32(255, 255, 255, 255));
    }

    // 有任何點改變，更新紋理
    if (modified) updateTFTexture();

    ImGui::SliderFloat2("x range", xRange, 0.0f, 1.0f, "%.3f");
    ImGui::SliderFloat2("y range", yRange, 0.0f, 1.0f, "%.3f");
    ImGui::SliderFloat2("z range", zRange, 0.0f, 1.0f, "%.3f");

    ImGui::End();
}

// 取得特定通道在特定位置 x 的強度 (線性插值)
float UIManager::evaluateChannel(int channelIndex, float x) {
    const auto& pts = m_channels[channelIndex];
    if (pts.empty()) return 0.0f;
    if (x <= pts.front().pos.x) return pts.front().pos.y;
    if (x >= pts.back().pos.x) return pts.back().pos.y;
    for (size_t i = 0; i < pts.size() - 1; ++i) {
        if (x >= pts[i].pos.x && x <= pts[i+1].pos.x) {
            float range_x = pts[i+1].pos.x - pts[i].pos.x;
            float t = (x - pts[i].pos.x) / range_x;
            return pts[i].pos.y + t * (pts[i+1].pos.y - pts[i].pos.y);
        }
    }
    return 0.0f;
}

void UIManager::updateTFTexture() {
    const int tf_size = 256;
    unsigned char transfer_function[tf_size][4];
    for (int i = 0; i < tf_size; ++i) { 
        float x = (float)i / 255.0f;
        transfer_function[i][0] = (unsigned char)(evaluateChannel(0, x) * 255.0f);
        transfer_function[i][1] = (unsigned char)(evaluateChannel(1, x) * 255.0f);
        transfer_function[i][2] = (unsigned char)(evaluateChannel(2, x) * 255.0f);
        transfer_function[i][3] = (unsigned char)(evaluateChannel(3, x) * 255.0f); // 注意！不透明度約限制在五以內比較好看
    }
    glBindTexture(GL_TEXTURE_1D, m_TFTextureID);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, tf_size, GL_RGBA, GL_UNSIGNED_BYTE, transfer_function);
}