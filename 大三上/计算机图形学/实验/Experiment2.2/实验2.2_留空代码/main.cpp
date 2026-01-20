// 实验2.2：OFF格式的模型显示
// 目标：
// - 读取并解析OFF模型文件（顶点、面片索引）
// - 将面片展开为顶点数组传入着色器进行绘制
// - 了解并实践深度测试（GL_DEPTH_TEST）、面剔除（GL_CULL_FACE）
// - 切换绘制模式（填充/线框）观察渲染差异
// OFF文件说明：以"OFF"开头；第二行依次为顶点数、面片数、边数；
// 顶点列表每行一个 x y z；面片列表每行一个，首数为该面顶点数，随后为顶点索引。
#include "Angel.h"

#include <vector>
#include <fstream>
#include <string>

using namespace std;

int window;
// 三角面片中的顶点序列（OFF面片索引）
// OFF面片行示例："3 1 6 2" 表示该面有3个顶点，索引为1、6、2。
typedef struct vIndex {
	unsigned int a, b, c;
	vIndex(int ia, int ib, int ic) : a(ia), b(ib), c(ic) {}
} vec3i;

// OFF顶点坐标列表（与文件中顶点顺序一致）
std::vector<glm::vec3> vertices;
// OFF面片索引列表（每个元素对应一个三角面，记录三个顶点的索引）
std::vector<vec3i> faces;

int nVertices = 0;
int nFaces = 0;
int nEdges = 0;

// 展开后的绘制顶点数组：根据 faces 将所有三角面片的顶点依次排入
// 注意：如果一个四边形由两个三角形构成，则会有6个绘制点（顶点重复展开）
std::vector<glm::vec3> points;   // 传入着色器的绘制点
// 与 points 一一对应的颜色数组：可用预设颜色或依据顶点坐标生成
std::vector<glm::vec3> colors;   // 传入着色器的颜色

const int NUM_VERTICES = 8;

const glm::vec3 vertex_colors[NUM_VERTICES] = {
	glm::vec3(1.0, 1.0, 1.0),  // White
	glm::vec3(1.0, 1.0, 0.0),  // Yellow
	glm::vec3(0.0, 1.0, 0.0),  // Green
	glm::vec3(0.0, 1.0, 1.0),  // Cyan
	glm::vec3(1.0, 0.0, 1.0),  // Magenta
	glm::vec3(1.0, 0.0, 0.0),  // Red
	glm::vec3(0.0, 0.0, 0.0),  // Black
	glm::vec3(0.0, 0.0, 1.0)   // Blue
};
// 说明：vertex_colors 为示例颜色，用于为展开后的顶点赋色。
// 在实际项目中也可按法线、位置或其他属性生成颜色。

void read_off(const std::string filename)
{
	// 读取并解析OFF文件：
	// - 第一行：应为字符串 "OFF"（此处仅读取不校验）
	// - 第二行：顶点数 nVertices、面片数 nFaces、边数 nEdges（边数可能省略）
	// - 接着 nVertices 行顶点坐标（x y z）
	// - 然后 nFaces 行面片：首个数为该面顶点数量，其后为顶点索引（本实验按三角面处理）
	// 注意：读取完成后填充全局容器 vertices 和 faces，供后续展开与绘制使用。
	// fin打开文件读取文件信息
	if (filename.empty()) {
		return;
	}
	std::ifstream fin;
	fin.open(filename);
	// @TODO: Task1:修改此函数读取OFF文件中三维模型的信息
	if (!fin)
	{
		printf("文件有误\n");
		return;
	}
	else
	{
		printf("文件打开成功\n");
		vertices.clear();
		faces.clear();

		// 读取OFF字符串（一般为 "OFF"）
		string str;
		fin >> str;
		// 读取文件中顶点数、面片数、边数（边数可能未使用）
		fin >> nVertices >> nFaces >> nEdges;

		// 读取顶点坐标：每行一个顶点，依次为 x y z；索引与文件顺序一致
		for (int i = 0; i < nVertices; i++) {
			float x, y, z;
			fin >> x >> y >> z;
			vertices.push_back(glm::vec3(x, y, z));
		}
		// 读取面片索引：<顶点数量> <索引1> <索引2> <索引3> ...
		// 本实验以三角面为例，读取前三个索引并保存到 faces。
		for (int i = 0; i < nFaces; i++) {
			int numVertices, a, b, c;
			fin >> numVertices >> a >> b >> c;
			faces.push_back(vec3i(a, b, c));
		}
	}
	fin.close();
}

void storeFacesPoints()
{
	// 根据 faces 展开所有三角面片的顶点到 points，并为每个顶点赋颜色到 colors。
	// 展开后的点序列将直接用于 glDrawArrays(GL_TRIANGLES, ...) 进行绘制。
	// 提示：顶点可能被多个面片复用，展开是为了按绘制顺序组织数据。
	points.clear();
	colors.clear();
	// @TODO: Task1:修改此函数在points和colors容器中存储每个三角面片的各个点和颜色信息
	// 在points容器中，依次添加每个面片的顶点，并在colors容器中，添加该点的颜色信息
	// 比如一个正方形由两个三角形构成，那么vertices会由4个顶点的数据构成，faces会记录两个三角形的顶点下标，
	// 而points就是记录这2个三角形的顶点，总共6个顶点的数据。
	// colors容器则是和points的顶点一一对应，保存这个顶点的颜色，这里我们可以使用顶点坐标或者自己设定的颜色赋值。
	for (int i = 0; i < faces.size(); i++) {
		// 获取三角面片的三个顶点索引
		vec3i face = faces[i];
		
		// 添加三个顶点到points容器
		points.push_back(vertices[face.a]);
		points.push_back(vertices[face.b]);
		points.push_back(vertices[face.c]);
		
		// 为每个顶点添加颜色：此处使用预定义 vertex_colors。
		// 使用顶点索引对颜色数组长度取模，避免索引越界（演示用途）。
		colors.push_back(vertex_colors[face.a % NUM_VERTICES]);
		colors.push_back(vertex_colors[face.b % NUM_VERTICES]);
		colors.push_back(vertex_colors[face.c % NUM_VERTICES]);
	}
}

void init()
{
	// 初始化：读取模型、展开顶点、创建VAO/VBO并上传数据、绑定着色器与属性。
	// 读取off模型文件
	read_off("./assets/cube.off");
	storeFacesPoints();

	// 创建顶点数组对象
	GLuint vao[1];
    glGenVertexArrays(1, vao);  	// 分配1个顶点数组对象
	glBindVertexArray(vao[0]);  	// 绑定顶点数组对象

	// 创建并初始化顶点缓存对象
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	// 为顶点与颜色开辟连续缓冲区：前半段存顶点，后半段存颜色
	glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3) + colors.size() * sizeof(glm::vec3), NULL, GL_DYNAMIC_DRAW);

	// @TODO: Task1:修改完成后再打开下面注释，否则程序会报错
	// 分别将 points 和 colors 填入缓冲的前后两段
	glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(glm::vec3), &points[0]);
	glBufferSubData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), colors.size() * sizeof(glm::vec3), &colors[0]);

	// 读取着色器并使用
	std::string vshader, fshader;
	vshader = "shaders/vshader.glsl";
	fshader = "shaders/fshader.glsl";
	GLuint program = InitShader(vshader.c_str(), fshader.c_str());
	glUseProgram(program);

	// 从顶点着色器中初始化顶点的位置
	GLuint pLocation = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(pLocation);
	glVertexAttribPointer(pLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	// 从顶点着色器中初始化顶点的颜色
	GLuint cLocation = glGetAttribLocation(program, "vColor");
	glEnableVertexAttribArray(cLocation);
	// 颜色属性起始位置为缓冲区中顶点数据之后
	glVertexAttribPointer(cLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(points.size() * sizeof(glm::vec3)));

	// 黑色背景
	glClearColor(0.0, 0.0, 0.0, 1.0);
}

void display(void)
{
	// 每帧渲染前清理缓冲：
	// - 开启深度测试时必须清除深度缓冲，否则会受到上一帧影响
	//  @TODO: Task2:清理窗口，包括颜色缓存和深度缓存
	// glClear(GL_COLOR_BUFFER_BIT);
	// 清理窗口，包括颜色缓存和深度缓存
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// 使用三角形按顺序绘制展开后的点序列
	glDrawArrays(GL_TRIANGLES, 0, points.size());
}


// 窗口键盘回调函数。
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	// 键盘控制说明：
	// - '-' 读取 cube.off；'=' 读取 cube2.off，并重新上传点与颜色
	// - '0' 重置：关闭深度测试与面剔除，恢复填充模式
	// - '1' 开启（不按Shift）/关闭（按Shift）深度测试：glEnable/Disable(GL_DEPTH_TEST)
	// - '2' 开启（不按Shift）/关闭（按Shift）反面剔除：glCullFace(GL_BACK)
	// - '3' 开启（不按Shift）/关闭（按Shift）正面剔除：glCullFace(GL_FRONT)
	// - '4' 开启（不按Shift）/关闭（按Shift）线框模式：glPolygonMode(GL_FRONT_AND_BACK, GL_LINE/FILL)
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	else if (key == GLFW_KEY_MINUS && action == GLFW_PRESS)
	{
		cout << "read cube.off" << endl;
		read_off("./assets/cube.off");
		storeFacesPoints();
		// @TODO: Task1:修改完成后再打开下面注释，否则程序会报错
		glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(glm::vec3), &points[0]);
		glBufferSubData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), colors.size() * sizeof(glm::vec3), &colors[0]);
	}
	else if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS)
	{
		cout << "read cube2.off" << endl;
		read_off("./assets/cube2.off");
		storeFacesPoints();
		// @TODO: Task1:修改完成后再打开下面注释，否则程序会报错
		glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(glm::vec3), &points[0]);
		glBufferSubData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), colors.size() * sizeof(glm::vec3), &colors[0]);
	}
	else if (key == GLFW_KEY_0 && action == GLFW_PRESS)
	{
		cout << "reset" << endl;
		// 关闭深度测试
		glDisable(GL_DEPTH_TEST);
		// 关闭面剔除
		glDisable(GL_CULL_FACE);
		// 使用填充绘制模式
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	// @TODO: Task2:启用深度测试
	else if (key == GLFW_KEY_1 && action == GLFW_PRESS && mode == 0x0000) // 0x0000表示组合键为空 
	{
		cout << "depth test: enable" << endl;
		glEnable(GL_DEPTH_TEST);
	}
	// @TODO: Task2:关闭深度测试
	else if (key == GLFW_KEY_1 && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		cout << "depth test: disable" << endl;
		glDisable(GL_DEPTH_TEST);
	}
	// @TODO: Task3:启用反面剔除
	else if (key == GLFW_KEY_2 && action == GLFW_PRESS && mode == 0x0000)
	{
		cout << "cull back: enable" << endl;
		// 增加下面代码
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}
	// @TODO: Task3:关闭反面剔除
	else if (key == GLFW_KEY_2 && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		cout << "cull back: disable" << endl;
		// 增加下面代码
		glDisable(GL_CULL_FACE);
	}
	// @TODO: Task4:启用正面剔除
	else if (key == GLFW_KEY_3 && action == GLFW_PRESS && mode == 0x0000)
	{
		cout << "cull front: enable" << endl;
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	}
	// @TODO: Task4:关闭正面剔除
	else if (key == GLFW_KEY_3 && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		cout << "cull front: disable" << endl;
		glDisable(GL_CULL_FACE);
	}
	// @TODO: Task5:启用线绘制模式
	else if (key == GLFW_KEY_4 && action == GLFW_PRESS && mode == 0x0000)
	{
		cout << "line mode: enable" << endl;
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	// @TODO: Task5:关闭线绘制模式
	else if (key == GLFW_KEY_4 && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		cout << "line mode: disable" << endl;
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

int main(int argc, char** argv)
{
	// 程序入口：初始化窗口与OpenGL上下文，配置GL版本与Profile，加载函数地址。
	// 初始化GLFW库，必须是应用程序调用的第一个GLFW函数
	glfwInit();

	// 配置GLFW：OpenGL 3.3 Core Profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif

	// 创建窗口并设置回调：键盘与窗口尺寸
	// 配置窗口属性
	GLFWwindow* window = glfwCreateWindow(600, 600, "李文俊-2023150001-实验2.2", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// 调用任何OpenGL的函数之前初始化GLAD（函数地址加载器）
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	init();
	while (!glfwWindowShouldClose(window))
	{
		display();

		// 交换颜色缓冲 以及 检查有没有触发什么事件（比如键盘输入、鼠标移动等）
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	return 0;
}