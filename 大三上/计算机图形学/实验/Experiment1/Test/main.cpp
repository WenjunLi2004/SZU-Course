#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
#define GLFW_INCLUDE_GLCOREARB
#include <OpenGL/gl3.h>
#endif

#include "Angel.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <functional>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

// 常量定义
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
const float PI = 3.14159265359f;

// 颜色定义
const float CYAN_R = 0.2f, CYAN_G = 0.8f, CYAN_B = 0.9f;
const float GOLD_R = 1.0f, GOLD_G = 0.8f, GOLD_B = 0.2f;
const float PINK_R = 1.0f, PINK_G = 0.4f, PINK_B = 0.7f;
const float PURPLE_R = 0.6f, PURPLE_G = 0.2f, PURPLE_B = 0.8f;
const float GREEN_R = 0.2f, GREEN_G = 0.8f, GREEN_B = 0.3f;
const float ORANGE_R = 1.0f, ORANGE_G = 0.5f, ORANGE_B = 0.0f;

// VAO索引
enum VAOIndex {
    ELLIPSE_OVERLAY_VAO = 0,
    MANDALA_VAO = 1,
    FLOWER_VAO = 2,
    SPIRAL_GALAXY_VAO = 3,
    FRACTAL_TREE_VAO = 4,
    ROSE_CURVE_VAO = 5,
    BUTTERFLY_VAO = 6,
    SNOWFLAKE_VAO = 7,
    NUM_VAOS_TOTAL = 8
};

// 全局变量
GLuint program;
GLuint vaos[NUM_VAOS_TOTAL];
GLuint vbos[NUM_VAOS_TOTAL];
int currentPattern = 0; // 当前显示的图案
int vertexCounts[NUM_VAOS_TOTAL]; // 存储每个图案的顶点数量

// 函数声明
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// 生成椭圆点
std::vector<vec3> generateEllipsePoints(float centerX, float centerY, float radiusX, float radiusY, int numPoints) {
    std::vector<vec3> points;
    for (int i = 0; i < numPoints; i++) {
        float angle = 2.0f * PI * i / numPoints;
        float x = centerX + radiusX * std::cos(angle);
        float y = centerY + radiusY * std::sin(angle);
        points.push_back(vec3(x, y, 0.0f));
    }
    return points;
}

// 生成椭圆左端点旋转叠加图案
std::vector<vec3> generateEllipseOverlay() {
    std::vector<vec3> vertices;
    
    const int numEllipses = 20;
    const int pointsPerEllipse = 100;
    const float baseRadiusX = 0.6f;
    const float baseRadiusY = 0.3f;
    
    for (int i = 0; i < numEllipses; i++) {
        float rotationAngle = 2.0f * PI * i / numEllipses;
        float radiusX = baseRadiusX * (1.0f - 0.3f * i / numEllipses);
        float radiusY = baseRadiusY * (1.0f - 0.3f * i / numEllipses);
        
        for (int j = 0; j < pointsPerEllipse; j++) {
            float angle = 2.0f * PI * j / pointsPerEllipse;
            float x = radiusX * std::cos(angle);
            float y = radiusY * std::sin(angle);
            
            // 旋转椭圆
            float rotatedX = x * std::cos(rotationAngle) - y * std::sin(rotationAngle);
            float rotatedY = x * std::sin(rotationAngle) + y * std::cos(rotationAngle);
            
            vertices.push_back(vec3(rotatedX, rotatedY, 0.0f));
        }
    }
    
    return vertices;
}

// 生成曼陀罗图案
std::vector<vec3> generateMandala() {
    std::vector<vec3> vertices;
    
    // 中心圆环
    const int centerPoints = 60;
    const float centerRadius = 0.08f;
    for (int i = 0; i < centerPoints; i++) {
        float angle = 2.0f * PI * i / centerPoints;
        float x = centerRadius * std::cos(angle);
        float y = centerRadius * std::sin(angle);
        vertices.push_back(vec3(x, y, 0.0f));
    }
    
    // 主要花瓣层
    const int numMainLayers = 8;
    const int numPetals = 12;
    
    for (int layer = 0; layer < numMainLayers; layer++) {
        float layerRadius = 0.12f + layer * 0.08f;
        float layerOffset = (layer % 2) * PI / numPetals; // 交替偏移
        
        for (int petal = 0; petal < numPetals; petal++) {
            float petalAngle = 2.0f * PI * petal / numPetals + layerOffset;
            
            // 复杂花瓣形状
            const int pointsPerPetal = 80;
            for (int point = 0; point < pointsPerPetal; point++) {
                float t = (float)point / pointsPerPetal;
                
                // 多重正弦波叠加创造复杂形状
                float radiusModulation = 1.0f + 0.3f * std::sin(6.0f * PI * t) + 
                                        0.2f * std::sin(12.0f * PI * t) +
                                        0.1f * std::sin(24.0f * PI * t);
                
                float localRadius = layerRadius * radiusModulation * (0.8f + 0.2f * layer / numMainLayers);
                
                // 花瓣的角度变化
                float petalWidth = PI / (numPetals * 1.5f);
                float localAngle = petalWidth * (t - 0.5f) * 2.0f;
                
                float x = localRadius * std::cos(localAngle);
                float y = localRadius * std::sin(localAngle);
                
                // 旋转到花瓣位置
                float rotatedX = x * std::cos(petalAngle) - y * std::sin(petalAngle);
                float rotatedY = x * std::sin(petalAngle) + y * std::cos(petalAngle);
                
                vertices.push_back(vec3(rotatedX, rotatedY, 0.0f));
            }
        }
    }
    
    // 外层装饰圆环
    const int numOuterRings = 3;
    for (int ring = 0; ring < numOuterRings; ring++) {
        float ringRadius = 0.75f + ring * 0.05f;
        int ringPoints = 120 + ring * 40;
        
        for (int i = 0; i < ringPoints; i++) {
            float angle = 2.0f * PI * i / ringPoints;
            float radiusVar = ringRadius * (1.0f + 0.1f * std::sin(16.0f * angle));
            
            float x = radiusVar * std::cos(angle);
            float y = radiusVar * std::sin(angle);
            vertices.push_back(vec3(x, y, 0.0f));
        }
    }
    
    // 连接线 - 从中心向外辐射
    const int numRadialLines = 24;
    for (int line = 0; line < numRadialLines; line++) {
        float lineAngle = 2.0f * PI * line / numRadialLines;
        
        const int pointsPerLine = 30;
        for (int point = 0; point < pointsPerLine; point++) {
            float t = (float)point / (pointsPerLine - 1);
            float radius = 0.08f + t * 0.7f;
            
            // 添加波浪效果
            radius *= (1.0f + 0.05f * std::sin(8.0f * PI * t));
            
            float x = radius * std::cos(lineAngle);
            float y = radius * std::sin(lineAngle);
            vertices.push_back(vec3(x, y, 0.0f));
        }
    }
    
    return vertices;
}

// 生成花朵图案
std::vector<vec3> generateFlower() {
    std::vector<vec3> vertices;
    
    // 花蕊中心
    const int centerPoints = 40;
    const float centerRadius = 0.06f;
    for (int i = 0; i < centerPoints; i++) {
        float angle = 2.0f * PI * i / centerPoints;
        float radius = centerRadius * (0.5f + 0.5f * std::cos(4.0f * angle));
        float x = radius * std::cos(angle);
        float y = radius * std::sin(angle);
        vertices.push_back(vec3(x, y, 0.0f));
    }
    
    // 内层小花瓣
    const int innerPetals = 8;
    const float innerPetalLength = 0.15f;
    for (int petal = 0; petal < innerPetals; petal++) {
        float petalAngle = 2.0f * PI * petal / innerPetals;
        
        const int pointsPerInnerPetal = 30;
        for (int point = 0; point < pointsPerInnerPetal; point++) {
            float t = (float)point / pointsPerInnerPetal;
            
            // 心形花瓣
            float petalRadius = innerPetalLength * std::sin(PI * t) * (1.0f + 0.3f * std::cos(3.0f * PI * t));
            
            float x = petalRadius * std::cos(petalAngle);
            float y = petalRadius * std::sin(petalAngle);
            vertices.push_back(vec3(x, y, 0.0f));
        }
    }
    
    // 主要花瓣层
    const int numMainPetals = 12;
    const float mainPetalLength = 0.5f;
    
    for (int petal = 0; petal < numMainPetals; petal++) {
        float petalAngle = 2.0f * PI * petal / numMainPetals;
        
        const int pointsPerPetal = 100;
        for (int point = 0; point < pointsPerPetal; point++) {
            float t = (float)point / pointsPerPetal;
            
            // 复杂花瓣形状 - 玫瑰花瓣
            float baseRadius = mainPetalLength * std::sin(PI * t);
            
            // 添加花瓣边缘的波浪效果
            float edgeWave = 1.0f + 0.2f * std::sin(8.0f * PI * t) + 0.1f * std::sin(16.0f * PI * t);
            
            // 花瓣宽度变化
            float widthFactor = 0.3f + 0.7f * std::sin(PI * t);
            
            float petalRadius = baseRadius * edgeWave;
            
            // 花瓣的横向偏移创造宽度
            float petalWidth = widthFactor * 0.15f;
            float lateralOffset = petalWidth * std::sin(2.0f * PI * t);
            
            float x = petalRadius * std::cos(petalAngle) + lateralOffset * std::cos(petalAngle + PI/2);
            float y = petalRadius * std::sin(petalAngle) + lateralOffset * std::sin(petalAngle + PI/2);
            
            vertices.push_back(vec3(x, y, 0.0f));
        }
    }
    
    // 外层装饰花瓣
    const int outerPetals = 6;
    const float outerPetalLength = 0.7f;
    
    for (int petal = 0; petal < outerPetals; petal++) {
        float petalAngle = 2.0f * PI * petal / outerPetals + PI / outerPetals; // 偏移角度
        
        const int pointsPerOuterPetal = 120;
        for (int point = 0; point < pointsPerOuterPetal; point++) {
            float t = (float)point / pointsPerOuterPetal;
            
            // 大花瓣形状 - 更加优雅的曲线
            float baseRadius = outerPetalLength * std::pow(std::sin(PI * t), 1.5f);
            
            // 花瓣边缘锯齿效果
            float serration = 1.0f + 0.15f * std::sin(12.0f * PI * t);
            
            float petalRadius = baseRadius * serration;
            
            // 花瓣的弯曲效果
            float curvature = 0.1f * std::sin(PI * t) * std::sin(2.0f * PI * t);
            
            float x = petalRadius * std::cos(petalAngle) + curvature * std::cos(petalAngle + PI/2);
            float y = petalRadius * std::sin(petalAngle) + curvature * std::sin(petalAngle + PI/2);
            
            vertices.push_back(vec3(x, y, 0.0f));
        }
    }
    
    // 花茎和叶子
    const int stemPoints = 50;
    for (int i = 0; i < stemPoints; i++) {
        float t = (float)i / (stemPoints - 1);
        float y = -0.1f - t * 0.6f;
        float x = 0.02f * std::sin(4.0f * PI * t); // 轻微弯曲
        vertices.push_back(vec3(x, y, 0.0f));
    }
    
    // 叶子
    const int numLeaves = 4;
    for (int leaf = 0; leaf < numLeaves; leaf++) {
        float leafY = -0.2f - leaf * 0.15f;
        float leafSide = (leaf % 2 == 0) ? 1.0f : -1.0f;
        
        const int leafPoints = 30;
        for (int point = 0; point < leafPoints; point++) {
            float t = (float)point / leafPoints;
            float leafLength = 0.2f * std::sin(PI * t);
            float leafWidth = 0.08f * std::sin(PI * t);
            
            float x = leafSide * leafLength + leafWidth * std::sin(2.0f * PI * t);
            float y = leafY + 0.1f * t;
            
            vertices.push_back(vec3(x, y, 0.0f));
        }
    }
    
    return vertices;
}

// 生成螺旋星系图案
std::vector<vec3> generateSpiralGalaxy() {
    std::vector<vec3> vertices;
    
    // 中心黑洞区域
    const int blackHolePoints = 80;
    const float blackHoleRadius = 0.03f;
    for (int i = 0; i < blackHolePoints; i++) {
        float angle = 2.0f * PI * i / blackHolePoints;
        float radius = blackHoleRadius * (0.5f + 0.5f * std::sin(8.0f * angle));
        float x = radius * std::cos(angle);
        float y = radius * std::sin(angle);
        vertices.push_back(vec3(x, y, 0.0f));
    }
    
    // 中心核球
    const int nucleusLayers = 5;
    for (int layer = 0; layer < nucleusLayers; layer++) {
        float layerRadius = 0.05f + layer * 0.02f;
        int layerPoints = 40 + layer * 20;
        
        for (int i = 0; i < layerPoints; i++) {
            float angle = 2.0f * PI * i / layerPoints + layer * 0.3f;
            float radius = layerRadius * (0.8f + 0.2f * std::sin(6.0f * angle));
            float x = radius * std::cos(angle);
            float y = radius * std::sin(angle);
            vertices.push_back(vec3(x, y, 0.0f));
        }
    }
    
    // 主要螺旋臂
    const int numMainArms = 4;
    const int pointsPerMainArm = 300;
    const float maxMainRadius = 0.85f;
    
    for (int arm = 0; arm < numMainArms; arm++) {
        float armOffset = 2.0f * PI * arm / numMainArms;
        
        for (int point = 0; point < pointsPerMainArm; point++) {
            float t = (float)point / pointsPerMainArm;
            float radius = 0.15f + maxMainRadius * t * t; // 非线性增长
            float angle = armOffset + 6.0f * PI * t;
            
            // 螺旋臂的密度变化
            float density = 1.0f + 0.5f * std::sin(12.0f * PI * t);
            
            // 添加星云效果的噪声
            float noise1 = 0.08f * std::sin(25.0f * t + arm);
            float noise2 = 0.04f * std::cos(50.0f * t + arm * 2);
            radius += noise1 + noise2;
            
            // 螺旋臂的宽度变化
            float armWidth = 0.1f * (1.0f - t * 0.5f);
            
            // 在螺旋臂宽度内生成多个点
            int widthPoints = (int)(density * 3);
            for (int w = 0; w < widthPoints; w++) {
                float widthOffset = (w - widthPoints/2.0f) * armWidth / widthPoints;
                float finalRadius = radius + widthOffset;
                
                float x = finalRadius * std::cos(angle);
                float y = finalRadius * std::sin(angle);
                vertices.push_back(vec3(x, y, 0.0f));
            }
        }
    }
    
    // 次要螺旋臂
    const int numMinorArms = 2;
    const int pointsPerMinorArm = 150;
    
    for (int arm = 0; arm < numMinorArms; arm++) {
        float armOffset = PI / numMinorArms + 2.0f * PI * arm / numMinorArms;
        
        for (int point = 0; point < pointsPerMinorArm; point++) {
            float t = (float)point / pointsPerMinorArm;
            float radius = 0.2f + 0.6f * t;
            float angle = armOffset + 4.0f * PI * t;
            
            // 次要臂的扰动
            float disturbance = 0.06f * std::sin(15.0f * t);
            radius += disturbance;
            
            float x = radius * std::cos(angle);
            float y = radius * std::sin(angle);
            vertices.push_back(vec3(x, y, 0.0f));
        }
    }
    
    // 外围星云和散布的恒星
    const int outerStars = 200;
    for (int star = 0; star < outerStars; star++) {
        float angle = 2.0f * PI * star / outerStars;
        float radius = 0.7f + 0.2f * std::sin(3.0f * angle);
        
        // 随机扰动
        radius += 0.1f * std::sin(17.0f * angle) * std::cos(23.0f * angle);
        
        float x = radius * std::cos(angle);
        float y = radius * std::sin(angle);
        vertices.push_back(vec3(x, y, 0.0f));
    }
    
    // 星系晕 - 外围稀疏恒星
    const int haloStars = 100;
    for (int star = 0; star < haloStars; star++) {
        float angle = 2.0f * PI * star / haloStars;
        float radius = 0.9f + 0.1f * std::sin(5.0f * angle);
        
        // 更大的随机性
        radius += 0.15f * std::sin(11.0f * angle) * std::cos(13.0f * angle);
        
        float x = radius * std::cos(angle);
        float y = radius * std::sin(angle);
        vertices.push_back(vec3(x, y, 0.0f));
    }
    
    return vertices;
}

// 生成分形树图案
std::vector<vec3> generateFractalTree() {
    std::vector<vec3> vertices;
    
    std::function<void(float, float, float, float, float, int, float)> drawBranch = 
        [&](float x1, float y1, float angle, float branchLength, float thickness, int depth, float windEffect) {
        if (depth <= 0 || branchLength < 0.01f) return;
        
        float x2 = x1 + branchLength * std::cos(angle + windEffect * 0.1f);
        float y2 = y1 + branchLength * std::sin(angle + windEffect * 0.1f);
        
        // 绘制树枝主体（多条线模拟粗细）
        int branchLines = std::max(1, (int)(thickness * 10));
        for (int line = 0; line < branchLines; line++) {
            float offset = (line - branchLines/2.0f) * thickness * 0.02f;
            float perpX = -std::sin(angle);
            float perpY = std::cos(angle);
            
            // 添加分支线段的点
            int numPoints = std::max(2, (int)(branchLength * 50));
            for (int i = 0; i <= numPoints; i++) {
                float t = (float)i / numPoints;
                float x = x1 + t * (x2 - x1) + perpX * offset;
                float y = y1 + t * (y2 - y1) + perpY * offset;
                vertices.push_back(vec3(x, y, 0.0f));
            }
        }
        
        // 添加叶子（在较小的分支上）
        if (depth <= 3 && depth > 0) {
            int numLeaves = 2 + depth;
            for (int leaf = 0; leaf < numLeaves; leaf++) {
                float leafT = (float)leaf / numLeaves;
                float leafX = x1 + (x2 - x1) * leafT;
                float leafY = y1 + (y2 - y1) * leafT;
                
                // 叶子形状（小椭圆）
                int leafPoints = 6;
                float leafSize = 0.015f * (1.0f + 0.3f * std::sin(leaf * 3.0f));
                for (int p = 0; p < leafPoints; p++) {
                    float leafAngle = 2.0f * PI * p / leafPoints;
                    float leafRadius = leafSize * (0.8f + 0.2f * std::cos(2.0f * leafAngle));
                    float lx = leafX + leafRadius * std::cos(leafAngle + windEffect);
                    float ly = leafY + leafRadius * std::sin(leafAngle + windEffect) * 0.6f;
                    vertices.push_back(vec3(lx, ly, 0.0f));
                }
            }
        }
        
        // 递归绘制子分支
        if (depth > 1) {
            float angleVariation = PI / 6 + 0.1f * std::sin(depth * 1.2f);
            float leftAngle = angle + angleVariation;
            float rightAngle = angle - angleVariation;
            float newLength = branchLength * (0.65f + 0.1f * std::sin(depth * 0.8f));
            float newThickness = thickness * 0.75f;
            
            drawBranch(x2, y2, leftAngle, newLength, newThickness, depth - 1, windEffect);
            drawBranch(x2, y2, rightAngle, newLength, newThickness, depth - 1, windEffect);
            
            // 添加额外的小分支（增加复杂性）
            if (depth > 3) {
                float middleAngle = angle + 0.1f * std::sin(depth * 2.0f);
                float shortLength = newLength * 0.6f;
                drawBranch(x2, y2, middleAngle, shortLength, newThickness * 0.5f, depth - 2, windEffect);
            }
            
            // 侧面小分支
            if (depth > 4 && depth % 2 == 0) {
                float sideAngle = angle + PI/3 + 0.2f * std::sin(depth);
                float sideLength = newLength * 0.4f;
                drawBranch(x2, y2, sideAngle, sideLength, newThickness * 0.3f, depth - 3, windEffect);
            }
        }
    };
    
    // 树根系统
    int numRoots = 5;
    for (int root = 0; root < numRoots; root++) {
        float rootAngle = -PI/2 + (root - numRoots/2.0f) * 0.4f;
        float rootLength = 0.12f + 0.03f * std::sin(root * 2.5f);
        float rootX = rootLength * std::cos(rootAngle);
        float rootY = -0.8f + rootLength * std::sin(rootAngle);
        
        // 主根
        int rootPoints = 15;
        for (int i = 0; i <= rootPoints; i++) {
            float t = (float)i / rootPoints;
            float x = t * rootX;
            float y = -0.8f + t * (rootY + 0.8f);
            vertices.push_back(vec3(x, y, 0.0f));
        }
        
        // 根的细分支
        for (int subRoot = 0; subRoot < 2; subRoot++) {
            float subAngle = rootAngle + (subRoot - 0.5f) * 0.5f;
            float subLength = rootLength * 0.5f;
            float subX = rootX + subLength * std::cos(subAngle);
            float subY = rootY + subLength * std::sin(subAngle);
            
            int subRootPoints = 8;
            for (int i = 0; i <= subRootPoints; i++) {
                float t = (float)i / subRootPoints;
                float x = rootX + t * (subX - rootX);
                float y = rootY + t * (subY - rootY);
                vertices.push_back(vec3(x, y, 0.0f));
            }
        }
    }
    
    // 主干和主要分支
    float windEffect = 0.08f * std::sin(1.2f); // 模拟风的效果
    
    // 主干
    drawBranch(0.0f, -0.8f, PI/2, 0.4f, 1.0f, 8, windEffect);
    
    // 额外的主要分支（让树更茂密）
    drawBranch(0.0f, -0.6f, PI/2 + 0.2f, 0.3f, 0.8f, 6, windEffect);
    drawBranch(0.0f, -0.5f, PI/2 - 0.15f, 0.25f, 0.6f, 5, windEffect);
    
    return vertices;
}

// 生成玫瑰曲线图案
std::vector<vec3> generateRoseCurve() {
    std::vector<vec3> vertices;
    
    const int numPoints = 1000;
    const float k = 4.0f; // 玫瑰曲线参数
    const float scale = 0.6f;
    
    for (int i = 0; i < numPoints; i++) {
        float t = 2.0f * PI * i / numPoints;
        float r = scale * std::cos(k * t);
        
        float x = r * std::cos(t);
        float y = r * std::sin(t);
        
        vertices.push_back(vec3(x, y, 0.0f));
    }
    
    return vertices;
}

// 生成蝴蝶图案
std::vector<vec3> generateButterfly() {
    std::vector<vec3> vertices;
    
    // 蝴蝶身体
    const int bodyPoints = 30;
    const float bodyLength = 0.6f;
    const float bodyWidth = 0.02f;
    
    for (int i = 0; i < bodyPoints; i++) {
        float t = (float)i / (bodyPoints - 1);
        float y = -bodyLength/2 + bodyLength * t;
        float width = bodyWidth * (1.0f - 0.3f * std::abs(t - 0.5f) * 2.0f);
        
        // 身体中线
        vertices.push_back(vec3(0.0f, y, 0.0f));
        
        // 身体轮廓
        vertices.push_back(vec3(-width, y, 0.0f));
        vertices.push_back(vec3(width, y, 0.0f));
        
        // 身体节段
        if (i % 3 == 0) {
            vertices.push_back(vec3(-width * 1.5f, y, 0.0f));
            vertices.push_back(vec3(width * 1.5f, y, 0.0f));
        }
    }
    
    // 触角
    const int antennaPoints = 15;
    for (int antenna = 0; antenna < 2; antenna++) {
        float side = (antenna == 0) ? -1.0f : 1.0f;
        for (int i = 0; i < antennaPoints; i++) {
            float t = (float)i / (antennaPoints - 1);
            float x = side * 0.05f * (1.0f + t);
            float y = 0.25f + 0.1f * t * std::sin(t * PI * 2);
            vertices.push_back(vec3(x, y, 0.0f));
        }
        
        // 触角末端小球
        float endX = side * 0.05f * 2.0f;
        float endY = 0.35f;
        const int ballPoints = 8;
        for (int i = 0; i < ballPoints; i++) {
            float angle = 2.0f * PI * i / ballPoints;
            float x = endX + 0.01f * std::cos(angle);
            float y = endY + 0.01f * std::sin(angle);
            vertices.push_back(vec3(x, y, 0.0f));
        }
    }
    
    // 上翅膀（左右对称）
    for (int wing = 0; wing < 2; wing++) {
        float side = (wing == 0) ? -1.0f : 1.0f;
        
        // 上翅膀外轮廓
        const int upperWingPoints = 50;
        for (int i = 0; i < upperWingPoints; i++) {
            float t = (float)i / (upperWingPoints - 1);
            
            // 蝴蝶翅膀的参数方程
            float angle = t * PI;
            float radius = 0.4f * (1.0f + 0.3f * std::sin(3.0f * angle));
            
            float x = side * radius * std::sin(angle) * (1.0f + 0.2f * std::sin(5.0f * angle));
            float y = 0.1f + radius * std::cos(angle) * 0.8f;
            
            vertices.push_back(vec3(x, y, 0.0f));
        }
        
        // 上翅膀内部装饰纹路
        for (int layer = 1; layer <= 3; layer++) {
            float layerScale = 1.0f - layer * 0.2f;
            const int decorPoints = 25;
            
            for (int i = 0; i < decorPoints; i++) {
                float t = (float)i / (decorPoints - 1);
                float angle = t * PI;
                float radius = 0.4f * layerScale * (1.0f + 0.3f * std::sin(3.0f * angle));
                
                float x = side * radius * std::sin(angle) * (1.0f + 0.2f * std::sin(5.0f * angle));
                float y = 0.1f + radius * std::cos(angle) * 0.8f;
                
                vertices.push_back(vec3(x, y, 0.0f));
            }
        }
        
        // 上翅膀斑点
        const int upperSpots = 8;
        for (int spot = 0; spot < upperSpots; spot++) {
            float spotAngle = (float)spot / upperSpots * PI;
            float spotRadius = 0.25f;
            float spotX = side * spotRadius * std::sin(spotAngle);
            float spotY = 0.1f + spotRadius * std::cos(spotAngle) * 0.8f;
            
            // 每个斑点是小圆
            const int spotPoints = 8;
            float spotSize = 0.02f + 0.01f * std::sin(spot * 2.0f);
            for (int p = 0; p < spotPoints; p++) {
                float pAngle = 2.0f * PI * p / spotPoints;
                float x = spotX + spotSize * std::cos(pAngle);
                float y = spotY + spotSize * std::sin(pAngle);
                vertices.push_back(vec3(x, y, 0.0f));
            }
        }
    }
    
    // 下翅膀（左右对称）
    for (int wing = 0; wing < 2; wing++) {
        float side = (wing == 0) ? -1.0f : 1.0f;
        
        // 下翅膀外轮廓
        const int lowerWingPoints = 40;
        for (int i = 0; i < lowerWingPoints; i++) {
            float t = (float)i / (lowerWingPoints - 1);
            
            // 下翅膀较小，形状不同
            float angle = t * PI * 0.8f;
            float radius = 0.25f * (1.0f + 0.4f * std::sin(2.0f * angle));
            
            float x = side * radius * std::sin(angle) * (1.0f + 0.3f * std::sin(4.0f * angle));
            float y = -0.05f - radius * std::cos(angle) * 0.6f;
            
            vertices.push_back(vec3(x, y, 0.0f));
        }
        
        // 下翅膀装饰纹路
        for (int layer = 1; layer <= 2; layer++) {
            float layerScale = 1.0f - layer * 0.3f;
            const int decorPoints = 20;
            
            for (int i = 0; i < decorPoints; i++) {
                float t = (float)i / (decorPoints - 1);
                float angle = t * PI * 0.8f;
                float radius = 0.25f * layerScale * (1.0f + 0.4f * std::sin(2.0f * angle));
                
                float x = side * radius * std::sin(angle) * (1.0f + 0.3f * std::sin(4.0f * angle));
                float y = -0.05f - radius * std::cos(angle) * 0.6f;
                
                vertices.push_back(vec3(x, y, 0.0f));
            }
        }
        
        // 下翅膀尾部装饰
        const int tailPoints = 12;
        for (int i = 0; i < tailPoints; i++) {
            float t = (float)i / (tailPoints - 1);
            float x = side * (0.15f + 0.05f * std::sin(t * PI * 3));
            float y = -0.25f - 0.1f * t;
            vertices.push_back(vec3(x, y, 0.0f));
        }
    }
    
    // 翅膀连接线（翅脉）
    for (int wing = 0; wing < 2; wing++) {
        float side = (wing == 0) ? -1.0f : 1.0f;
        
        // 主翅脉
        const int veinLines = 5;
        for (int vein = 0; vein < veinLines; vein++) {
            float veinAngle = (float)vein / veinLines * PI * 0.8f;
            const int veinPoints = 15;
            
            for (int i = 0; i < veinPoints; i++) {
                float t = (float)i / (veinPoints - 1);
                float radius = t * 0.3f;
                float x = side * radius * std::sin(veinAngle);
                float y = 0.05f + radius * std::cos(veinAngle) * 0.7f;
                vertices.push_back(vec3(x, y, 0.0f));
            }
        }
    }
    
    return vertices;
}

// 生成雪花图案
std::vector<vec3> generateSnowflake() {
    std::vector<vec3> vertices;
    
    // 雪花中心
    vertices.push_back(vec3(0.0f, 0.0f, 0.0f));
    
    // 主要的6条射线
    const int numMainRays = 6;
    const float mainRayLength = 0.7f;
    
    for (int ray = 0; ray < numMainRays; ray++) {
        float angle = ray * PI / 3.0f; // 60度间隔
        
        // 主射线
        const int rayPoints = 50;
        for (int i = 0; i <= rayPoints; i++) {
            float t = (float)i / rayPoints;
            float radius = mainRayLength * t;
            
            // 添加一些变化让射线不那么直
            radius += 0.02f * std::sin(t * PI * 8) * t;
            
            float x = radius * std::cos(angle);
            float y = radius * std::sin(angle);
            vertices.push_back(vec3(x, y, 0.0f));
        }
        
        // 主射线上的分支
        const int numBranches = 8;
        for (int branch = 1; branch <= numBranches; branch++) {
            float branchT = (float)branch / (numBranches + 1);
            float branchRadius = mainRayLength * branchT;
            float branchX = branchRadius * std::cos(angle);
            float branchY = branchRadius * std::sin(angle);
            
            // 左分支
            float leftAngle = angle + PI / 6; // 30度
            float branchLength = 0.15f * (1.0f - branchT * 0.5f);
            const int branchPoints = 15;
            
            for (int i = 0; i <= branchPoints; i++) {
                float bt = (float)i / branchPoints;
                float bRadius = branchLength * bt;
                float bx = branchX + bRadius * std::cos(leftAngle);
                float by = branchY + bRadius * std::sin(leftAngle);
                vertices.push_back(vec3(bx, by, 0.0f));
            }
            
            // 右分支
            float rightAngle = angle - PI / 6; // -30度
            for (int i = 0; i <= branchPoints; i++) {
                float bt = (float)i / branchPoints;
                float bRadius = branchLength * bt;
                float bx = branchX + bRadius * std::cos(rightAngle);
                float by = branchY + bRadius * std::sin(rightAngle);
                vertices.push_back(vec3(bx, by, 0.0f));
            }
            
            // 二级分支（在主分支上）
            if (branch % 2 == 0) {
                float subBranchLength = branchLength * 0.6f;
                
                // 左分支的子分支
                float leftSubX = branchX + branchLength * 0.7f * std::cos(leftAngle);
                float leftSubY = branchY + branchLength * 0.7f * std::sin(leftAngle);
                
                for (int side = 0; side < 2; side++) {
                    float subAngle = leftAngle + (side == 0 ? PI/8 : -PI/8);
                    const int subBranchPoints = 8;
                    
                    for (int i = 0; i <= subBranchPoints; i++) {
                        float st = (float)i / subBranchPoints;
                        float sRadius = subBranchLength * st;
                        float sx = leftSubX + sRadius * std::cos(subAngle);
                        float sy = leftSubY + sRadius * std::sin(subAngle);
                        vertices.push_back(vec3(sx, sy, 0.0f));
                    }
                }
                
                // 右分支的子分支
                float rightSubX = branchX + branchLength * 0.7f * std::cos(rightAngle);
                float rightSubY = branchY + branchLength * 0.7f * std::sin(rightAngle);
                
                for (int side = 0; side < 2; side++) {
                    float subAngle = rightAngle + (side == 0 ? PI/8 : -PI/8);
                    const int subBranchPoints = 8;
                    
                    for (int i = 0; i <= subBranchPoints; i++) {
                        float st = (float)i / subBranchPoints;
                        float sRadius = subBranchLength * st;
                        float sx = rightSubX + sRadius * std::cos(subAngle);
                        float sy = rightSubY + sRadius * std::sin(subAngle);
                        vertices.push_back(vec3(sx, sy, 0.0f));
                    }
                }
            }
        }
        
        // 射线末端的装饰
        float endX = mainRayLength * std::cos(angle);
        float endY = mainRayLength * std::sin(angle);
        
        // 末端星形装饰
        const int starPoints = 8;
        float starSize = 0.08f;
        for (int star = 0; star < starPoints; star++) {
            float starAngle = star * PI / 4;
            float starRadius = starSize * (star % 2 == 0 ? 1.0f : 0.5f);
            float sx = endX + starRadius * std::cos(starAngle);
            float sy = endY + starRadius * std::sin(starAngle);
            vertices.push_back(vec3(sx, sy, 0.0f));
        }
    }
    
    // 中心装饰环
    const int centerRings = 3;
    for (int ring = 1; ring <= centerRings; ring++) {
        float ringRadius = 0.05f * ring;
        const int ringPoints = 12 * ring;
        
        for (int i = 0; i < ringPoints; i++) {
            float angle = 2.0f * PI * i / ringPoints;
            float radius = ringRadius * (1.0f + 0.2f * std::sin(6.0f * angle));
            float x = radius * std::cos(angle);
            float y = radius * std::sin(angle);
            vertices.push_back(vec3(x, y, 0.0f));
        }
    }
    
    // 六边形内部装饰
    const int hexLayers = 4;
    for (int layer = 1; layer <= hexLayers; layer++) {
        float hexRadius = 0.2f + layer * 0.08f;
        const int hexPoints = 6;
        
        for (int hex = 0; hex < hexPoints; hex++) {
            float hexAngle = hex * PI / 3.0f;
            
            // 六边形顶点
            float hx = hexRadius * std::cos(hexAngle);
            float hy = hexRadius * std::sin(hexAngle);
            vertices.push_back(vec3(hx, hy, 0.0f));
            
            // 六边形边上的装饰点
            if (layer <= 2) {
                float nextAngle = (hex + 1) * PI / 3.0f;
                float nextX = hexRadius * std::cos(nextAngle);
                float nextY = hexRadius * std::sin(nextAngle);
                
                const int edgePoints = 5;
                for (int edge = 1; edge < edgePoints; edge++) {
                    float et = (float)edge / edgePoints;
                    float ex = hx + et * (nextX - hx);
                    float ey = hy + et * (nextY - hy);
                    
                    // 添加小装饰
                    float decorSize = 0.02f;
                    float decorAngle = hexAngle + PI/2;
                    float dx = ex + decorSize * std::cos(decorAngle);
                    float dy = ey + decorSize * std::sin(decorAngle);
                    vertices.push_back(vec3(dx, dy, 0.0f));
                    
                    dx = ex - decorSize * std::cos(decorAngle);
                    dy = ey - decorSize * std::sin(decorAngle);
                    vertices.push_back(vec3(dx, dy, 0.0f));
                }
            }
        }
    }
    
    // 随机雪花晶体点
    const int crystalPoints = 100;
    for (int crystal = 0; crystal < crystalPoints; crystal++) {
        float crystalAngle = 2.0f * PI * crystal / crystalPoints;
        float crystalRadius = 0.3f + 0.3f * std::sin(crystal * 0.1f);
        
        // 只在特定角度附近生成（保持六重对称）
        bool nearMainRay = false;
        for (int ray = 0; ray < 6; ray++) {
            float rayAngle = ray * PI / 3.0f;
            float angleDiff = std::abs(crystalAngle - rayAngle);
            if (angleDiff < PI/12 || angleDiff > 2*PI - PI/12) {
                nearMainRay = true;
                break;
            }
        }
        
        if (nearMainRay) {
            float x = crystalRadius * std::cos(crystalAngle);
            float y = crystalRadius * std::sin(crystalAngle);
            vertices.push_back(vec3(x, y, 0.0f));
        }
    }
    
    return vertices;
}

void init() {
    // 生成顶点数组对象
    glGenVertexArrays(NUM_VAOS_TOTAL, vaos);
    glGenBuffers(NUM_VAOS_TOTAL, vbos);
    
    // 读取着色器
    std::string vshader = "shaders/vshader.glsl";
    std::string fshader = "shaders/fshader.glsl";
    program = InitShader(vshader.c_str(), fshader.c_str());
    glUseProgram(program);
    
    GLuint posLocation = glGetAttribLocation(program, "vPosition");
    
    // 初始化椭圆叠加图案
    std::vector<vec3> ellipseVertices = generateEllipseOverlay();
    vertexCounts[ELLIPSE_OVERLAY_VAO] = ellipseVertices.size();
    glBindVertexArray(vaos[ELLIPSE_OVERLAY_VAO]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[ELLIPSE_OVERLAY_VAO]);
    glBufferData(GL_ARRAY_BUFFER, ellipseVertices.size() * sizeof(vec3), ellipseVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(posLocation);
    glVertexAttribPointer(posLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    // 初始化曼陀罗图案
    std::vector<vec3> mandalaVertices = generateMandala();
    vertexCounts[MANDALA_VAO] = mandalaVertices.size();
    glBindVertexArray(vaos[MANDALA_VAO]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[MANDALA_VAO]);
    glBufferData(GL_ARRAY_BUFFER, mandalaVertices.size() * sizeof(vec3), mandalaVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(posLocation);
    glVertexAttribPointer(posLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    // 初始化花朵图案
    std::vector<vec3> flowerVertices = generateFlower();
    vertexCounts[FLOWER_VAO] = flowerVertices.size();
    glBindVertexArray(vaos[FLOWER_VAO]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[FLOWER_VAO]);
    glBufferData(GL_ARRAY_BUFFER, flowerVertices.size() * sizeof(vec3), flowerVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(posLocation);
    glVertexAttribPointer(posLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    // 初始化螺旋星系图案
    std::vector<vec3> spiralVertices = generateSpiralGalaxy();
    vertexCounts[SPIRAL_GALAXY_VAO] = spiralVertices.size();
    glBindVertexArray(vaos[SPIRAL_GALAXY_VAO]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[SPIRAL_GALAXY_VAO]);
    glBufferData(GL_ARRAY_BUFFER, spiralVertices.size() * sizeof(vec3), spiralVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(posLocation);
    glVertexAttribPointer(posLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    // 初始化分形树图案
    std::vector<vec3> treeVertices = generateFractalTree();
    vertexCounts[FRACTAL_TREE_VAO] = treeVertices.size();
    glBindVertexArray(vaos[FRACTAL_TREE_VAO]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[FRACTAL_TREE_VAO]);
    glBufferData(GL_ARRAY_BUFFER, treeVertices.size() * sizeof(vec3), treeVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(posLocation);
    glVertexAttribPointer(posLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    // 初始化玫瑰曲线图案
    std::vector<vec3> roseVertices = generateRoseCurve();
    vertexCounts[ROSE_CURVE_VAO] = roseVertices.size();
    glBindVertexArray(vaos[ROSE_CURVE_VAO]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[ROSE_CURVE_VAO]);
    glBufferData(GL_ARRAY_BUFFER, roseVertices.size() * sizeof(vec3), roseVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(posLocation);
    glVertexAttribPointer(posLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    // 初始化蝴蝶图案
    std::vector<vec3> butterflyVertices = generateButterfly();
    vertexCounts[BUTTERFLY_VAO] = butterflyVertices.size();
    glBindVertexArray(vaos[BUTTERFLY_VAO]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[BUTTERFLY_VAO]);
    glBufferData(GL_ARRAY_BUFFER, butterflyVertices.size() * sizeof(vec3), butterflyVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(posLocation);
    glVertexAttribPointer(posLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    // 初始化雪花图案
    std::vector<vec3> snowflakeVertices = generateSnowflake();
    vertexCounts[SNOWFLAKE_VAO] = snowflakeVertices.size();
    glBindVertexArray(vaos[SNOWFLAKE_VAO]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[SNOWFLAKE_VAO]);
    glBufferData(GL_ARRAY_BUFFER, snowflakeVertices.size() * sizeof(vec3), snowflakeVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(posLocation);
    glVertexAttribPointer(posLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    // 设置背景色
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    // 启用点的大小设置
    glEnable(GL_PROGRAM_POINT_SIZE);
    glPointSize(2.0f);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    // 设置颜色uniform
    GLuint colorLocation = glGetUniformLocation(program, "uColor");
    
    switch (currentPattern) {
        case 0: // 椭圆叠加
            glBindVertexArray(vaos[ELLIPSE_OVERLAY_VAO]);
            glUniform3f(colorLocation, CYAN_R, CYAN_G, CYAN_B);
            glDrawArrays(GL_POINTS, 0, vertexCounts[ELLIPSE_OVERLAY_VAO]);
            break;
            
        case 1: // 曼陀罗
            glBindVertexArray(vaos[MANDALA_VAO]);
            glUniform3f(colorLocation, GOLD_R, GOLD_G, GOLD_B);
            glDrawArrays(GL_POINTS, 0, vertexCounts[MANDALA_VAO]);
            break;
            
        case 2: // 花朵
            glBindVertexArray(vaos[FLOWER_VAO]);
            glUniform3f(colorLocation, PINK_R, PINK_G, PINK_B);
            glDrawArrays(GL_POINTS, 0, vertexCounts[FLOWER_VAO]);
            break;
            
        case 3: // 螺旋星系
            glBindVertexArray(vaos[SPIRAL_GALAXY_VAO]);
            glUniform3f(colorLocation, PURPLE_R, PURPLE_G, PURPLE_B);
            glDrawArrays(GL_POINTS, 0, vertexCounts[SPIRAL_GALAXY_VAO]);
            break;
            
        case 4: // 分形树
            glBindVertexArray(vaos[FRACTAL_TREE_VAO]);
            glUniform3f(colorLocation, GREEN_R, GREEN_G, GREEN_B);
            glDrawArrays(GL_POINTS, 0, vertexCounts[FRACTAL_TREE_VAO]);
            break;
            
        case 5: // 玫瑰曲线
            glBindVertexArray(vaos[ROSE_CURVE_VAO]);
            glUniform3f(colorLocation, ORANGE_R, ORANGE_G, ORANGE_B);
            glDrawArrays(GL_POINTS, 0, vertexCounts[ROSE_CURVE_VAO]);
            break;
            
        case 6: // 蝴蝶
            glBindVertexArray(vaos[BUTTERFLY_VAO]);
            glUniform3f(colorLocation, PINK_R, PINK_G, PINK_B);
            glDrawArrays(GL_POINTS, 0, vertexCounts[BUTTERFLY_VAO]);
            break;
            
        case 7: // 雪花图案
            glBindVertexArray(vaos[SNOWFLAKE_VAO]);
            glUniform3f(colorLocation, 0.9f, 0.95f, 1.0f); // 淡蓝白色
            glDrawArrays(GL_POINTS, 0, vertexCounts[SNOWFLAKE_VAO]);
            break;
    }
    
    glFlush();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_1:
                currentPattern = 0;
                std::cout << "切换到椭圆左端点旋转叠加图案" << std::endl;
                break;
            case GLFW_KEY_2:
                currentPattern = 1;
                std::cout << "切换到曼陀罗图案" << std::endl;
                break;
            case GLFW_KEY_3:
                currentPattern = 2;
                std::cout << "切换到花朵图案" << std::endl;
                break;
            case GLFW_KEY_4:
                currentPattern = 3;
                std::cout << "切换到螺旋星系图案" << std::endl;
                break;
            case GLFW_KEY_5:
                currentPattern = 4;
                std::cout << "切换到分形树图案" << std::endl;
                break;
            case GLFW_KEY_6:
                currentPattern = 5;
                std::cout << "切换到玫瑰曲线图案" << std::endl;
                break;
            case GLFW_KEY_7:
            currentPattern = 6;
            std::cout << "切换到蝴蝶图案" << std::endl;
            break;
        case GLFW_KEY_8:
            currentPattern = 7;
            std::cout << "切换到雪花图案" << std::endl;
            break;
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, true);
                break;
        }
    }
}

int main() {
    // 初始化GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "复杂几何图形绘制", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    init();
    
    std::cout << "复杂几何图形绘制程序" << std::endl;
    std::cout << "控制说明:" << std::endl;
    std::cout << "按键1: 椭圆左端点旋转叠加图案 (青色)" << std::endl;
    std::cout << "按键2: 曼陀罗图案 (金色)" << std::endl;
    std::cout << "按键3: 花朵图案 (粉色)" << std::endl;
    std::cout << "按键4: 螺旋星系图案 (紫色)" << std::endl;
    std::cout << "按键5: 分形树图案 (绿色)" << std::endl;
    std::cout << "按键6: 玫瑰曲线图案 (橙色)" << std::endl;
    std::cout << "ESC: 退出程序" << std::endl;

    // 渲染循环
    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 清理资源
    glDeleteVertexArrays(NUM_VAOS_TOTAL, vaos);
    glDeleteBuffers(NUM_VAOS_TOTAL, vbos);
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
