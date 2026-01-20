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
const float BROWN_R = 0.6f, BROWN_G = 0.3f, BROWN_B = 0.1f;

// VAO索引
enum VAOIndex {
    ELLIPSE_OVERLAY_VAO = 0,
    SPIRAL_GALAXY_VAO = 1,
    FRACTAL_TREE_VAO = 2,
    FRACTAL_TREE_ROOTS_VAO = 3,
    ROSE_CURVE_VAO = 4,
    BUTTERFLY_VAO = 5,
    SNOWFLAKE_VAO = 6,
    NUM_VAOS_TOTAL = 7
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

// 生成对称椭圆叠加图案
std::vector<vec3> generateEllipseOverlay() {
    std::vector<vec3> vertices;
    
    // 创建更规整的对称图案
    const int numLayers = 8;  // 减少层数，增加对称性
    const int pointsPerEllipse = 120;
    
    // 中心圆
    const int centerPoints = 60;
    const float centerRadius = 0.05f;
    for (int i = 0; i < centerPoints; i++) {
        float angle = 2.0f * PI * i / centerPoints;
        float x = centerRadius * std::cos(angle);
        float y = centerRadius * std::sin(angle);
        vertices.push_back(vec3(x, y, 0.0f));
    }
    
    // 对称的椭圆层
    for (int layer = 0; layer < numLayers; layer++) {
        float layerScale = 1.0f - (float)layer / (numLayers + 2);
        float baseRadiusX = 0.7f * layerScale;
        float baseRadiusY = 0.4f * layerScale;
        
        // 每层有固定数量的椭圆，确保对称
        const int ellipsesPerLayer = 12;
        
        for (int ellipse = 0; ellipse < ellipsesPerLayer; ellipse++) {
            float rotationAngle = 2.0f * PI * ellipse / ellipsesPerLayer;
            
            // 添加层间的微小偏移，但保持整体对称
            float layerOffset = (layer % 2) * PI / ellipsesPerLayer;
            rotationAngle += layerOffset;
            
            for (int point = 0; point < pointsPerEllipse; point++) {
                float t = (float)point / pointsPerEllipse;
                float angle = 2.0f * PI * t;
                
                // 椭圆参数方程
                float x = baseRadiusX * std::cos(angle);
                float y = baseRadiusY * std::sin(angle);
                
                // 旋转到指定角度
                float rotatedX = x * std::cos(rotationAngle) - y * std::sin(rotationAngle);
                float rotatedY = x * std::sin(rotationAngle) + y * std::cos(rotationAngle);
                
                vertices.push_back(vec3(rotatedX, rotatedY, 0.0f));
            }
        }
    }
    
    // 添加外围装饰圆环，增强对称性
    const int numRings = 3;
    for (int ring = 0; ring < numRings; ring++) {
        float ringRadius = 0.75f + ring * 0.05f;
        int ringPoints = 80 + ring * 20;
        
        for (int i = 0; i < ringPoints; i++) {
            float angle = 2.0f * PI * i / ringPoints;
            
            // 添加规律的波动，保持对称
            float radiusModulation = 1.0f + 0.1f * std::cos(8.0f * angle);
            float finalRadius = ringRadius * radiusModulation;
            
            float x = finalRadius * std::cos(angle);
            float y = finalRadius * std::sin(angle);
            vertices.push_back(vec3(x, y, 0.0f));
        }
    }
    
    return vertices;
}





// 生成螺旋星系图案
std::vector<vec3> generateSpiralGalaxy() {
    std::vector<vec3> vertices;
    
    // 中心核心 - 明亮的星系核心
    const int corePoints = 80;
    const float coreRadius = 0.06f;
    for (int i = 0; i < corePoints; i++) {
        float angle = 2.0f * PI * i / corePoints;
        float radius = coreRadius * std::sqrt((float)i / corePoints);
        float x = radius * std::cos(angle);
        float y = radius * std::sin(angle);
        vertices.push_back(vec3(x, y, 0.0f));
    }
    
    // 主螺旋臂 - 更清晰的螺旋结构
    const int numArms = 2; // 减少到2个主要螺旋臂，更典型的星系结构
    const int pointsPerArm = 1200;
    
    for (int arm = 0; arm < numArms; arm++) {
        float armOffset = PI * arm; // 两个螺旋臂相对180度
        
        for (int point = 0; point < pointsPerArm; point++) {
            float t = (float)point / pointsPerArm;
            
            // 螺旋参数 - 更紧密的螺旋
            float spiralTightness = 2.5f;
            float angle = armOffset + spiralTightness * 2.0f * PI * t;
            
            // 径向距离 - 从中心向外扩展
            float baseRadius = 0.08f + t * 0.75f;
            
            // 螺旋臂密度变化 - 创造明暗交替效果
            float densityWave = 1.0f + 0.6f * std::cos(4.0f * PI * t);
            
            // 螺旋臂宽度 - 向外逐渐变宽
            float armWidth = 0.03f + 0.04f * t;
            
            // 在螺旋臂宽度内分布点
            const int pointsAcrossArm = 8;
            for (int w = 0; w < pointsAcrossArm; w++) {
                float widthT = (float)w / (pointsAcrossArm - 1) - 0.5f; // -0.5 到 0.5
                float widthOffset = widthT * armWidth * densityWave;
                
                // 计算垂直于螺旋臂的方向
                float perpAngle = angle + PI / 2.0f;
                float offsetX = widthOffset * std::cos(perpAngle);
                float offsetY = widthOffset * std::sin(perpAngle);
                
                float radius = baseRadius;
                float x = radius * std::cos(angle) + offsetX;
                float y = radius * std::sin(angle) + offsetY;
                
                vertices.push_back(vec3(x, y, 0.0f));
            }
        }
    }
    
    // 次要螺旋臂 - 更细的分支结构
    const int numMinorArms = 2;
    const int pointsPerMinorArm = 600;
    
    for (int arm = 0; arm < numMinorArms; arm++) {
        float armOffset = PI * arm + PI / 2.0f; // 与主螺旋臂错开90度
        
        for (int point = 0; point < pointsPerMinorArm; point++) {
            float t = (float)point / pointsPerMinorArm;
            
            float spiralTightness = 2.2f;
            float angle = armOffset + spiralTightness * 2.0f * PI * t;
            
            float baseRadius = 0.15f + t * 0.6f;
            float armWidth = 0.02f + 0.02f * t;
            
            // 次要螺旋臂密度较低
            if (point % 3 == 0) { // 只取1/3的点
                float widthOffset = (std::sin(8.0f * PI * t) * 0.5f) * armWidth;
                float perpAngle = angle + PI / 2.0f;
                float offsetX = widthOffset * std::cos(perpAngle);
                float offsetY = widthOffset * std::sin(perpAngle);
                
                float x = baseRadius * std::cos(angle) + offsetX;
                float y = baseRadius * std::sin(angle) + offsetY;
                
                vertices.push_back(vec3(x, y, 0.0f));
            }
        }
    }
    
    // 外围光晕 - 稀疏的外围结构
    const int haloPoints = 150;
    for (int i = 0; i < haloPoints; i++) {
        float angle = 2.0f * PI * i / haloPoints;
        float radius = 0.85f + 0.1f * std::sin(3.0f * angle);
        
        // 稀疏分布
        if (i % 4 == 0) {
            float x = radius * std::cos(angle);
            float y = radius * std::sin(angle);
            vertices.push_back(vec3(x, y, 0.0f));
        }
    }
    
    return vertices;
}

// 生成分形树图案
std::vector<vec3> generateFractalTree() {
    std::vector<vec3> vertices;
    
    std::function<void(float, float, float, float, float, int, float, float)> drawBranch = 
        [&](float x1, float y1, float angle, float branchLength, float thickness, int depth, float windEffect, float naturalVariation) {
        if (depth <= 0 || branchLength < 0.008f) return;
        
        // 添加自然弯曲效果
        float curvature = 0.15f * std::sin(depth * 0.7f + naturalVariation);
        float midAngle = angle + curvature * 0.5f;
        float endAngle = angle + curvature;
        
        float x2 = x1 + branchLength * std::cos(endAngle + windEffect * 0.08f);
        float y2 = y1 + branchLength * std::sin(endAngle + windEffect * 0.08f);
        
        // 绘制弯曲的树枝主体
        int branchLines = std::max(1, (int)(thickness * 12));
        for (int line = 0; line < branchLines; line++) {
            float offset = (line - branchLines/2.0f) * thickness * 0.015f;
            
            // 创建弯曲的分支
            int numSegments = std::max(3, (int)(branchLength * 80));
            for (int i = 0; i <= numSegments; i++) {
                float t = (float)i / numSegments;
                float currentAngle = angle + curvature * t;
                float segmentLength = branchLength * t;
                
                float perpX = -std::sin(currentAngle);
                float perpY = std::cos(currentAngle);
                
                float x = x1 + segmentLength * std::cos(currentAngle);
                float y = y1 + segmentLength * std::sin(currentAngle);
                x += perpX * offset;
                y += perpY * offset;
                
                vertices.push_back(vec3(x, y, 0.0f));
            }
        }
        
        // 添加更精美的叶子群
        if (depth <= 4 && depth > 0) {
            int numLeafClusters = 1 + depth / 2;
            for (int cluster = 0; cluster < numLeafClusters; cluster++) {
                float clusterT = 0.3f + 0.7f * cluster / std::max(1.0f, (float)(numLeafClusters - 1));
                float clusterX = x1 + (x2 - x1) * clusterT;
                float clusterY = y1 + (y2 - y1) * clusterT;
                
                // 每个叶子群包含多片叶子
                int leavesPerCluster = 3 + (depth % 3);
                for (int leaf = 0; leaf < leavesPerCluster; leaf++) {
                    float leafAngleOffset = (leaf - leavesPerCluster/2.0f) * 0.8f;
                    float leafDistance = 0.02f + 0.01f * std::sin(leaf * 2.0f);
                    
                    float leafCenterX = clusterX + leafDistance * std::cos(angle + leafAngleOffset);
                    float leafCenterY = clusterY + leafDistance * std::sin(angle + leafAngleOffset);
                    
                    // 精美的叶子形状
                    int leafPoints = 8;
                    float leafSize = 0.012f * (0.8f + 0.4f * std::sin(leaf * 1.5f + depth));
                    for (int p = 0; p < leafPoints; p++) {
                        float leafAngle = 2.0f * PI * p / leafPoints;
                        float leafRadius = leafSize * (0.6f + 0.4f * std::cos(3.0f * leafAngle));
                        float lx = leafCenterX + leafRadius * std::cos(leafAngle + windEffect);
                        float ly = leafCenterY + leafRadius * std::sin(leafAngle + windEffect) * 0.7f;
                        vertices.push_back(vec3(lx, ly, 0.0f));
                    }
                }
            }
        }
        
        // 递归绘制子分支 - 更大的角度变化和更多分支
        if (depth > 1) {
            // 主要分支 - 更大的角度分散
            float baseAngleVariation = PI / 4.5f + 0.15f * std::sin(depth * 0.9f + naturalVariation);
            float asymmetry = 0.1f * std::sin(depth * 1.3f + naturalVariation * 2.0f);
            
            float leftAngle = angle + baseAngleVariation + asymmetry;
            float rightAngle = angle - baseAngleVariation + asymmetry * 0.5f;
            float newLength = branchLength * (0.62f + 0.08f * std::sin(depth * 0.6f + naturalVariation));
            float newThickness = thickness * 0.72f;
            float newVariation = naturalVariation + 0.3f * depth;
            
            drawBranch(x2, y2, leftAngle, newLength, newThickness, depth - 1, windEffect, newVariation);
            drawBranch(x2, y2, rightAngle, newLength, newThickness, depth - 1, windEffect, newVariation + 1.0f);
            
            // 中间分支 - 增加复杂性
            if (depth > 2) {
                float middleAngle = angle + 0.08f * std::sin(depth * 1.8f + naturalVariation);
                float middleLength = newLength * (0.7f + 0.2f * std::sin(depth + naturalVariation));
                drawBranch(x2, y2, middleAngle, middleLength, newThickness * 0.6f, depth - 2, windEffect, newVariation + 2.0f);
            }
            
            // 额外的侧分支 - 增加分散效果
            if (depth > 3) {
                float sideAngle1 = angle + PI/2.5f + 0.3f * std::sin(depth * 0.8f + naturalVariation);
                float sideAngle2 = angle - PI/2.8f + 0.25f * std::cos(depth * 1.1f + naturalVariation);
                float sideLength = newLength * 0.45f;
                
                drawBranch(x2, y2, sideAngle1, sideLength, newThickness * 0.4f, depth - 3, windEffect, newVariation + 3.0f);
                drawBranch(x2, y2, sideAngle2, sideLength, newThickness * 0.4f, depth - 3, windEffect, newVariation + 4.0f);
            }
            
            // 细小装饰分支
            if (depth > 4 && depth % 2 == 1) {
                for (int tiny = 0; tiny < 2; tiny++) {
                    float tinyAngle = angle + (tiny - 0.5f) * PI/1.8f + 0.4f * std::sin(depth * 2.0f + naturalVariation + tiny);
                    float tinyLength = newLength * 0.25f;
                    drawBranch(x2, y2, tinyAngle, tinyLength, newThickness * 0.2f, depth - 4, windEffect, newVariation + 5.0f + tiny);
                }
            }
        }
    };
    
    // 树根部分已移至单独的函数generateFractalTreeRoots()
    
    // 多个主干 - 创建更分散的树冠
    float windEffect = 0.06f * std::sin(1.5f);
    
    // 中央主干
    drawBranch(0.0f, -0.85f, PI/2, 0.45f, 1.2f, 9, windEffect, 0.0f);
    
    // 左侧主要分支
    drawBranch(-0.08f, -0.7f, PI/2 + 0.35f, 0.38f, 0.9f, 7, windEffect, 1.5f);
    drawBranch(-0.12f, -0.55f, PI/2 + 0.5f, 0.32f, 0.7f, 6, windEffect, 2.8f);
    
    // 右侧主要分支
    drawBranch(0.08f, -0.7f, PI/2 - 0.35f, 0.38f, 0.9f, 7, windEffect, 3.2f);
    drawBranch(0.12f, -0.55f, PI/2 - 0.5f, 0.32f, 0.7f, 6, windEffect, 4.1f);
    
    // 额外的分散分支
    drawBranch(-0.15f, -0.4f, PI/2 + 0.8f, 0.25f, 0.5f, 5, windEffect, 5.5f);
    drawBranch(0.15f, -0.4f, PI/2 - 0.8f, 0.25f, 0.5f, 5, windEffect, 6.3f);
    
    return vertices;
}

// 生成分形树根部图案
std::vector<vec3> generateFractalTreeRoots() {
    std::vector<vec3> vertices;
    
    // 更分散的根系统
    int numMainRoots = 7;
    for (int root = 0; root < numMainRoots; root++) {
        float rootSpread = 1.2f; // 增加根系分散度
        float rootAngle = -PI/2 + (root - numMainRoots/2.0f) * rootSpread / numMainRoots;
        float rootLength = 0.15f + 0.05f * std::sin(root * 1.8f);
        float rootX = rootLength * std::cos(rootAngle);
        float rootY = -0.85f + rootLength * std::sin(rootAngle);
        
        // 主根 - 更自然的弯曲
        int rootPoints = 20;
        for (int i = 0; i <= rootPoints; i++) {
            float t = (float)i / rootPoints;
            float curvature = 0.1f * std::sin(t * PI + root);
            float x = t * rootX + curvature * 0.05f;
            float y = -0.85f + t * (rootY + 0.85f);
            vertices.push_back(vec3(x, y, 0.0f));
        }
        
        // 根的分支网络
        for (int subRoot = 0; subRoot < 3; subRoot++) {
            float subAngle = rootAngle + (subRoot - 1.0f) * 0.6f;
            float subLength = rootLength * (0.4f + 0.2f * subRoot);
            float subX = rootX + subLength * std::cos(subAngle);
            float subY = rootY + subLength * std::sin(subAngle);
            
            int subRootPoints = 12;
            for (int i = 0; i <= subRootPoints; i++) {
                float t = (float)i / subRootPoints;
                float x = rootX + t * (subX - rootX);
                float y = rootY + t * (subY - rootY);
                vertices.push_back(vec3(x, y, 0.0f));
            }
        }
    }
    
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

// 生成酷炫爱心图案
std::vector<vec3> generateButterfly() {
    std::vector<vec3> vertices;
    
    // 主爱心轮廓 - 使用心形参数方程
    const int heartOutlinePoints = 200;
    for (int i = 0; i < heartOutlinePoints; i++) {
        float t = (float)i / heartOutlinePoints * 2.0f * PI;
        
        // 心形参数方程: x = 16sin³(t), y = 13cos(t) - 5cos(2t) - 2cos(3t) - cos(4t)
        float scale = 0.03f; // 缩放因子
        float x = scale * 16.0f * std::pow(std::sin(t), 3);
        float y = scale * (13.0f * std::cos(t) - 5.0f * std::cos(2.0f * t) - 2.0f * std::cos(3.0f * t) - std::cos(4.0f * t));
        
        vertices.push_back(vec3(x, y, 0.0f));
    }
    
    // 内层爱心 - 稍小一些，创造层次感
    const int innerHeartPoints = 150;
    for (int layer = 1; layer <= 3; layer++) {
        float layerScale = 0.025f - layer * 0.005f; // 逐层缩小
        
        for (int i = 0; i < innerHeartPoints; i++) {
            float t = (float)i / innerHeartPoints * 2.0f * PI;
            
            float x = layerScale * 16.0f * std::pow(std::sin(t), 3);
            float y = layerScale * (13.0f * std::cos(t) - 5.0f * std::cos(2.0f * t) - 2.0f * std::cos(3.0f * t) - std::cos(4.0f * t));
            
            vertices.push_back(vec3(x, y, 0.0f));
        }
    }
    
    // 爱心填充 - 密集的内部点创造实心效果
    const int fillPoints = 300;
    for (int i = 0; i < fillPoints; i++) {
        float t = (float)i / fillPoints * 2.0f * PI;
        
        // 多个半径层次的填充
        for (int r = 1; r <= 8; r++) {
            float radiusScale = 0.02f * r / 8.0f;
            float x = radiusScale * 16.0f * std::pow(std::sin(t), 3);
            float y = radiusScale * (13.0f * std::cos(t) - 5.0f * std::cos(2.0f * t) - 2.0f * std::cos(3.0f * t) - std::cos(4.0f * t));
            
            vertices.push_back(vec3(x, y, 0.0f));
        }
    }
    
    // 爱心周围的粒子效果 - 创造浪漫氛围
    const int particleRings = 5;
    const int particlesPerRing = 60;
    
    for (int ring = 0; ring < particleRings; ring++) {
        float ringRadius = 0.6f + ring * 0.15f; // 粒子环半径
        
        for (int i = 0; i < particlesPerRing; i++) {
            float angle = (float)i / particlesPerRing * 2.0f * PI;
            
            // 添加一些随机性让粒子分布更自然
            float randomOffset = 0.05f * std::sin(angle * 7.0f + ring * 2.0f);
            float radius = ringRadius + randomOffset;
            
            float x = radius * std::cos(angle);
            float y = radius * std::sin(angle);
            
            vertices.push_back(vec3(x, y, 0.0f));
            
            // 为每个粒子添加小的装饰点
            const int decorPoints = 4;
            for (int d = 0; d < decorPoints; d++) {
                float decorAngle = d * PI / 2.0f;
                float decorRadius = 0.02f;
                float decorX = x + decorRadius * std::cos(decorAngle);
                float decorY = y + decorRadius * std::sin(decorAngle);
                vertices.push_back(vec3(decorX, decorY, 0.0f));
            }
        }
    }
    
    // 爱心中心的闪烁效果 - 多层星形图案
    const int starLayers = 4;
    for (int layer = 0; layer < starLayers; layer++) {
        const int starPoints = 8;
        float starRadius = 0.08f + layer * 0.03f;
        
        for (int i = 0; i < starPoints; i++) {
            float angle = (float)i / starPoints * 2.0f * PI;
            
            // 创造星形的尖角效果
            float radius = (i % 2 == 0) ? starRadius : starRadius * 0.5f;
            float x = radius * std::cos(angle);
            float y = radius * std::sin(angle);
            
            vertices.push_back(vec3(x, y, 0.0f));
            
            // 连接线创造更复杂的图案
            if (i % 2 == 0) {
                const int connectPoints = 5;
                for (int c = 1; c < connectPoints; c++) {
                    float t = (float)c / connectPoints;
                    float connX = x * t;
                    float connY = y * t;
                    vertices.push_back(vec3(connX, connY, 0.0f));
                }
            }
        }
    }
    
    // 爱心顶部的装饰弧线
    const int arcDecorations = 3;
    for (int arc = 0; arc < arcDecorations; arc++) {
        const int arcPoints = 40;
        float arcHeight = 0.4f + arc * 0.1f;
        float arcWidth = 0.3f + arc * 0.05f;
        
        for (int i = 0; i < arcPoints; i++) {
            float t = (float)i / (arcPoints - 1);
            float angle = (t - 0.5f) * PI; // -π/2 到 π/2
            
            float x = arcWidth * std::sin(angle);
            float y = arcHeight + 0.1f * std::cos(3.0f * angle); // 添加波浪效果
            
            vertices.push_back(vec3(x, y, 0.0f));
        }
    }
    
    // 爱心底部的光芒效果
    const int rayCount = 12;
    for (int ray = 0; ray < rayCount; ray++) {
        float rayAngle = (float)ray / rayCount * 2.0f * PI;
        const int rayPoints = 15;
        
        for (int i = 0; i < rayPoints; i++) {
            float t = (float)i / rayPoints;
            float rayLength = 0.3f + 0.2f * std::sin(rayAngle * 3.0f); // 变化的光芒长度
            
            float x = t * rayLength * std::cos(rayAngle);
            float y = -0.4f + t * rayLength * std::sin(rayAngle); // 从爱心底部发出
            
            // 光芒强度随距离衰减
            if (t > 0.7f) {
                vertices.push_back(vec3(x, y, 0.0f));
            }
        }
    }
    
    // 环绕爱心的螺旋装饰
    const int spiralTurns = 3;
    const int spiralPoints = 200;
    
    for (int turn = 0; turn < spiralTurns; turn++) {
        for (int i = 0; i < spiralPoints; i++) {
            float t = (float)i / spiralPoints;
            float totalAngle = t * 2.0f * PI + turn * 2.0f * PI;
            
            float spiralRadius = 0.5f + 0.3f * t;
            float x = spiralRadius * std::cos(totalAngle);
            float y = spiralRadius * std::sin(totalAngle);
            
            // 只在特定位置添加螺旋点，创造断续效果
            if (i % 3 == 0) {
                vertices.push_back(vec3(x, y, 0.0f));
            }
        }
    }
    
    // 爱心周围的小心形装饰
    const int smallHearts = 8;
    for (int heart = 0; heart < smallHearts; heart++) {
        float heartAngle = (float)heart / smallHearts * 2.0f * PI;
        float heartDistance = 0.8f;
        
        float centerX = heartDistance * std::cos(heartAngle);
        float centerY = heartDistance * std::sin(heartAngle);
        
        // 小心形
        const int smallHeartPoints = 20;
        for (int i = 0; i < smallHeartPoints; i++) {
            float t = (float)i / smallHeartPoints * 2.0f * PI;
            float smallScale = 0.008f; // 很小的心形
            
            float x = centerX + smallScale * 16.0f * std::pow(std::sin(t), 3);
            float y = centerY + smallScale * (13.0f * std::cos(t) - 5.0f * std::cos(2.0f * t) - 2.0f * std::cos(3.0f * t) - std::cos(4.0f * t));
            
            vertices.push_back(vec3(x, y, 0.0f));
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
    
    // 初始化分形树根部图案
    std::vector<vec3> treeRootsVertices = generateFractalTreeRoots();
    vertexCounts[FRACTAL_TREE_ROOTS_VAO] = treeRootsVertices.size();
    glBindVertexArray(vaos[FRACTAL_TREE_ROOTS_VAO]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[FRACTAL_TREE_ROOTS_VAO]);
    glBufferData(GL_ARRAY_BUFFER, treeRootsVertices.size() * sizeof(vec3), treeRootsVertices.data(), GL_STATIC_DRAW);
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
            

            

            
        case 1: // 螺旋星系
            glBindVertexArray(vaos[SPIRAL_GALAXY_VAO]);
            glUniform3f(colorLocation, PURPLE_R, PURPLE_G, PURPLE_B);
            glDrawArrays(GL_POINTS, 0, vertexCounts[SPIRAL_GALAXY_VAO]);
            break;
            
        case 2: // 分形树
            // 先绘制棕色树根
            glBindVertexArray(vaos[FRACTAL_TREE_ROOTS_VAO]);
            glUniform3f(colorLocation, BROWN_R, BROWN_G, BROWN_B);
            glDrawArrays(GL_POINTS, 0, vertexCounts[FRACTAL_TREE_ROOTS_VAO]);
            
            // 再绘制绿色树冠
            glBindVertexArray(vaos[FRACTAL_TREE_VAO]);
            glUniform3f(colorLocation, GREEN_R, GREEN_G, GREEN_B);
            glDrawArrays(GL_POINTS, 0, vertexCounts[FRACTAL_TREE_VAO]);
            break;
            
        case 3: // 玫瑰曲线
            glBindVertexArray(vaos[ROSE_CURVE_VAO]);
            glUniform3f(colorLocation, ORANGE_R, ORANGE_G, ORANGE_B);
            glDrawArrays(GL_POINTS, 0, vertexCounts[ROSE_CURVE_VAO]);
            break;
            
        case 4: // 蝴蝶
            glBindVertexArray(vaos[BUTTERFLY_VAO]);
            glUniform3f(colorLocation, PINK_R, PINK_G, PINK_B);
            glDrawArrays(GL_POINTS, 0, vertexCounts[BUTTERFLY_VAO]);
            break;
            
        case 5: // 雪花图案
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
            std::cout << "切换到螺旋星系图案" << std::endl;
            break;
        case GLFW_KEY_3:
            currentPattern = 2;
            std::cout << "切换到分形树图案" << std::endl;
            break;
        case GLFW_KEY_4:
            currentPattern = 3;
            std::cout << "切换到玫瑰曲线图案" << std::endl;
            break;
        case GLFW_KEY_5:
            currentPattern = 4;
            std::cout << "切换到蝴蝶图案" << std::endl;
            break;
        case GLFW_KEY_6:
            currentPattern = 5;
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
    std::cout << "按键2: 螺旋星系图案 (紫色)" << std::endl;
    std::cout << "按键3: 分形树图案 (绿色)" << std::endl;
    std::cout << "按键4: 玫瑰曲线图案 (橙色)" << std::endl;
    std::cout << "按键5: 酷炫爱心图案 (粉色)" << std::endl;
    std::cout << "按键6: 雪花图案 (淡蓝白色)" << std::endl;
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
