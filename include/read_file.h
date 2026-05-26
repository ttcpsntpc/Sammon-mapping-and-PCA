#ifndef READ_FILE_H
#define READ_FILE_H

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include "vertex.h"
using namespace std;

enum SampleType
{
    INT, UNSIGNED_INT, CHAR, UNSIGNED_CHAR, FLOAT, UNSIGNED_SHORT, SHORT
};
enum IndianType
{
    LITTLE, BIG
};

class ReadFile_c
{
private:
    bool ReadDatFile(const char *dat_filename);
    bool ReadVecFile(const char *vec_filename);
    bool ReadInfFile(const char *inf_filename);
    bool ReadRawFile(const char *raw_filename);
    //bool ReadTxtFile(const char *txt_filename);
    template <typename T>
    T ReadAndConvert(std::ifstream& ifs);

public:
    ReadFile_c(const char *filename);
    ~ReadFile_c();

    struct DatData {
        int num;
        int dimension;
        vector<float> data;
    } dat_file;
    struct VecData {
        int resolution[2];
        vector<glm::vec2> data;
        float max_speed;
    } vec_file;

    struct InfData
    {
        int data_resolution[3]; // [width, height, depth]
        SampleType sample_type;
        float voxel_size[3];
        IndianType endian;
        glm::vec3 max, min;
    } inf_data;

    struct RawData
    {
        vector<unsigned char> voxel_data; 
        int size;
        int intensity_counts[256];
    } raw_data;

    // struct TxtData
    // {
    //     int data_resolution;
    //     int dimension;
    //     vector<glm::vec3> weight;
    //     glm::vec3 max, min;
    // } txt_data;

    void readFile(const char *filename);
    int idx(int i, int j, int k) {return k * inf_data.data_resolution[0] * inf_data.data_resolution[1] + j * inf_data.data_resolution[0] + i;}
    int idx(int i, int j) {return j * vec_file.resolution[0] + i;}
    vector<unsigned char> cumulativeDistributionEqualization();
    vector<unsigned char> adaptiveHistogramEqualization(int side);
    vector<unsigned char> CLAHE(int block_edge, float threshold);
};

ReadFile_c::ReadFile_c(const char *filename)
{
    string vec_filename = string(filename) + ".dat";
    ReadDatFile(vec_filename.c_str());
    // string inf_filename = string(filename) + ".inf";
    // string raw_filename = string(filename) + ".raw";
    // ReadInfFile(inf_filename.c_str());
    // ReadRawFile(raw_filename.c_str());
    //ReadTxtFile(txt_filename);
}

ReadFile_c::~ReadFile_c()
{
}

void ReadFile_c::readFile(const char *filename) {
    string vec_filename = string(filename) + ".vec";
    ReadVecFile(vec_filename.c_str());
    // string inf_filename = string(filename) + ".inf";
    // string raw_filename = string(filename) + ".raw";
    // ReadInfFile(inf_filename.c_str());
    // ReadRawFile(raw_filename.c_str());
    //ReadTxtFile(txt_filename);
}

bool ReadFile_c::ReadDatFile(const char *dat_filename) 
{
    ifstream ifs(dat_filename, ios::in);
    if(ifs.fail())
    {
        cout<< "Failed to open dat file." << '\n';
        return false;
    }

    string line, token;

    // 讀取第一行 (num, dimension)
    if(getline(ifs, line)) {
        stringstream ss(line);

        getline(ss, token, ',');
        dat_file.num = stoi(token);

        getline(ss, token);
        dat_file.dimension = stoi(token);
    } else {
        return false;
    }
    
    //讀取剩下資料
    dat_file.data.clear();
    dat_file.data.resize(dat_file.num * dat_file.dimension);
    int index = 0;
    while(getline(ifs, line)) {
        stringstream ss(line);
        
        while(getline(ss, token, ',')) {
            if(index < dat_file.num * dat_file.dimension) {
                dat_file.data[index] = stof(token);
            }
            index++;
        }
    }

    ifs.close();
    return true;
}

bool ReadFile_c::ReadVecFile(const char *vec_filename)
{
    ifstream ifs(vec_filename, ios::in);
    if(ifs.fail())
    {
        cout<< "Failed to open vec file." << '\n';
        return false;
    }

    if (!(ifs >> vec_file.resolution[0] >> vec_file.resolution[1])) {
        cout << "Failed to read resolution." << endl;
        return false;
    }

    int total_vector = vec_file.resolution[0] * vec_file.resolution[1];
    vec_file.data.clear();
    vec_file.data.reserve(total_vector);

    float max_speed = 0;
    float x, y;
    for(int i = 0; i < total_vector; i++) {
        if(ifs >> y >> x) {
            vec_file.data.push_back(glm::vec2(x, y));
            float speed = sqrt(x * x + y * y);
            max_speed = (speed > max_speed) ? speed : max_speed;
        }
        else {
            cout<<"Warning: 資料實際個數和resolution乘積不一致"<<endl;
            return false;
        }
    }
    vec_file.max_speed = max_speed;

    ifs.close();
    cout<<"vector resolution: "<<vec_file.resolution[0]<<" "<<vec_file.resolution[1]<<endl;
    cout<<"max speed: "<<vec_file.max_speed<<endl;
    return true;
}

bool ReadFile_c::ReadInfFile(const char *inf_filename)
{
    ifstream ifs(inf_filename, ios::in);
    if (ifs.fail())
    {
        cout << "Failed to open inf file." << '\n';
        return false;
    }

    string line;
    //string raw_file_name;
    while(getline(ifs, line))
    {
        if(line.empty() || line[0] == '#') continue; // 跳過空行和註解行
        size_t pos = line.find('=');
        if (pos == string::npos) continue; // 沒有等號的行也跳過

        // key跟value改成全部小寫，以免大小寫不一致導致讀取失敗
        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);
        transform(key.begin(), key.end(), key.begin(), ::tolower);
        transform(value.begin(), value.end(), value.begin(), ::tolower);
        
        if(key == "raw-file" || key == "filename")   
        {
            //raw_file_name = value; 
        }
        // resolution的格式不固定，中間用%*c直接跳過
        else if(key.find("resolution") != string::npos)
            sscanf(value.c_str(), "%d%*c%d%*c%d", 
                &inf_data.data_resolution[0], 
                &inf_data.data_resolution[1], 
                &inf_data.data_resolution[2]);
        else if(key.find("sample") != string::npos || key.find("value") != string::npos) {
            if(value.find("int") != string::npos)
                inf_data.sample_type = INT;
            else if(value.find("unsigned") != string::npos) {
                if(value.find("int") != string::npos)
                    inf_data.sample_type = UNSIGNED_INT;
                else if(value.find("char") != string::npos)
                    inf_data.sample_type = UNSIGNED_CHAR;
                else if(value.find("short") != string::npos)
                    inf_data.sample_type = UNSIGNED_SHORT;
            }
            else if(value.find("char") != string::npos)
                inf_data.sample_type = CHAR;
            else if(value.find("float") != string::npos)
                inf_data.sample_type = FLOAT;
            else if(value.find("short") != string::npos)
                inf_data.sample_type = SHORT;
            // 完整型態名稱都沒有就代表是縮寫
            else if(value.find("ui") != string::npos)
                inf_data.sample_type = UNSIGNED_INT;
            else if(value.find("ub") != string::npos)
                inf_data.sample_type = UNSIGNED_CHAR;
            else if(value.find("us") != string::npos)
                inf_data.sample_type = UNSIGNED_SHORT;
            else if(value.find("i") != string::npos)
                inf_data.sample_type = INT;
            else if(value.find("b") != string::npos)
                inf_data.sample_type = CHAR;
            else if(value.find("f") != string::npos)
                inf_data.sample_type = FLOAT;
            else if(value.find("s") != string::npos)
                inf_data.sample_type = SHORT;
        }
        else if(key.find("voxel-size") != string::npos)
            sscanf(value.c_str(), "%f%*c%f%*c%f", 
                &inf_data.voxel_size[0], 
                &inf_data.voxel_size[1], 
                &inf_data.voxel_size[2]);
        else if(key.find("endian") != string::npos) {
            if(value.find("b") != string::npos) 
                inf_data.endian = BIG;
            else 
                inf_data.endian = LITTLE;
        }
        else if(key.find("max") != string::npos)
            sscanf(value.c_str(), "%f%*c%f%*c%f", 
                &inf_data.max.x, 
                &inf_data.max.y, 
                &inf_data.max.z);
        else if(key.find("min") != string::npos)
            sscanf(value.c_str(), "%f%*c%f%*c%f", 
                &inf_data.min.x, 
                &inf_data.min.y, 
                &inf_data.min.z);
    }
    
    return true;
}

template <typename T>
T ReadFile_c::ReadAndConvert(std::ifstream& ifs) {
    T value;
    char buffer[sizeof(T)];
    ifs.read(buffer, sizeof(T));

    // 假設電腦是 Little-Endian
    // 如果檔案宣告為 BIG Endian，就將讀到的 Byte 順序反轉
    if (inf_data.endian == BIG) {
        std::reverse(buffer, buffer + sizeof(T));
    }

    // 用C++ 的 memcpy 處理二進位轉型較安全
    std::memcpy(&value, buffer, sizeof(T));
    return value;
}

bool ReadFile_c::ReadRawFile(const char *raw_filename)
{
    if(raw_filename == nullptr)
    {
        return false;
    }
    ifstream ifs(raw_filename, ios::in | ios::binary);
    if (ifs.fail())
    {
        cout << "Failed to open raw file." << '\n';
        return false;
    }

    int x = inf_data.data_resolution[0];
    int y = inf_data.data_resolution[1];
    int z = inf_data.data_resolution[2];

    raw_data.size = x * y * z; 

    // 讀進的數值都先用float存
    vector<float> temp_float_data;
    temp_float_data.clear();
    temp_float_data.reserve(raw_data.size);
    raw_data.voxel_data.reserve(raw_data.size);

    float minimum = numeric_limits<float>::max();
    float maximum = numeric_limits<float>::lowest();

    // 根據不同inf的資訊決定怎麼讀並算最大最小值
    for(int i = 0; i < raw_data.size; i++) {
        float value;

        switch(inf_data.sample_type) {
            case CHAR:           value = static_cast<float>(ReadAndConvert<char>(ifs)); break;
            case UNSIGNED_CHAR:  value = static_cast<float>(ReadAndConvert<unsigned char>(ifs)); break;
            case SHORT:          value = static_cast<float>(ReadAndConvert<short>(ifs)); break;
            case UNSIGNED_SHORT: value = static_cast<float>(ReadAndConvert<unsigned short>(ifs)); break;
            case INT:            value = static_cast<float>(ReadAndConvert<int>(ifs)); break;
            case UNSIGNED_INT:   value = static_cast<float>(ReadAndConvert<unsigned int>(ifs)); break;
            case FLOAT:          value = ReadAndConvert<float>(ifs); break;
        }

        if(value > maximum) maximum = value;
        if(value < minimum) minimum = value;

        temp_float_data.push_back(value);
    }
    cout<<endl<<maximum<<" "<<minimum<<endl;

    raw_data.voxel_data.clear();

    // 將資料正規化到0~255
    float range = maximum - minimum;
    for(int i = 0; i < raw_data.size; i++) {
        float normalized_data = ((temp_float_data[i] - minimum) / range) * 255.0f;
        if(normalized_data < 0 || normalized_data > 255)
            cout<<"錯了";
        unsigned char data = static_cast<unsigned char>(normalized_data + 0.5f);
        raw_data.voxel_data.push_back(data);
        raw_data.intensity_counts[data]++;
    }


    //測試輸出中間100個
    for(int k = z / 2; k < z / 2 + 1; ++k)
    {
        for(int j = y / 2; j < y / 2 + 5; ++j)
        {
            for(int i = x / 2; i < x / 2 + 20; ++i)
            {
                cout << (int)raw_data.voxel_data[idx(i, j, k)] << " ";
            }
            cout << endl;
        }
        cout << endl;
    }
    
    return true;
}

// 最基本的資料平均方法
vector<unsigned char> ReadFile_c::cumulativeDistributionEqualization() {
    int width = inf_data.data_resolution[0];
    int height = inf_data.data_resolution[1];
    int depth = inf_data.data_resolution[2];
    vector<unsigned char> equalized_data; 
    equalized_data.resize(width * height * depth);
    int count[256] = {0}, sumCount[256] = {0};

    for(int k = 0; k < depth; k++)
        for(int j = 0; j < height; j++)
            for(int i = 0; i < width; i++)
                count[raw_data.voxel_data[idx(i, j, k)]]++;
    
    for(int i = 1; i < 256; i++)
        sumCount[i] += sumCount[i - 1] + count[i - 1];
    for(int i = 1; i < 256; i++)
        sumCount[i] = sumCount[i] / (float)raw_data.size * 255.0f + 0.4f;
        
    for(int k = 0; k < depth; k++)
        for(int j = 0; j < height; j++)
            for(int i = 0; i < width; i++)
                equalized_data[idx(i, j, k)] = sumCount[raw_data.voxel_data[idx(i, j, k)]];
    cout<<"CDE finished！"<<endl;
    return equalized_data;
}

// 用積分圖儲存每個體素累積起來的各個硬度值個數，更新硬度值時直接從積分圖取值做運算即可，都是以目前體素當作中心擴展直方圖統計範圍，因此遇邊緣時滑動視窗會被刪減
// 需注意記憶體大小 Ex: 256*256*256*256*4bytes ~= 17.18GB
vector<unsigned char> ReadFile_c::adaptiveHistogramEqualization(int side) { // side代表正方體的邊長，需設成奇數
    int offsets = side / 2;
    int width = inf_data.data_resolution[0];
    int height = inf_data.data_resolution[1];
    int depth = inf_data.data_resolution[2];

    vector<unsigned char> equalized_data;
    equalized_data.resize(width * height * depth);

    vector<vector<int>> integral_data;
    integral_data.resize(width * height * depth, vector<int>(256, 0));
    
    // 1-1: 初始化，將每個 voxel 的原始數值存入對應的 bin
    for(int k = 0; k < depth; k++) {
        for(int j = 0; j < height; j++) {
            for(int i = 0; i < width; i++) {
                int val = raw_data.voxel_data[idx(i, j, k)];
                integral_data[idx(i, j, k)][val] = 1;
            }
        }
    }

    // 對 X 軸累加
    for(int k = 0; k < depth; k++) {
        for(int j = 0; j < height; j++) {
            for(int i = 1; i < width; i++) {
                for(int b = 0; b < 256; b++) {
                    integral_data[idx(i, j, k)][b] += integral_data[idx(i - 1, j, k)][b];
                }
            }
        }
    }
    // 對 Y 軸累加
    for(int k = 0; k < depth; k++) {
        for(int j = 1; j < height; j++) {
            for(int i = 0; i < width; i++) {
                for(int b = 0; b < 256; b++) {
                    integral_data[idx(i, j, k)][b] += integral_data[idx(i, j - 1, k)][b];
                }
            }
        }
    }
    // 對 Z 軸累加
    for(int k = 1; k < depth; k++) {
        for(int j = 0; j < height; j++) {
            for(int i = 0; i < width; i++) {
                for(int b = 0; b < 256; b++) {
                    integral_data[idx(i, j, k)][b] += integral_data[idx(i, j, k - 1)][b];
                }
            }
        }
    }

    // 輔助 Lambda 函式：安全讀取積分圖，超出邊界時自動截斷，避免 Out of bounds
    auto getIntegral = [&](int x, int y, int z, int b) -> int {
        x = std::max(0, std::min(x, width - 1));
        y = std::max(0, std::min(y, height - 1));
        z = std::max(0, std::min(z, depth - 1));
        return integral_data[idx(x, y, z)][b];
    };

    // 利用積分圖進行局部直方圖等化
    for(int k = 0; k < depth; k++) {
        for(int j = 0; j < height; j++) {
            for(int i = 0; i < width; i++) {
                
                // 計算局部視窗的 XYZ 範圍 (包含邊界外推)
                int x_max = i + offsets;
                int y_max = j + offsets;
                int z_max = k + offsets;
                
                // 積分圖的特性，計算下限時需要多減去 1
                int x_min = i - offsets - 1;
                int y_min = j - offsets - 1;
                int z_min = k - offsets - 1;

                int target_val = raw_data.voxel_data[idx(i, j, k)];
                int sumCount = 0;

                for(int b = 0; b <= target_val; b++) {
                    int count_in_window = 
                          getIntegral(x_max, y_max, z_max, b)
                        - getIntegral(x_min, y_max, z_max, b)
                        - getIntegral(x_max, y_min, z_max, b)
                        - getIntegral(x_max, y_max, z_min, b)
                        + getIntegral(x_min, y_min, z_max, b)
                        + getIntegral(x_min, y_max, z_min, b)
                        + getIntegral(x_max, y_min, z_min, b)
                        - getIntegral(x_min, y_min, z_min, b);
                    
                    sumCount += count_in_window;
                }

                // 計算該視窗實際涵蓋的 Voxel 總數 (需考量邊界處視窗會被截斷)
                int actual_x_min = std::max(0, i - offsets);
                int actual_x_max = std::min(width - 1, i + offsets);
                int actual_y_min = std::max(0, j - offsets);
                int actual_y_max = std::min(height - 1, j + offsets);
                int actual_z_min = std::max(0, k - offsets);
                int actual_z_max = std::min(depth - 1, k + offsets);
                
                float windowVolume = (actual_x_max - actual_x_min + 1) * (actual_y_max - actual_y_min + 1) * (actual_z_max - actual_z_min + 1);

                // 計算新的灰階值
                float equalized_val = ((float)sumCount * 255.0f) / windowVolume;
                
                // 限制在 0~255 範圍內
                equalized_data[idx(i, j, k)] = (unsigned char)std::max(0.0f, std::min(255.0f, equalized_val));
            }
        }
    }
    cout<<"AHE finished！"<<endl;
    return equalized_data;
}

vector<unsigned char> ReadFile_c::CLAHE(int block_edge, float alpha) {
    // 取得資料維度
    int width = inf_data.data_resolution[0];
    int height = inf_data.data_resolution[1];
    int depth = inf_data.data_resolution[2];

    int block_num[3] = {width / block_edge, height / block_edge, depth / block_edge}; // 幾個block
    
    int total_blocks = block_num[0] * block_num[1] * block_num[2];
    std::vector<int> tiles_cdf(total_blocks * 256, 0); // 紀錄每個block的cdf

    // 輔助函式：計算 4D index
    auto getTileIdx = [&](int i, int j, int k, int bin) -> int {
        return ((k * block_num[1] * block_num[0]) + (j * block_num[0]) + i) * 256 + bin;
    };

    // 計算 Threshold
    int block_size = block_edge * block_edge * block_edge;
    int average = block_size / 256;
    int threshold = average * alpha;
    
    if (threshold < average) threshold = average;

    // 階段一：計算每個block的CDF
    for(int k = 0; k < block_num[2]; k++) {
        for(int j = 0; j < block_num[1]; j++) {
            for(int i = 0; i < block_num[0]; i++) {
                
                // 1. 統計該區塊的直方圖
                int tile_offset = getTileIdx(i, j, k, 0); // 計算該區塊在cdf table的起始index
                // 計算該區塊在體素資料的起始index
                int start_x = i * block_edge;
                int start_y = j * block_edge;
                int start_z = k * block_edge;

                for(int z = start_z; z < start_z + block_edge; z++) {
                    for(int y = start_y; y < start_y + block_edge; y++) {
                        for(int x = start_x; x < start_x + block_edge; x++) {
                            int val = raw_data.voxel_data[idx(x, y, z)];
                            tiles_cdf[tile_offset + val]++;
                        }
                    }
                }

                // 2. 直方圖超出threshold的部分做Clipping
                bool needs_clipping = true;
                while (needs_clipping) {
                    int excess = 0;
                    
                    // 算多出多少
                    for (int b = 0; b < 256; b++) {
                        if (tiles_cdf[tile_offset + b] > threshold) {
                            excess += tiles_cdf[tile_offset + b] - threshold;
                            tiles_cdf[tile_offset + b] = threshold;
                        }
                    }

                    // 如果有多出來的，就重新分配
                    if (excess > 255) {
                        int step = excess / 256;
                        int rem = excess % 256;
                        
                        // 平均分配 Step
                        for (int b = 0; b < 256; b++) {
                            tiles_cdf[tile_offset + b] += step;
                        }
                        
                        // 分配餘數 (平均打散)
                        int b = 0;
                        while(rem > 0) {
                            tiles_cdf[tile_offset + b] += 1;
                            b++;
                            rem--;
                        }
                        // 重新跑一次 while 迴圈檢查是否有新的超出
                    } else {
                        // 沒有多出很多就跳出迴圈
                        needs_clipping = false; 
                    }
                }

                // 3. 計算累積分布函數 (CDF) 並轉換為 Mapping Table (LUT)
                int sum = 0;
                for (int b = 0; b < 256; b++) {
                    sum += tiles_cdf[tile_offset + b];
                    // 將 CDF 映射到 0~255 的灰階值
                    // 由於可能有些微精度誤差，強制約束在 0~255
                    int mapped_val = (sum * 255) / block_size;
                    tiles_cdf[tile_offset + b] = std::max(0, std::min(255, mapped_val));
                }
            }
        }
    }

    // 階段二：藉由鄰居跟映射表更新硬度值
    vector<unsigned char> equalized_data(width * height * depth);

    for(int z = 0; z < depth; z++) {
        for(int y = 0; y < height; y++) {
            for(int x = 0; x < width; x++) {
                
                int val = raw_data.voxel_data[idx(x, y, z)];

                // 計算當前 Voxel 相對的「區塊小數座標」(找出它在哪些區塊中心之間)
                // -0.5 是因為區塊中心是在 block_edge 的一半
                float tx = ((float)x / block_edge) - 0.5f;
                float ty = ((float)y / block_edge) - 0.5f;
                float tz = ((float)z / block_edge) - 0.5f;

                // 找出左下前 (x1, y1, z1) 與 右上後 (x2, y2, z2) 的區塊 Index
                int x1 = std::max(0, (int)std::floor(tx));
                int y1 = std::max(0, (int)std::floor(ty));
                int z1 = std::max(0, (int)std::floor(tz));

                int x2 = std::min(block_num[0] - 1, x1 + 1);
                int y2 = std::min(block_num[1] - 1, y1 + 1);
                int z2 = std::min(block_num[2] - 1, z1 + 1);

                // 計算距離比例 (用於內插權重)
                float px = tx - std::floor(tx);
                float py = ty - std::floor(ty);
                float pz = tz - std::floor(tz);

                // 防呆：如果是邊緣的體素，權重設為 0，直接使用邊界區塊的數值
                if (x < block_edge / 2 || x >= width - block_edge / 2) px = 0.0f;
                if (y < block_edge / 2 || y >= height - block_edge / 2) py = 0.0f;
                if (z < block_edge / 2 || z >= depth - block_edge / 2) pz = 0.0f;

                // 從 8 個相鄰區塊的 CDF 中查出對應的轉換值
                float c000 = tiles_cdf[getTileIdx(x1, y1, z1, val)];
                float c100 = tiles_cdf[getTileIdx(x2, y1, z1, val)];
                float c010 = tiles_cdf[getTileIdx(x1, y2, z1, val)];
                float c110 = tiles_cdf[getTileIdx(x2, y2, z1, val)];
                float c001 = tiles_cdf[getTileIdx(x1, y1, z2, val)];
                float c101 = tiles_cdf[getTileIdx(x2, y1, z2, val)];
                float c011 = tiles_cdf[getTileIdx(x1, y2, z2, val)];
                float c111 = tiles_cdf[getTileIdx(x2, y2, z2, val)];

                // X 軸方向內插
                float c00 = c000 * (1 - px) + c100 * px;
                float c10 = c010 * (1 - px) + c110 * px;
                float c01 = c001 * (1 - px) + c101 * px;
                float c11 = c011 * (1 - px) + c111 * px;

                // Y 軸方向內插
                float c0 = c00 * (1 - py) + c10 * py;
                float c1 = c01 * (1 - py) + c11 * py;

                // Z 軸方向內插得到最終結果
                float final_val = c0 * (1 - pz) + c1 * pz;

                equalized_data[idx(x, y, z)] = (unsigned char)std::max(0.0f, std::min(255.0f, final_val));
            }
        }
    }
    cout<<"CLAHE finished！"<<endl;
    return equalized_data;
}
/*
bool ReadFile_c::ReadTxtFile(const char *txt_filename)
{
    ifstream ifs(txt_filename, ios::in);
    if (ifs.fail())
    {
        cout << "Failed to open file." << '\n';
        return false;
    }

    string my_line;

    getline(ifs, my_line, ' ');
    txt_data.data_resolution = stoi(my_line);
    getline(ifs, my_line, '\n');
    txt_data.dimension = stoi(my_line);

    glm::vec3 max_temp = glm::vec3(0.0f, 0.0f, 0.0f), min_temp = glm::vec3(9999.9f, 9999.9f, 9999.9f);
    for (int i = 0; i < txt_data.data_resolution; i++)
    {
        glm::vec3 temp;
        getline(ifs, my_line, ' ');
        temp.x = stof(my_line);
        if(temp.x > max_temp.x) max_temp.x = temp.x;
        else if (temp.x < min_temp.x) min_temp.x = temp.x;
        getline(ifs, my_line, ' ');
        temp.y = stof(my_line);
        if(temp.y > max_temp.y) max_temp.y = temp.y;
        else if (temp.y < min_temp.y) min_temp.y = temp.y;
        getline(ifs, my_line, ' ');
        temp.z = stof(my_line);
        if(temp.z > max_temp.z) max_temp.z = temp.z;
        else if (temp.z < min_temp.z) min_temp.z = temp.z;
        txt_data.weight.push_back(temp);
    }
    txt_data.max = max_temp;
    txt_data.min = min_temp;

    return true;
}
*/
#endif