// ==================== 头文件包含 ====================
#include "Angel.h"      // OpenGL相关的头文件，包含GLFW、GLAD、GLM等
#include "TriMesh.h"    // 三角网格类，用于处理3D模型的顶点和面片数据

// 标准库头文件
#include <vector>       // 动态数组容器
#include <string>       // 字符串处理
#include <iostream>     // 输入输出流
#include <cstdio>       // C标准输入输出

// ==================== 常量定义 ====================
// 坐标轴枚举常量，用于标识X、Y、Z轴
const int X_AXIS = 0;   // X轴索引
const int Y_AXIS = 1;   // Y轴索引
const int Z_AXIS = 2;   // Z轴索引

// 变换类型枚举常量，用于标识当前的变换模式
const int TRANSFORM_SCALE = 0;      // 缩放变换模式
const int TRANSFORM_ROTATE = 1;     // 旋转变换模式
const int TRANSFORM_TRANSLATE = 2;  // 平移变换模式

// 变换参数控制常量
const double DELTA_DELTA = 0.3;     // Delta值的变化率，用于调整变换的增量步长
const double DEFAULT_DELTA = 0.5;   // 默认的Delta值，控制变换的基础步长

// ==================== 全局变量定义 ====================

// 变换增量控制变量 - 控制每次变换的步长大小
double scaleDelta = DEFAULT_DELTA;      // 缩放变换的增量步长
double rotateDelta = DEFAULT_DELTA;     // 旋转变换的增量步长（度数）
double translateDelta = DEFAULT_DELTA;  // 平移变换的增量步长

// 变换累积量 - 记录当前的变换状态
glm::vec3 scaleTheta(1.0, 1.0, 1.0);       // 缩放控制变量，初始值为(1,1,1)表示无缩放
glm::vec3 rotateTheta(0.0, 0.0, 0.0);      // 旋转控制变量，分别表示绕X、Y、Z轴的旋转角度（度）
glm::vec3 translateTheta(0.0, 0.0, 0.0);   // 平移控制变量，分别表示在X、Y、Z轴上的平移距离

// 当前变换模式和窗口句柄
int currentTransform = TRANSFORM_ROTATE;    // 当前变换模式，默认为旋转模式
int mainWindow;                             // 主窗口句柄（保留变量）

// ==================== 鼠标交互相关变量 ====================
// 鼠标状态控制
bool mousePressed = false;          // 鼠标左键是否处于按下状态
double lastMouseX = 0.0;            // 上一帧鼠标的X坐标（窗口坐标系）
double lastMouseY = 0.0;            // 上一帧鼠标的Y坐标（窗口坐标系）
double currentMouseX = 0.0;         // 当前帧鼠标的X坐标（窗口坐标系）
double currentMouseY = 0.0;         // 当前帧鼠标的Y坐标（窗口坐标系）

// 窗口尺寸信息
int windowWidth = 600;              // 窗口宽度（像素），用于坐标系转换
int windowHeight = 600;             // 窗口高度（像素），用于坐标系转换

// ==================== OpenGL对象结构体定义 ====================
/**
 * @brief OpenGL渲染对象结构体
 * 
 * 该结构体封装了渲染一个3D对象所需的所有OpenGL资源，
 * 包括顶点数据、着色器程序和uniform变量位置等。
 */
struct openGLObject
{
	// ========== 顶点数据相关 ==========
	GLuint vao;                 // 顶点数组对象(VAO)，管理顶点属性配置
	GLuint vbo;                 // 顶点缓冲对象(VBO)，存储顶点数据

	// ========== 着色器相关 ==========
	GLuint program;             // 着色器程序对象，链接顶点着色器和片元着色器
	std::string vshader;        // 顶点着色器文件路径
	std::string fshader;        // 片元着色器文件路径
	
	// ========== 着色器变量位置 ==========
	GLuint pLocation;           // 顶点位置属性在着色器中的位置(vPosition)
	GLuint cLocation;           // 顶点颜色属性在着色器中的位置(vColor)
	GLuint matrixLocation;      // 变换矩阵uniform变量在着色器中的位置(matrix)
	GLuint darkLocation;        // 亮度控制uniform变量在着色器中的位置(brightness)
};

// ==================== 全局对象实例 ====================
openGLObject cube_object;      // 立方体的OpenGL渲染对象实例

TriMesh* cube = new TriMesh(); // 立方体的三角网格数据对象，用于存储顶点和面片信息

// ==================== 回调函数定义 ====================

/**
 * @brief 窗口大小改变回调函数
 * 
 * 当用户调整窗口大小时，GLFW会自动调用此函数。
 * 主要功能：
 * 1. 更新OpenGL视口大小以匹配新的窗口尺寸
 * 2. 更新全局窗口尺寸变量，用于坐标系转换
 * 
 * @param window 窗口句柄
 * @param width  新的窗口宽度（像素）
 * @param height 新的窗口高度（像素）
 */
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// 设置OpenGL视口，确保渲染区域与窗口大小一致
	glViewport(0, 0, width, height);
	
	// 更新全局窗口尺寸变量，用于鼠标坐标转换
	windowWidth = width;
	windowHeight = height;
}

/**
 * @brief 窗口坐标系到OpenGL标准坐标系的转换函数
 * 
 * 坐标系转换说明：
 * - 窗口坐标系：左上角为原点(0,0)，X轴向右递增，Y轴向下递增，单位为像素
 * - OpenGL坐标系：屏幕中心为原点(0,0)，X轴向右递增，Y轴向上递增，范围[-1,1]
 * 
 * @param x 窗口坐标系中的X坐标
 * @param y 窗口坐标系中的Y坐标
 * @return glm::vec2 OpenGL标准坐标系中的坐标
 */
glm::vec2 windowToOpenGL(double x, double y) {
	// X坐标转换：[0, windowWidth] -> [-1, 1]
	float openglX = (2.0f * x / windowWidth) - 1.0f;
	
	// Y坐标转换：[0, windowHeight] -> [1, -1]（注意Y轴方向相反）
	float openglY = 1.0f - (2.0f * y / windowHeight);
	
	return glm::vec2(openglX, openglY);
}

/**
 * @brief 鼠标按钮事件回调函数
 * 
 * 处理鼠标按钮的按下和释放事件，主要用于控制拖拽操作的开始和结束。
 * 
 * @param window 窗口句柄
 * @param button 鼠标按钮标识符（GLFW_MOUSE_BUTTON_LEFT等）
 * @param action 动作类型（GLFW_PRESS按下，GLFW_RELEASE释放）
 * @param mods   修饰键状态（Ctrl、Shift等）
 */
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// 只处理鼠标左键事件
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			// 鼠标按下：开始拖拽操作
			mousePressed = true;
			
			// 获取当前鼠标位置作为拖拽起始点
			glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
			currentMouseX = lastMouseX;
			currentMouseY = lastMouseY;
		}
		else if (action == GLFW_RELEASE) {
			// 鼠标释放：结束拖拽操作
			mousePressed = false;
		}
	}
}

/**
 * @brief 鼠标移动事件回调函数
 * 
 * 当鼠标在窗口内移动时被调用。如果鼠标处于按下状态，则根据当前变换模式
 * 计算鼠标移动量并应用相应的变换（旋转、平移或缩放）。
 * 
 * @param window 窗口句柄
 * @param xpos   鼠标当前X坐标（窗口坐标系）
 * @param ypos   鼠标当前Y坐标（窗口坐标系）
 */
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	// 只在鼠标按下状态时处理拖拽
	if (mousePressed) {
		// 更新当前鼠标位置
		currentMouseX = xpos;
		currentMouseY = ypos;
		
		// 计算鼠标移动的增量
		double deltaX = currentMouseX - lastMouseX;  // 水平移动量
		double deltaY = currentMouseY - lastMouseY;  // 垂直移动量
		
		// 根据当前变换模式应用不同的变换
		switch (currentTransform) {
		case TRANSFORM_ROTATE:
			// 旋转模式：鼠标水平移动控制Y轴旋转，垂直移动控制X轴旋转
			rotateTheta.y += deltaX * 0.5f;  // 水平拖拽 -> Y轴旋转
			rotateTheta.x += deltaY * 0.5f;  // 垂直拖拽 -> X轴旋转
			break;
			
		case TRANSFORM_TRANSLATE:
			// 平移模式：将像素移动转换为OpenGL坐标移动
			translateTheta.x += deltaX * 0.01f;      // X轴平移
			translateTheta.y -= deltaY * 0.01f;      // Y轴平移（注意方向相反）
			break;
			
		case TRANSFORM_SCALE:
			// 缩放模式：鼠标向右上移动增大缩放，向左下移动减小缩放
			float scaleFactor = 1.0f + (deltaX + deltaY) * 0.001f;
			scaleTheta *= scaleFactor;  // 应用缩放因子
			break;
		}
		
		// 更新上一次鼠标位置，为下一帧计算增量做准备
		lastMouseX = currentMouseX;
		lastMouseY = currentMouseY;
	}
}

/**
 * @brief 绑定3D对象数据到OpenGL渲染管线
 * 
 * 该函数是OpenGL渲染管线初始化的核心函数，负责：
 * 1. 创建和配置顶点数组对象(VAO)和顶点缓冲对象(VBO)
 * 2. 将三角网格数据上传到GPU显存
 * 3. 编译和链接着色器程序
 * 4. 配置顶点属性指针
 * 5. 获取uniform变量在着色器中的位置
 * 
 * @param mesh    三角网格对象指针，包含顶点坐标和颜色数据
 * @param object  OpenGL对象引用，用于存储所有OpenGL资源句柄
 * @param vshader 顶点着色器文件路径
 * @param fshader 片元着色器文件路径
 */
void bindObjectAndData(TriMesh* mesh, openGLObject& object, const std::string& vshader, const std::string& fshader) {

	// ========== 第一步：创建和配置顶点数组对象(VAO) ==========
    glGenVertexArrays(1, &object.vao);     // 生成1个顶点数组对象
	glBindVertexArray(object.vao);          // 绑定VAO，后续的顶点属性配置将被记录在此VAO中

	// ========== 第二步：创建和配置顶点缓冲对象(VBO) ==========
	glGenBuffers(1, &object.vbo);           // 生成1个顶点缓冲对象
	glBindBuffer(GL_ARRAY_BUFFER, object.vbo); // 绑定VBO为当前的数组缓冲
	
	// 计算所需的缓冲区大小：顶点坐标数据 + 顶点颜色数据
	size_t bufferSize = mesh->getPoints().size() * sizeof(glm::vec3) + 
	                   mesh->getColors().size() * sizeof(glm::vec3);
	glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_STATIC_DRAW);

	// ========== 第三步：上传顶点数据到GPU显存 ==========
	// 上传顶点坐标数据到缓冲区的前半部分
	glBufferSubData(GL_ARRAY_BUFFER, 0, 
	               mesh->getPoints().size() * sizeof(glm::vec3), 
	               &mesh->getPoints()[0]);
	
	// 上传顶点颜色数据到缓冲区的后半部分
	glBufferSubData(GL_ARRAY_BUFFER, 
	               mesh->getPoints().size() * sizeof(glm::vec3), 
	               mesh->getColors().size() * sizeof(glm::vec3), 
	               &mesh->getColors()[0]);

	// ========== 第四步：编译和链接着色器程序 ==========
	object.vshader = vshader;               // 保存顶点着色器文件路径
	object.fshader = fshader;               // 保存片元着色器文件路径
	object.program = InitShader(object.vshader.c_str(), object.fshader.c_str()); // 编译链接着色器

	// ========== 第五步：配置顶点属性指针 ==========
	// 配置顶点位置属性(vPosition)
	object.pLocation = glGetAttribLocation(object.program, "vPosition"); // 获取位置属性在着色器中的位置
	glEnableVertexAttribArray(object.pLocation);                        // 启用该属性数组
	glVertexAttribPointer(object.pLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0)); // 指定属性数据格式和位置

	// 配置顶点颜色属性(vColor)
	object.cLocation = glGetAttribLocation(object.program, "vColor");    // 获取颜色属性在着色器中的位置
	glEnableVertexAttribArray(object.cLocation);                        // 启用该属性数组
	// 颜色数据从顶点坐标数据之后开始
	glVertexAttribPointer(object.cLocation, 3, GL_FLOAT, GL_FALSE, 0, 
	                     BUFFER_OFFSET(mesh->getPoints().size() * sizeof(glm::vec3)));

	// ========== 第六步：获取uniform变量位置 ==========
	// 获取变换矩阵uniform变量的位置，用于传递模型变换矩阵
	object.matrixLocation = glGetUniformLocation(object.program, "matrix");
	
	// 获取亮度控制uniform变量的位置，用于实现呼吸灯效果
	object.darkLocation = glGetUniformLocation(object.program, "brightness");
}

/**
 * @brief 初始化函数 - 设置OpenGL环境和立方体数据
 * 
 * 该函数负责：
 * 1. 指定着色器文件路径
 * 2. 生成立方体的顶点和面片数据
 * 3. 绑定OpenGL对象和数据到GPU
 * 4. 设置渲染环境（背景色等）
 */
void init()
{
	std::string vshader, fshader;
	// 指定顶点着色器和片元着色器的文件路径
    vshader = "shaders/vshader.glsl";
	fshader = "shaders/fshader.glsl";

	// 生成立方体的顶点坐标、颜色和面片索引数据
	cube->generateCube();
	// 将立方体数据绑定到OpenGL对象，编译着色器程序
	bindObjectAndData(cube, cube_object, vshader, fshader);

	// 设置清屏颜色为黑色 (R=0, G=0, B=0, A=1)
	glClearColor(0.0, 0.0, 0.0, 1.0);
}

/**
 * @brief 渲染函数 - 每帧调用，负责绘制立方体
 * 
 * 该函数的渲染流程：
 * 1. 清空颜色缓冲区和深度缓冲区
 * 2. 激活着色器程序和VAO
 * 3. 计算当前帧的变换矩阵（缩放、旋转、平移的组合）
 * 4. 将变换矩阵传递给着色器的uniform变量
 * 5. 计算并传递亮度控制参数
 * 6. 执行绘制命令渲染立方体
 */
void display()
{
	// 清空颜色缓冲区和深度缓冲区，准备新一帧的渲染
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 激活立方体的着色器程序
	glUseProgram(cube_object.program);

    // 绑定立方体的顶点数组对象(VAO)，激活顶点属性配置
    glBindVertexArray(cube_object.vao);

	// 初始化为4x4单位矩阵，作为变换的起始矩阵
	glm::mat4 m(1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0);

	// ========== 变换矩阵计算 ==========
	// 根据当前的变换参数计算最终的变换矩阵
	// 变换顺序：缩放 -> 旋转 -> 平移（从右到左相乘）
	
	// 创建缩放变换矩阵，使用scaleTheta作为各轴的缩放因子
	glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scaleTheta);
	
	// 创建旋转变换矩阵（分别绕X、Y、Z轴旋转）
	// 注意：glm::rotate需要弧度制角度，所以使用glm::radians转换
	glm::mat4 rotateMatrixX = glm::rotate(glm::mat4(1.0f), glm::radians(rotateTheta.x), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 rotateMatrixY = glm::rotate(glm::mat4(1.0f), glm::radians(rotateTheta.y), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 rotateMatrixZ = glm::rotate(glm::mat4(1.0f), glm::radians(rotateTheta.z), glm::vec3(0.0f, 0.0f, 1.0f));
	// 组合旋转矩阵：先绕X轴，再绕Y轴，最后绕Z轴
	glm::mat4 rotateMatrix = rotateMatrixZ * rotateMatrixY * rotateMatrixX;
	
	// 创建平移变换矩阵，使用translateTheta作为各轴的平移距离
	glm::mat4 translateMatrix = glm::translate(glm::mat4(1.0f), translateTheta);
	
	// 组合最终的变换矩阵：T * R * S（先缩放，再旋转，最后平移）
	m = translateMatrix * rotateMatrix * scaleMatrix;

	// ========== 传递uniform变量到着色器 ==========
	// 将计算好的变换矩阵传递给顶点着色器的matrix uniform变量
	glUniformMatrix4fv(cube_object.matrixLocation, 1, GL_FALSE, glm::value_ptr(m));
	
	// 计算基于时间的亮度值，实现颜色明暗的周期性变化
	float time = glfwGetTime();  // 获取程序运行时间（秒）
	float brightness = (sin(time) + 1.0f) / 2.0f;  // 将sin值从[-1,1]映射到[0,1]
	// 将亮度值传递给片元着色器的brightness uniform变量
	glUniform1f(cube_object.darkLocation, brightness);

	// ========== 执行绘制 ==========
	// 使用三角形图元绘制立方体，顶点数量为cube中存储的所有顶点
	glDrawArrays(GL_TRIANGLES, 0, cube->getPoints().size());
}

/**
 * @brief 更新变换参数函数 - 根据当前变换模式和增量更新对应的变换参数
 * 
 * @param axis 要更新的轴向（X_AXIS=0, Y_AXIS=1, Z_AXIS=2）
 * @param sign 变化方向（1表示增加，-1表示减少）
 * 
 * 该函数根据当前的变换模式（缩放/旋转/平移）和指定的轴向，
 * 使用对应的Delta值来更新相应的Theta参数
 */
void updateTheta(int axis, int sign) {
	switch (currentTransform) {
		// 缩放模式：更新指定轴的缩放因子
	case TRANSFORM_SCALE:
		scaleTheta[axis] += sign * scaleDelta;
		break;
		// 旋转模式：更新指定轴的旋转角度（度数）
	case TRANSFORM_ROTATE:
		rotateTheta[axis] += sign * rotateDelta;
		break;
		// 平移模式：更新指定轴的平移距离
	case TRANSFORM_TRANSLATE:
		translateTheta[axis] += sign * translateDelta;
		break;
	}
}

/**
 * @brief 重置所有变换参数函数 - 将所有变换参数和增量步长恢复到初始状态
 * 
 * 该函数将：
 * - 缩放参数重置为(1,1,1)，表示无缩放
 * - 旋转参数重置为(0,0,0)，表示无旋转
 * - 平移参数重置为(0,0,0)，表示无平移
 * - 所有Delta值重置为默认值
 */
void resetTheta()
{
	// 重置变换参数到初始状态
	scaleTheta = glm::vec3(1.0, 1.0, 1.0);       // 无缩放
	rotateTheta = glm::vec3(0.0, 0.0, 0.0);      // 无旋转
	translateTheta = glm::vec3(0.0, 0.0, 0.0);   // 无平移
	
	// 重置增量步长到默认值
	scaleDelta = DEFAULT_DELTA;
	rotateDelta = DEFAULT_DELTA;
	translateDelta = DEFAULT_DELTA;
}

/**
 * @brief 更新变换增量函数 - 调整当前变换模式的增量步长
 * 
 * @param sign 调整方向（1表示增加步长，-1表示减少步长）
 * 
 * 该函数根据当前的变换模式，调整对应的Delta值，
 * 从而控制每次按键操作时变换的幅度
 */
void updateDelta(int sign)
{
	switch (currentTransform) {
		// 缩放模式：调整缩放增量步长
	case TRANSFORM_SCALE:
		scaleDelta += sign * DELTA_DELTA;
		break;
		// 旋转模式：调整旋转增量步长
	case TRANSFORM_ROTATE:
		rotateDelta += sign * DELTA_DELTA;
		break;
		// 平移模式：调整平移增量步长
	case TRANSFORM_TRANSLATE:
		translateDelta += sign * DELTA_DELTA;
		break;
	}
}

/**
 * @brief 键盘事件回调函数 - 处理用户的键盘输入，控制立方体的变换和渲染模式
 * 
 * @param window GLFW窗口指针
 * @param key 按下的键码
 * @param scancode 系统特定的扫描码
 * @param action 按键动作（按下/释放/重复）
 * @param mode 修饰键状态（Ctrl、Shift等）
 * 
 * 键盘控制说明：
 * - ESC: 退出程序
 * - 1/2/3: 切换变换模式（缩放/旋转/平移）
 * - 4/5: 切换渲染模式（线框/填充）
 * - Q/A: 控制X轴变换（增加/减少）
 * - W/S: 控制Y轴变换（增加/减少）
 * - E/D: 控制Z轴变换（增加/减少）
 * - R/F: 调整变换步长（增加/减少）
 * - T: 重置所有变换参数
 */
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	switch (key)
	{	
		// ========== 程序控制 ==========
		// ESC键：退出程序
		case GLFW_KEY_ESCAPE:
			if(action==GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);
			break;
			
		// ========== 变换模式切换 ==========
		// 数字键1：切换到缩放变换模式
		case GLFW_KEY_1:
			if(action==GLFW_PRESS) currentTransform = TRANSFORM_SCALE;
			break;
		// 数字键2：切换到旋转变换模式
		case GLFW_KEY_2:
			if(action==GLFW_PRESS) currentTransform = TRANSFORM_ROTATE;
			break;
		// 数字键3：切换到平移变换模式
		case GLFW_KEY_3:
			if(action==GLFW_PRESS) currentTransform = TRANSFORM_TRANSLATE;
			break;
			
		// ========== 渲染模式切换 ==========
		// 数字键4：切换到线框渲染模式
		case GLFW_KEY_4:
			if(action==GLFW_PRESS) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		// 数字键5：切换到填充渲染模式
		case GLFW_KEY_5:
			if(action==GLFW_PRESS) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);;
			break;
			
		// ========== X轴变换控制 ==========
		// Q键：增加X轴变换值
		case GLFW_KEY_Q:
			if(action==GLFW_PRESS || action==GLFW_REPEAT) updateTheta(X_AXIS, 1);
			break;
		// A键：减少X轴变换值
		case GLFW_KEY_A:
			if(action==GLFW_PRESS || action==GLFW_REPEAT) updateTheta(X_AXIS, -1);
			break;
			
		// ========== Y轴变换控制 ==========
		// W键：增加Y轴变换值
		case GLFW_KEY_W:
			if(action==GLFW_PRESS || action==GLFW_REPEAT) updateTheta(Y_AXIS, 1);
			break;
		// S键：减少Y轴变换值
		case GLFW_KEY_S:
			if(action==GLFW_PRESS || action==GLFW_REPEAT) updateTheta(Y_AXIS, -1);
			break;
			
		// ========== Z轴变换控制 ==========
		// E键：增加Z轴变换值
		case GLFW_KEY_E:
			if(action==GLFW_PRESS || action==GLFW_REPEAT) updateTheta(Z_AXIS, 1);
			break;
		// D键：减少Z轴变换值
		case GLFW_KEY_D:
			if(action==GLFW_PRESS || action==GLFW_REPEAT) updateTheta(Z_AXIS, -1);
			break;
			
		// ========== 变换步长控制 ==========
		// R键：增加当前变换模式的步长
		case GLFW_KEY_R:
			if(action==GLFW_PRESS) updateDelta(1);
			break;
		// F键：减少当前变换模式的步长
		case GLFW_KEY_F:
			if(action==GLFW_PRESS) updateDelta(-1);
			break;
			
		// ========== 重置控制 ==========
		// T键：重置所有变换参数和步长到初始状态
		case GLFW_KEY_T:
			if(action==GLFW_PRESS) resetTheta();
			break;
	}
}

/**
 * @brief 打印帮助信息函数 - 向控制台输出程序的使用说明
 * 
 * 该函数在程序启动时调用，向用户展示所有可用的键盘控制选项，
 * 包括变换模式切换、轴向控制、步长调整和重置功能的说明
 */
void printHelp() {
	printf("%s\n\n", "3D Transfomations");
	printf("Keyboard options:\n");
	printf("1: Transform Scale\n");           // 切换到缩放变换模式
	printf("2: Transform Rotate\n");          // 切换到旋转变换模式
	printf("3: Transform Translate\n");       // 切换到平移变换模式
	printf("q: Increase x\n");                // 增加X轴变换值
	printf("a: Decrease x\n");                // 减少X轴变换值
	printf("w: Increase y\n");                // 增加Y轴变换值
	printf("s: Decrease y\n");                // 减少Y轴变换值
	printf("e: Increase z\n");                // 增加Z轴变换值
	printf("d: Decrease z\n");                // 减少Z轴变换值
	printf("r: Increase delta of currently selected transform\n");  // 增加当前变换模式的步长
	printf("f: Decrease delta of currently selected transform\n");  // 减少当前变换模式的步长
	printf("t: Reset all transformations and deltas\n");           // 重置所有变换参数和步长
}

/**
 * @brief 清理数据函数 - 释放程序中分配的所有资源
 * 
 * 该函数在程序结束前调用，负责：
 * 1. 清理TriMesh对象中的数据
 * 2. 释放动态分配的内存
 * 3. 删除OpenGL对象（VAO、VBO、着色器程序）
 * 4. 防止内存泄漏
 */
void cleanData() {
	// 清理立方体网格数据
	cube->cleanData();

	// 释放动态分配的TriMesh对象内存
	delete cube;
	cube = NULL;

	// 删除OpenGL对象，释放GPU资源
    glDeleteVertexArrays(1, &cube_object.vao);    // 删除顶点数组对象
	glDeleteBuffers(1, &cube_object.vbo);         // 删除顶点缓冲对象
	glDeleteProgram(cube_object.program);         // 删除着色器程序
}

/**
 * @brief 主函数 - 程序入口点，负责初始化OpenGL环境和运行主循环
 * 
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return int 程序退出状态码（0表示正常退出，-1表示错误）
 * 
 * 主函数的执行流程：
 * 1. 初始化GLFW库和OpenGL上下文
 * 2. 创建窗口并配置回调函数
 * 3. 初始化GLAD加载OpenGL函数指针
 * 4. 调用init()函数设置渲染环境
 * 5. 进入主循环，持续渲染和处理事件
 * 6. 程序结束时清理资源
 */
int main(int argc, char** argv)
{
	// ========== GLFW初始化和配置 ==========
	// 初始化GLFW库，这是使用GLFW的第一步
	glfwInit();

	// 配置OpenGL版本为3.3 Core Profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);  // 主版本号
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);  // 次版本号
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 核心模式

	// macOS系统需要额外的前向兼容设置
	#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif

	// ========== 窗口创建和配置 ==========
	// 创建600x600像素的窗口
	GLFWwindow* window = glfwCreateWindow(600, 600, "李文俊-2023150001-实验2.3", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	
	// 将窗口的上下文设置为当前线程的主上下文
	glfwMakeContextCurrent(window);
	
	// ========== 回调函数设置 ==========
	glfwSetKeyCallback(window, key_callback);                    // 键盘事件回调
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);  // 窗口大小改变回调
	glfwSetMouseButtonCallback(window, mouse_button_callback);   // 鼠标按键回调
	glfwSetCursorPosCallback(window, cursor_position_callback);  // 鼠标移动回调

	// ========== GLAD初始化 ==========
	// 初始化GLAD，加载OpenGL函数指针
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// ========== 程序初始化 ==========
	init();          // 初始化OpenGL环境和立方体数据
	printHelp();     // 输出键盘控制帮助信息
	glEnable(GL_DEPTH_TEST);  // 启用深度测试，确保3D渲染的正确性
	
	// ========== 主渲染循环 ==========
	// 持续运行直到用户关闭窗口
	while (!glfwWindowShouldClose(window))
	{
		display();  // 执行渲染

		// 交换前后缓冲区（双缓冲技术，避免闪烁）
		glfwSwapBuffers(window);
		// 检查并处理事件（键盘输入、鼠标移动等）
		glfwPollEvents();
	}
	
	// ========== 程序清理 ==========
	cleanData();  // 清理分配的资源

	return 0;  // 正常退出
}