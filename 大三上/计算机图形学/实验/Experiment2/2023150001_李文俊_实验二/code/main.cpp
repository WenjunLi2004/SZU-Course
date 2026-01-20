// 实验二：OFF格式三维模型文件的读取与显示（参考实验2.2实现）
// 功能：
// - 解析OFF文件（支持三角面与多边形面片的扇形三角化）
// - 展开面片为绘制顶点数组并上传至GPU
// - 提供多种颜色模式（调色板、按高度渐变、彩虹渐变、统一自定义色）
// - 键盘交互：切换模型、切换颜色、线框模式、深度测试、面剔除等

#include "Angel.h"

#include <vector>
#include <fstream>
#include <string>
#include <random>

using namespace std;

// 三角面片顶点索引
struct vec3i { unsigned int a, b, c; vec3i(unsigned int ia=0, unsigned int ib=0, unsigned int ic=0):a(ia),b(ib),c(ic){} };

// 全局数据容器
static std::vector<glm::vec3> g_vertices;   // 原始顶点列表
static std::vector<vec3i>     g_facesTri;   // 三角面片列表（多边形已三角化）
static std::vector<glm::vec3> g_points;     // 展开后的绘制点序列
static std::vector<glm::vec3> g_colors;     // 与g_points一一对应的颜色

// OpenGL对象
static GLuint g_vao = 0;
static GLuint g_vbo = 0;
static GLuint g_program = 0;
static GLuint g_matrixLocation = 0; // 变换矩阵uniform位置

// 颜色模式：0=调色板, 1=按高度渐变, 2=彩虹渐变, 3=统一自定义色
static int   g_colorMode = 1;
static float g_baseHue   = 0.65f; // 自定义色/渐变基色（0~1），初始为淡青绿

// 旋转动画相关变量
static bool  g_animationEnabled = false; // 是否启用自动旋转动画
static float g_rotationSpeed = 15.0f;    // 旋转速度（度/秒）
static glm::vec3 g_rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // 旋转轴（默认Y轴）
static glm::mat4 g_accumulatedRotation = glm::mat4(1.0f);   // 累积的旋转变换矩阵

// 预置调色板（8色）
static const glm::vec3 kPalette[8] = {
    {1.0f, 1.0f, 1.0f},  // White
    {1.0f, 1.0f, 0.0f},  // Yellow
    {0.0f, 1.0f, 0.0f},  // Green
    {0.0f, 1.0f, 1.0f},  // Cyan
    {1.0f, 0.0f, 1.0f},  // Magenta
    {1.0f, 0.0f, 0.0f},  // Red
    {0.0f, 0.0f, 0.0f},  // Black
    {0.0f, 0.0f, 1.0f}   // Blue
};

// HSV转RGB（S,V范围0..1, H范围0..1）
static glm::vec3 hsv2rgb(float h, float s, float v)
{
    h = fmodf(h, 1.0f);
    float c = v * s;
    float hh = h * 6.0f;
    float x = c * (1.0f - fabsf(fmodf(hh, 2.0f) - 1.0f));
    float r=0, g=0, b=0;
    if      (0.0f <= hh && hh < 1.0f) { r=c; g=x; }
    else if (1.0f <= hh && hh < 2.0f) { r=x; g=c; }
    else if (2.0f <= hh && hh < 3.0f) { g=c; b=x; }
    else if (3.0f <= hh && hh < 4.0f) { g=x; b=c; }
    else if (4.0f <= hh && hh < 5.0f) { r=x; b=c; }
    else                               { r=c; b=x; }
    float m = v - c;
    return glm::vec3(r+m, g+m, b+m);
}

// 读取并解析OFF文件；支持多边形面片的扇形三角化
static bool read_off(const std::string& filename)
{
    if (filename.empty()) return false;
    std::ifstream fin(filename);
    if (!fin) { std::cerr << "无法打开OFF文件: " << filename << std::endl; return false; }

    std::string header; fin >> header; // 一般为"OFF"
    int nVertices=0, nFaces=0, nEdges=0;
    fin >> nVertices >> nFaces >> nEdges;

    g_vertices.clear(); g_facesTri.clear();
    g_vertices.reserve(nVertices);

    for (int i=0;i<nVertices;i++) {
        float x,y,z; fin >> x >> y >> z;
        g_vertices.emplace_back(x,y,z);
    }
    for (int i=0;i<nFaces;i++) {
        int cnt; fin >> cnt;
        if (cnt < 3) { // 非法面片，跳过
            for (int j=0;j<cnt;j++){ int tmp; fin >> tmp; }
            continue;
        }
        std::vector<unsigned int> idx(cnt);
        for (int j=0;j<cnt;j++) fin >> idx[j];
        // 扇形三角化： (0, k, k+1)
        for (int k=1;k<=cnt-2;k++) {
            g_facesTri.emplace_back(idx[0], idx[k], idx[k+1]);
        }
    }
    std::cout << "读取成功：顶点=" << g_vertices.size() << ", 三角面片=" << g_facesTri.size() << std::endl;
    return true;
}

// 为展开顶点生成颜色（根据模式）
static void fill_colors()
{
    
    // 首先为原始顶点生成颜色
    std::vector<glm::vec3> vertex_colors(g_vertices.size());

    if (g_colorMode == 0) {
        // 调色板模式：使用预设的8种颜色
        for (size_t i = 0; i < g_vertices.size(); ++i) {
            vertex_colors[i] = kPalette[i % 8];
        }
    } else if (g_colorMode == 1) {
        // 按高度渐变（多层次版本）
        float minY=FLT_MAX, maxY=-FLT_MAX;
        for (const auto& v : g_vertices) { minY = std::min(minY, v.y); maxY = std::max(maxY, v.y); }
        float range = (maxY - minY);
        if (range < 1e-6f) range = 1.0f;
        
        for (size_t i = 0; i < g_vertices.size(); i++) {
            const auto& v = g_vertices[i];
            float t = (v.y - minY) / range; // 0..1
            
            // 创建多层次的颜色变化
            if (t < 0.33f) {
                // 底部：蓝色到青色
                float local_t = t / 0.33f;
                glm::vec3 c1 = hsv2rgb(fmodf(g_baseHue + 0.6f, 1.0f), 0.8f, 0.8f); // 蓝色
                glm::vec3 c2 = hsv2rgb(fmodf(g_baseHue + 0.5f, 1.0f), 0.7f, 0.9f); // 青色
                vertex_colors[i] = c1*(1.0f - local_t) + c2*local_t;
            } else if (t < 0.66f) {
                // 中部：青色到绿色
                float local_t = (t - 0.33f) / 0.33f;
                glm::vec3 c1 = hsv2rgb(fmodf(g_baseHue + 0.5f, 1.0f), 0.7f, 0.9f); // 青色
                glm::vec3 c2 = hsv2rgb(fmodf(g_baseHue + 0.33f, 1.0f), 0.8f, 0.9f); // 绿色
                vertex_colors[i] = c1*(1.0f - local_t) + c2*local_t;
            } else {
                // 顶部：绿色到黄色/红色
                float local_t = (t - 0.66f) / 0.34f;
                glm::vec3 c1 = hsv2rgb(fmodf(g_baseHue + 0.33f, 1.0f), 0.8f, 0.9f); // 绿色
                glm::vec3 c2 = hsv2rgb(fmodf(g_baseHue + 0.1f, 1.0f), 0.9f, 0.95f); // 黄色/红色
                vertex_colors[i] = c1*(1.0f - local_t) + c2*local_t;
            }
        }
    } else if (g_colorMode == 2) {
        // 彩虹渐变：基于顶点索引的平滑分布
        for (size_t i = 0; i < g_vertices.size(); i++) {
            // 为每个顶点计算一个基础色相
            float h = fmodf((float)i / (float)std::max<size_t>(1, g_vertices.size()) + g_baseHue, 1.0f);
            glm::vec3 col = hsv2rgb(h, 0.7f, 0.9f);
            vertex_colors[i] = col;
        }
    } else if (g_colorMode == 3) {
        // 统一纯白色
        glm::vec3 col = glm::vec3(1.0f, 1.0f, 1.0f);  // 纯白色
        std::fill(vertex_colors.begin(), vertex_colors.end(), col);
    }
    
    // 然后根据面片索引将顶点颜色展开到绘制点
    g_colors.clear();
    g_colors.reserve(g_facesTri.size() * 3);
    for (const auto& face : g_facesTri) {
        g_colors.push_back(vertex_colors[face.a]);
        g_colors.push_back(vertex_colors[face.b]);
        g_colors.push_back(vertex_colors[face.c]);
    }
}

// 根据三角面片展开绘制点
static void store_faces_points()
{
    g_points.clear();
    g_points.reserve(g_facesTri.size()*3);
    for (const auto& f : g_facesTri) {
        g_points.push_back(g_vertices[f.a]);
        g_points.push_back(g_vertices[f.b]);
        g_points.push_back(g_vertices[f.c]);
    }
    
    fill_colors();
}

// 更新GPU缓冲：根据当前g_points/g_colors重新分配并上传
static void upload_buffers()
{
    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    GLsizeiptr ptsBytes = (GLsizeiptr)(g_points.size() * sizeof(glm::vec3));
    GLsizeiptr clrBytes = (GLsizeiptr)(g_colors.size() * sizeof(glm::vec3));
    glBufferData(GL_ARRAY_BUFFER, ptsBytes + clrBytes, NULL, GL_DYNAMIC_DRAW);
    // 位置数据在前，颜色数据在后（标准布局）
    if (!g_points.empty()) glBufferSubData(GL_ARRAY_BUFFER, 0, ptsBytes, g_points.data());
    if (!g_colors.empty()) glBufferSubData(GL_ARRAY_BUFFER, ptsBytes, clrBytes, g_colors.data());
    
    // 重新设置顶点属性指针（因为VBO重新分配后原指针失效）
    GLuint pLocation = glGetAttribLocation(g_program, "vPosition");
    glEnableVertexAttribArray(pLocation);
    glVertexAttribPointer(pLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    GLuint cLocation = glGetAttribLocation(g_program, "vColor");
    glEnableVertexAttribArray(cLocation);
    glVertexAttribPointer(cLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(ptsBytes));
}

static void init()
{
    // 默认读取cow.off
    read_off("./Models/cow.off");
    store_faces_points();

    // VAO
    glGenVertexArrays(1, &g_vao);
    glBindVertexArray(g_vao);

    // VBO
    glGenBuffers(1, &g_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    upload_buffers();

    // 着色器
    g_program = InitShader("shaders/vshader.glsl", "shaders/fshader.glsl");
    glUseProgram(g_program);

    // 顶点属性：位置（在VBO前半段，绑定到着色器中的vPosition）
    GLuint pLocation = glGetAttribLocation(g_program, "vPosition");
    glEnableVertexAttribArray(pLocation);
    glVertexAttribPointer(pLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    // 顶点属性：颜色（在VBO后半段，绑定到着色器中的vColor）
    GLuint cLocation = glGetAttribLocation(g_program, "vColor");
    glEnableVertexAttribArray(cLocation);
    glVertexAttribPointer(cLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(g_points.size() * sizeof(glm::vec3)));

    // 获取变换矩阵uniform位置
    g_matrixLocation = glGetUniformLocation(g_program, "matrix");

    glClearColor(0.06f, 0.06f, 0.08f, 1.0f); // 略带深蓝的背景
}

static void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    static float lastTime = 0.0f;
    static bool wasAnimationEnabled = false;
    
    if (g_animationEnabled) {
        float currentTime = (float)glfwGetTime();
        
        // 如果刚从暂停状态恢复，重置时间基准
        if (!wasAnimationEnabled) {
            lastTime = currentTime;
            wasAnimationEnabled = true;
        }
        
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        
        // 计算这一帧的旋转增量
        float deltaRotation = g_rotationSpeed * deltaTime;
        glm::mat4 deltaMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(deltaRotation), g_rotationAxis);
        
        // 累积旋转变换
        g_accumulatedRotation = deltaMatrix * g_accumulatedRotation;
    } else {
        wasAnimationEnabled = false;
    }
    
    // 传递累积的变换矩阵到着色器
    if (g_matrixLocation != -1) {
        glUniformMatrix4fv(g_matrixLocation, 1, GL_FALSE, glm::value_ptr(g_accumulatedRotation));
    }
    
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)g_points.size());
}

static void cycle_color_mode()
{
    g_colorMode = (g_colorMode + 1) % 4;
    fill_colors();
    upload_buffers();
    std::cout << "颜色模式已切换" << std::endl;
}

static void randomize_base_hue()
{
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    g_baseHue = dist(rng);
    cycle_color_mode(); // 触发一次颜色刷新（保持模式不变）
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_MINUS) {
        // 加载cube.off模型
        std::cout << "加载 cube.off 模型" << std::endl;
        if (read_off("Models/cube.off")) {
            store_faces_points();
            upload_buffers();
        }
    } else if (key == GLFW_KEY_EQUAL) {
        // 加载cow.off模型
        std::cout << "加载 cow.off 模型" << std::endl;
        if (read_off("Models/cow.off")) {
            store_faces_points();
            upload_buffers();
        }
    } else if (key == GLFW_KEY_C) {
        std::cout << "切换颜色模式" << std::endl;
        cycle_color_mode();
    } else if (key == GLFW_KEY_R) {
        std::cout << "随机基色" << std::endl;
        randomize_base_hue();
    } else if (key == GLFW_KEY_0) {
        std::cout << "重置渲染状态" << std::endl;
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else if (key == GLFW_KEY_1 && (mods == 0x0000)) {
        std::cout << "启用深度测试" << std::endl;
        glEnable(GL_DEPTH_TEST);
    } else if (key == GLFW_KEY_1 && (mods == GLFW_MOD_SHIFT)) {
        std::cout << "关闭深度测试" << std::endl;
        glDisable(GL_DEPTH_TEST);
    } else if (key == GLFW_KEY_2 && (mods == 0x0000)) {
        std::cout << "启用反面剔除" << std::endl;
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else if (key == GLFW_KEY_2 && (mods == GLFW_MOD_SHIFT)) {
        std::cout << "关闭反面剔除" << std::endl;
        glDisable(GL_CULL_FACE);
    } else if (key == GLFW_KEY_3 && (mods == 0x0000)) {
        std::cout << "启用正面剔除" << std::endl;
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
    } else if (key == GLFW_KEY_3 && (mods == GLFW_MOD_SHIFT)) {
        std::cout << "关闭正面剔除" << std::endl;
        glDisable(GL_CULL_FACE);
    } else if (key == GLFW_KEY_4 && (mods == 0x0000)) {
        std::cout << "线框模式：开启" << std::endl;
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else if (key == GLFW_KEY_4 && (mods == GLFW_MOD_SHIFT)) {
        std::cout << "线框模式：关闭" << std::endl;
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else if (key == GLFW_KEY_X) {
        g_rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);
        std::cout << "旋转轴：X轴" << std::endl;
    } else if (key == GLFW_KEY_Y) {
        g_rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
        std::cout << "旋转轴：Y轴" << std::endl;
    } else if (key == GLFW_KEY_Z) {
        g_rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);
        std::cout << "旋转轴：Z轴" << std::endl;
    }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            g_animationEnabled = true;
            std::cout << "鼠标左键：开始旋转动画" << std::endl;
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            g_animationEnabled = false;
            std::cout << "鼠标右键：暂停旋转动画" << std::endl;
        }
    }
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main(int argc, char** argv)
{
    // 初始化GLFW
    glfwInit();

    // OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(800, 600, "李文俊-2023150001-实验二 OFF模型读取与显示", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    init();
    while (!glfwWindowShouldClose(window)) {
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    return 0;
}