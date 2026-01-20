/**
 * 实验3.1 相机定位和投影
 * 
 * 实验目标：
 * 1. 了解OpenGL中相机的模型视图变换的基本原理
 * 2. 掌握OpenGL中相机观察变换矩阵的推导
 * 3. 掌握在OpenGL中实现相机观察变换
 * 4. 了解OpenGL中正交投影和透视投影变换
 * 5. 了解在OpenGL中实现正交投影和透视投影变换
 * 
 * 程序功能：
 * 在一个窗口中显示四个视口，分别展示：
 * - 左上角：相机1的视角（默认视角）
 * - 右上角：相机2的视角（默认视角）  
 * - 左下角：相机3的视角（正交投影）
 * - 右下角：相机4的视角（透视投影）
 * 
 * 键盘控制：
 * X/Shift+X: 增加/减少方位角（绕Y轴旋转）
 * Y/Shift+Y: 增加/减少仰角（上下旋转）
 * R/Shift+R: 增加/减少相机距离
 * F/Shift+F: 增加/减少透视投影视角
 * A/Shift+A: 增加/减少透视投影宽高比
 * S/Shift+S: 增加/减少正交投影缩放
 * SPACE: 重置所有参数
 */

#include "Angel.h"
#include "TriMesh.h"
#include "Camera.h"
#include <vector>
#include <string>

// 窗口尺寸常量
int WIDTH = 600;
int HEIGHT = 600;
int mainWindow;

/**
 * OpenGL对象结构体
 * 封装了渲染一个对象所需的所有OpenGL资源
 */
struct openGLObject
{
	// 顶点数组对象 (VAO)
	// 存储顶点属性的配置状态
	GLuint vao;
	
	// 顶点缓存对象 (VBO)  
	// 存储顶点数据（位置、颜色等）
	GLuint vbo;

	// 着色器程序对象
	// 编译链接后的着色器程序
	GLuint program;
	
	// 着色器源文件路径
	std::string vshader;  // 顶点着色器
	std::string fshader;  // 片段着色器
	
	// 着色器中的attribute变量位置
	GLuint pLocation;  // 位置属性
	GLuint cLocation;  // 颜色属性

	// 着色器中的uniform变量位置
	// 用于传递变换矩阵到着色器
	GLuint modelLocation;      // 模型变换矩阵
	GLuint viewLocation;       // 观察变换矩阵  
	GLuint projectionLocation; // 投影变换矩阵
};

// 全局对象实例
openGLObject cube_object;  // 立方体的OpenGL对象

// 几何对象
TriMesh *cube = new TriMesh();  // 立方体网格对象

// 四个相机实例，对应四个不同的视口
Camera* camera_1 = new Camera();  // 左上角视口相机
Camera* camera_2 = new Camera();  // 右上角视口相机  
Camera* camera_3 = new Camera();  // 左下角视口相机（正交投影）
Camera *camera_4 = new Camera();  // 右下角视口相机（透视投影）

/**
 * bindObjectAndData函数：绑定几何对象数据到OpenGL
 * 
 * 功能说明：
 * 1. 创建并配置VAO和VBO
 * 2. 将顶点数据上传到GPU
 * 3. 编译和链接着色器程序
 * 4. 获取着色器变量的位置
 * 
 * @param mesh 三角网格对象，包含顶点和颜色数据
 * @param object OpenGL对象，存储相关的OpenGL资源ID
 * @param vshader 顶点着色器文件路径
 * @param fshader 片段着色器文件路径
 */
void bindObjectAndData(TriMesh* mesh, openGLObject& object, const std::string &vshader, const std::string &fshader) {

	// 步骤1：创建并绑定顶点数组对象(VAO)
	// VAO用于存储顶点属性的配置状态
    glGenVertexArrays(1, &object.vao);  	// 分配1个顶点数组对象
	glBindVertexArray(object.vao);  	// 绑定顶点数组对象，后续的顶点属性配置将存储在此VAO中

	// 步骤2：创建并配置顶点缓存对象(VBO)
	// VBO用于在GPU内存中存储顶点数据
	glGenBuffers(1, &object.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, object.vbo);
	
	// 分配缓存空间：顶点位置数据 + 顶点颜色数据
	glBufferData(GL_ARRAY_BUFFER, 
		mesh->getPoints().size() * sizeof(glm::vec3) + mesh->getColors().size() * sizeof(glm::vec3),
		NULL,  // 暂时不传入数据，只分配空间
		GL_STATIC_DRAW);  // 数据不会经常改变

	// 步骤3：上传顶点数据到GPU
	// 先上传顶点位置数据到缓存的前半部分
	glBufferSubData(GL_ARRAY_BUFFER, 0, mesh->getPoints().size() * sizeof(glm::vec3), &mesh->getPoints()[0]);
	// 再上传顶点颜色数据到缓存的后半部分
	glBufferSubData(GL_ARRAY_BUFFER, mesh->getPoints().size() * sizeof(glm::vec3), mesh->getColors().size() * sizeof(glm::vec3), &mesh->getColors()[0]);

	// 步骤4：编译和链接着色器程序
	object.vshader = vshader;
	object.fshader = fshader;
	object.program = InitShader(object.vshader.c_str(), object.fshader.c_str());

	// 步骤5：配置顶点属性指针
	// 配置位置属性：从顶点着色器中获取vPosition变量的位置
	object.pLocation = glGetAttribLocation(object.program, "vPosition");
	glEnableVertexAttribArray(object.pLocation);  // 启用位置属性数组
	// 指定位置属性的数据格式：3个float，从缓存开始位置读取
	glVertexAttribPointer(object.pLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	// 配置颜色属性：从顶点着色器中获取vColor变量的位置
	object.cLocation = glGetAttribLocation(object.program, "vColor");
	glEnableVertexAttribArray(object.cLocation);  // 启用颜色属性数组
	// 指定颜色属性的数据格式：3个float，从位置数据之后开始读取
	glVertexAttribPointer(object.cLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(mesh->getPoints().size() * sizeof(glm::vec3)));

	// 步骤6：获取着色器中uniform变量的位置
	// 这些uniform变量用于传递变换矩阵到着色器
	object.modelLocation = glGetUniformLocation(object.program, "model");          // 模型变换矩阵
	object.viewLocation = glGetUniformLocation(object.program, "view");            // 观察变换矩阵
	object.projectionLocation = glGetUniformLocation(object.program, "projection"); // 投影变换矩阵
}


/**
 * framebuffer_size_callback函数：窗口大小改变回调函数
 * 
 * 当GLFW窗口大小发生改变时，此函数会被自动调用
 * 主要功能是更新全局的窗口尺寸变量
 * 
 * @param window GLFW窗口指针
 * @param width 新的窗口宽度
 * @param height 新的窗口高度
 */
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{	
	// 更新全局窗口尺寸变量
	HEIGHT = height;
	WIDTH = width;
    // 注意：这里不设置glViewport，因为每个display函数会根据需要设置自己的视口
}

/**
 * init函数：初始化OpenGL环境和资源
 * 
 * 主要功能：
 * 1. 设置着色器文件路径
 * 2. 加载立方体几何数据
 * 3. 绑定几何数据到OpenGL对象
 * 4. 设置渲染状态
 */
void init()
{
	// 设置着色器文件路径
	std::string vshader, fshader;
	vshader = "shaders/vshader.glsl";  // 顶点着色器：处理顶点变换
	fshader = "shaders/fshader.glsl";  // 片段着色器：处理像素着色

	// 加载立方体几何数据
	// 从OFF文件格式读取立方体的顶点和面信息
	cube->readOff("./assets/cube.off");
	
	// 将立方体数据绑定到OpenGL对象
	// 这会创建VAO、VBO，编译着色器，配置顶点属性
	bindObjectAndData(cube, cube_object, vshader, fshader);

	// 设置渲染状态
	glClearColor(0.0, 0.0, 0.0, 1.0);  // 设置清除颜色为黑色
}


/**
 * display_1函数：左上角视口渲染函数
 * 
 * 功能说明：
 * - 视口位置：左上角 (0, HEIGHT/2, WIDTH/2, HEIGHT/2)
 * - 投影方式：简单的默认投影（单位矩阵）
 * - 变换方式：手动应用旋转和缩放变换到模型矩阵
 * - 观察方式：简单的观察矩阵，只是翻转Z轴方向
 * 
 * 这个视口展示了最基本的3D渲染方式，不使用相机的lookAt函数
 */
void display_1()
{
	// 设置视口：左上角四分之一窗口
	glViewport(0, HEIGHT / 2, WIDTH / 2, HEIGHT / 2);

	// 更新相机参数（主要是根据用户输入更新角度、距离等）
	camera_1->updateCamera();

	// 构建模型变换矩阵
	// 模型变换：将对象从模型坐标系变换到世界坐标系
	glm::mat4 model = glm::mat4(1.0f);  // 初始化为单位矩阵
	// 应用仰角旋转（绕X轴）
	model = glm::rotate(model, glm::radians(camera_1->upAngle), glm::vec3(1.0, 0.0, 0.0));
	// 应用方位角旋转（绕Y轴，注意负号）
	model = glm::rotate(model, glm::radians(-camera_1->rotateAngle), glm::vec3(0.0, 1.0, 0.0));
	// 应用缩放变换
	model = glm::scale(model, glm::vec3(1.0 / camera_1->scale, 1.0 / camera_1->scale, 1.0 / camera_1->scale));

	// 构建观察变换矩阵
	// 这里使用简单的观察矩阵，只是翻转Z轴方向
	glm::mat4 view = glm::mat4(1.0f);
	view[2][2] = -1.0;  // 翻转Z轴，使相机朝向-Z方向

	// 构建投影变换矩阵
	// 这里使用单位矩阵，相当于正交投影到单位立方体
	glm::mat4 projection = glm::mat4(1.0f);

	// 将变换矩阵传递给着色器
	glUniformMatrix4fv(cube_object.modelLocation, 1, GL_FALSE, &model[0][0]);
	glUniformMatrix4fv(cube_object.viewLocation, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(cube_object.projectionLocation, 1, GL_FALSE, &projection[0][0]);

	// 使用着色器程序并绘制立方体
	glUseProgram(cube_object.program);
	glDrawArrays(GL_TRIANGLES, 0, cube->getPoints().size());
}

/**
 * display_2函数：左下角视口渲染函数
 * 
 * 功能说明：
 * - 视口位置：左下角 (0, 0, WIDTH/2, HEIGHT/2)
 * - 投影方式：透视投影（使用frustum函数）
 * - 变换方式：使用相机的lookAt函数计算观察变换
 * - 特点：展示了完整的相机观察变换和透视投影的组合使用
 * 
 * 这个视口展示了标准的3D相机系统，包含完整的观察变换和透视投影
 */
void display_2()
{
	// 设置视口：左下角四分之一窗口
	glViewport(0, 0, WIDTH / 2, HEIGHT / 2);

	// 更新相机参数（计算相机在球坐标系中的位置）
	camera_2->updateCamera();

	// 构建模型变换矩阵（这里使用单位矩阵，不对模型进行额外变换）
	glm::mat4 modelMatrix= glm::mat4(1.0);
	
	// 计算观察变换矩阵
	// 使用lookAt函数：根据相机位置(eye)、观察目标(at)、上方向(up)计算观察矩阵
	camera_2->viewMatrix = camera_2->lookAt(camera_2->eye, camera_2->at, camera_2->up);	
	
	// 计算透视投影矩阵
	// 使用frustum函数：根据视锥体参数计算透视投影矩阵
	float top = tan(camera_2->fov * M_PI / 180 / 2) * camera_2->zNear;    // 近平面上边界
	float right = top * camera_2->aspect;                                  // 近平面右边界
	camera_2->projMatrix = camera_2->frustum(-right, right, -top, top, camera_2->zNear, camera_2->zFar);
	
	// 将变换矩阵传递给着色器
	glUniformMatrix4fv(cube_object.modelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
	glUniformMatrix4fv(cube_object.viewLocation, 1, GL_FALSE, &camera_2->viewMatrix[0][0]);
	glUniformMatrix4fv(cube_object.projectionLocation, 1, GL_FALSE, &camera_2->projMatrix[0][0]);

	// 使用着色器程序并绘制立方体
	glUseProgram(cube_object.program);
	glDrawArrays(GL_TRIANGLES, 0, cube->getPoints().size());
}

/**
 * display_3函数：右上角视口渲染函数
 * 
 * 功能说明：
 * - 视口位置：右上角 (WIDTH/2, HEIGHT/2, WIDTH/2, HEIGHT/2)
 * - 投影方式：正交投影（使用ortho函数）
 * - 变换方式：使用相机的lookAt函数计算观察变换
 * - 特点：展示正交投影的效果，物体不会因距离而产生透视缩放
 * 
 * 正交投影特点：
 * - 平行线保持平行
 * - 物体大小不随距离变化
 * - 常用于工程制图、建筑设计等需要精确尺寸的场合
 */
void display_3()
{
	// 设置视口：右上角四分之一窗口
	glViewport(WIDTH / 2, HEIGHT / 2, WIDTH / 2, HEIGHT / 2);

	// 更新相机参数
	camera_3->updateCamera();

	// 构建模型变换矩阵（单位矩阵）
	glm::mat4 modelMatrix = glm::mat4(1.0);
	
	// 计算观察变换矩阵
	// 使用lookAt函数建立相机坐标系
	camera_3->viewMatrix = camera_3->lookAt(camera_3->eye, camera_3->at, camera_3->up);
	
	// 计算正交投影矩阵
	// ortho函数参数：left, right, bottom, top, near, far
	// 定义一个平行六面体作为视景体，超出此范围的物体将被裁剪
	camera_3->projMatrix = camera_3->ortho(-camera_3->scale, camera_3->scale, -camera_3->scale, camera_3->scale, camera_3->zNear, camera_3->zFar);

	// 将变换矩阵传递给着色器
	glUniformMatrix4fv(cube_object.modelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
	glUniformMatrix4fv(cube_object.viewLocation, 1, GL_FALSE, &camera_3->viewMatrix[0][0]);
	glUniformMatrix4fv(cube_object.projectionLocation, 1, GL_FALSE, &camera_3->projMatrix[0][0]);

	// 使用着色器程序并绘制立方体
	glUseProgram(cube_object.program);
	glDrawArrays(GL_TRIANGLES, 0, cube->getPoints().size());
}

/**
 * display_4函数：右下角视口渲染函数
 * 
 * 功能说明：
 * - 视口位置：右下角 (WIDTH/2, 0, WIDTH/2, HEIGHT/2)
 * - 投影方式：透视投影（使用perspective函数）
 * - 变换方式：使用相机的lookAt函数计算观察变换
 * - 特点：展示透视投影的效果，符合人眼视觉感知（近大远小）
 * 
 * 透视投影特点：
 * - 近大远小的视觉效果
 * - 平行线会在远处汇聚到消失点
 * - 更符合人类的视觉感知
 * - 常用于游戏、电影等需要真实感的场合
 */
void display_4()
{
	// 设置视口：右下角四分之一窗口
	glViewport(WIDTH / 2, 0, WIDTH / 2, HEIGHT / 2);

	// 更新相机参数
	camera_4->updateCamera();

	// 构建模型变换矩阵（单位矩阵）
	glm::mat4 modelMatrix = glm::mat4(1.0);
	
	// 计算观察变换矩阵
	// 使用lookAt函数建立相机坐标系
	camera_4->viewMatrix = camera_4->lookAt(camera_4->eye, camera_4->at, camera_4->up);

	// 计算透视投影矩阵
	// perspective函数参数：fov(视角), aspect(宽高比), near(近平面), far(远平面)
	// 这种方式比frustum更直观，直接指定视角和宽高比
	camera_4->projMatrix = camera_4->perspective(camera_4->fov, camera_4->aspect, camera_4->zNear, camera_4->zFar);

	// 将变换矩阵传递给着色器
	glUniformMatrix4fv(cube_object.modelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
	glUniformMatrix4fv(cube_object.viewLocation, 1, GL_FALSE, &camera_4->viewMatrix[0][0]);
	glUniformMatrix4fv(cube_object.projectionLocation, 1, GL_FALSE, &camera_4->projMatrix[0][0]);

	// 使用着色器程序并绘制立方体
	glUseProgram(cube_object.program);
	glDrawArrays(GL_TRIANGLES, 0, cube->getPoints().size());
}


/**
 * display函数：主渲染函数
 * 
 * 功能说明：
 * - 清除颜色缓冲区和深度缓冲区
 * - 依次调用四个子视口的渲染函数
 * - 每个子视口展示不同的投影方式和相机控制方法
 * 
 * 四个视口的布局：
 * ┌─────────┬─────────┐
 * │display_1│display_3│  <- 上半部分
 * │ (左上)  │ (右上)  │
 * ├─────────┼─────────┤
 * │display_2│display_4│  <- 下半部分  
 * │ (左下)  │ (右下)  │
 * └─────────┴─────────┘
 */
void display()
{
	// 清除缓冲区，为新一帧的渲染做准备
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 调用4个display分别在4个子视图内绘制立方体
	// 每个display使用的相机的投影类型与计算方法都不一样
	display_1();  // 左上角：基本变换，单位投影矩阵
	display_2();  // 左下角：lookAt + frustum透视投影
	display_3();  // 右上角：lookAt + 正交投影
	display_4();  // 右下角：lookAt + perspective透视投影
}

/**
 * reshape函数：窗口大小改变处理函数
 * 
 * 当窗口大小发生改变时调用此函数
 * 更新全局窗口尺寸变量并重新设置视口
 * 
 * @param w 新的窗口宽度
 * @param h 新的窗口高度
 */
void reshape(GLsizei w, GLsizei h)
{
	// 更新全局窗口尺寸变量
	WIDTH = w;
	HEIGHT = h;
	// 重新设置整个窗口的视口（这里设置的是整个窗口，各个display函数会设置自己的子视口）
	glViewport(0, 0, WIDTH, HEIGHT);
}

/**
 * printHelp函数：打印键盘操作帮助信息
 * 
 * 向控制台输出所有可用的键盘控制命令及其功能说明
 * 帮助用户了解如何与程序交互，控制相机参数
 */
void printHelp()
{
	std::cout << "Keyboard Usage" << std::endl;
	std::cout <<
		"Q/ESC: 	Exit" << std::endl <<                                    // 退出程序
		"h: 	Print help message" << std::endl <<                        // 打印帮助信息
		"SPACE: 	Reset all parameters" << std::endl <<                  // 重置所有相机参数
		"x/(shift+x): 	Increase/Decrease the rotate angle" << std::endl <<    // 增加/减少方位角（绕Y轴旋转）
		"y/(shift+y): 	Increase/Decrease the up angle" << std::endl <<        // 增加/减少仰角（上下旋转）
		"r/(shift+r): 	Increase/Decrease the distance between camera and object" << std::endl <<  // 增加/减少相机距离
		"f/(shift+f): 	Increase/Decrease FOV of perspective projection" << std::endl <<           // 增加/减少透视投影视角
		"a/(shift+a): 	Increase/Decrease WIDTH/HEIGHT aspect of perspective projection" << std::endl <<  // 增加/减少透视投影宽高比
		"s/(shift+s):	Increase/Decrease the extent of orthogonal projection" << std::endl;        // 增加/减少正交投影缩放
}

/**
 * mainWindow_key_callback函数：键盘输入回调函数
 * 
 * 当用户按下键盘按键时，GLFW会自动调用此函数
 * 处理程序级别的按键（如退出、帮助），其他按键转发给相机处理
 * 
 * @param window GLFW窗口指针
 * @param key 按下的键码
 * @param scancode 系统相关的扫描码
 * @param action 按键动作（按下、释放、重复）
 * @param mode 修饰键状态（Shift、Ctrl、Alt等）
 */
void mainWindow_key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	// 处理ESC键：退出程序
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	// 处理H键：打印帮助信息
	else if(key == GLFW_KEY_H && action == GLFW_PRESS) 
	{
		printHelp();
	}
	// 其他按键：转发给所有相机对象处理
	// 这样所有四个视口的相机都会同步响应用户的输入
	else
	{
		camera_1->keyboard(key,action,mode);
		camera_2->keyboard(key,action,mode);
		camera_3->keyboard(key,action,mode);
		camera_4->keyboard(key,action,mode);
	}
}


/**
 * cleanData函数：清理和释放资源
 * 
 * 程序结束前调用此函数，负责释放所有动态分配的内存和OpenGL资源
 * 包括：几何对象、相机对象、OpenGL缓冲区和着色器程序
 */
void cleanData() {
	// 清理立方体几何数据
	cube->cleanData();

	// 释放相机对象内存
	delete camera_1;
	delete camera_2;
	delete camera_3;
	delete camera_4;

	// 将指针设置为NULL，避免悬空指针
	camera_1 = NULL;
	camera_2 = NULL;
	camera_3 = NULL;
	camera_4 = NULL;

	// 释放几何对象内存
	delete cube;
	cube = NULL;

	// 删除OpenGL资源
	glDeleteVertexArrays(1, &cube_object.vao);    // 删除顶点数组对象
	glDeleteBuffers(1, &cube_object.vbo);         // 删除顶点缓冲对象
	glDeleteProgram(cube_object.program);         // 删除着色器程序
}

/**
 * main函数：程序入口点
 * 
 * 主要功能：
 * 1. 初始化GLFW和OpenGL环境
 * 2. 创建窗口并设置回调函数
 * 3. 初始化渲染资源
 * 4. 进入主渲染循环
 * 5. 程序结束时清理资源
 * 
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 程序退出状态码
 */
int main(int argc, char **argv)
{	
	// 步骤1：初始化GLFW库
	glfwInit();
		
	// 步骤2：配置GLFW窗口属性
	// 开启多重采样抗锯齿，提高渲染质量
	glfwWindowHint(GLFW_SAMPLES,5);
	
	// 设置OpenGL版本为3.3 Core Profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	// macOS兼容性设置
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 步骤3：创建窗口
	GLFWwindow* mainwindow = glfwCreateWindow(WIDTH, HEIGHT, "李文俊-2023150001-实验3.1", NULL, NULL);
	if (mainwindow == NULL)
	{
		std::cout << "Failed to create GLFW window!" << std::endl;
		glfwTerminate();
		return -1;
	}
	
	// 步骤4：设置OpenGL上下文和回调函数
	glfwMakeContextCurrent(mainwindow);                                    // 设置当前OpenGL上下文
	glfwSetFramebufferSizeCallback(mainwindow, framebuffer_size_callback); // 窗口大小改变回调
	glfwSetKeyCallback(mainwindow,mainWindow_key_callback);                // 键盘输入回调
	
	// 步骤5：初始化GLAD（OpenGL函数加载器）
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
	// 步骤6：配置OpenGL渲染状态
	glEnable(GL_DEPTH_TEST);  // 启用深度测试，确保3D物体正确遮挡
	
	// 步骤7：初始化渲染资源
	init();  // 加载几何数据、编译着色器、配置OpenGL对象
    
    // 步骤8：初始显示
    // 确保程序启动时就显示正确的四象限布局，解决初始显示问题
    display();
    glfwSwapBuffers(mainwindow);
    
    // 步骤9：打印操作帮助信息
    printHelp();
    
	// 步骤10：主渲染循环
	// 持续渲染直到用户关闭窗口
	while (!glfwWindowShouldClose(mainwindow))
    {
        display();                    // 渲染一帧
        glfwSwapBuffers(mainwindow);  // 交换前后缓冲区，显示渲染结果
        glfwPollEvents();             // 处理窗口事件（键盘、鼠标等）
    }
    
    // 步骤11：程序结束，清理资源
    glfwTerminate();  // 终止GLFW
    return 0;         // 正常退出
}

