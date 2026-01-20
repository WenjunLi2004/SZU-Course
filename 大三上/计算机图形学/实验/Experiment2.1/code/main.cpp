#include "Angel.h"
#include <string>

const glm::vec3 WHITE(1.0, 1.0, 1.0);
const glm::vec3 BLACK(0.0, 0.0, 0.0);
const glm::vec3 RED(1.0, 0.0, 0.0);
const glm::vec3 GREEN(0.0, 1.0, 0.0);
const glm::vec3 BLUE(0.0, 0.0, 1.0);
const glm::vec3 YELLOW(1.0, 1.0, 0.0);
const glm::vec3 ORANGE(1.0, 0.65, 0.0);
const glm::vec3 PURPLE(0.8, 0.0, 0.8);

// 窗口变量
const int SQUARE_NUM = 6;
const int SQUARE_NUM_POINTS = 4 * SQUARE_NUM;
int Window;
int width = 600;		// 主窗口宽度
int height = 600;		// 主窗口高度
double offsetAngle = 0; // 角度偏移量
double delta = 0.01;	// 每次改变角度偏移的变化量
glm::vec3 WindowSquareColor = WHITE;

GLuint square_vao, program;

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// 获得正方形每个顶点的角度
double getSquareAngle(int point)
{
	return (M_PI / 4 + (M_PI / 2 * point)) + offsetAngle;
}

// 生成正方形上每个顶点的属性（位置和颜色）
void generateSquarePoints(glm::vec2 vertices[], glm::vec3 colors[], int squareNum, int startVertexIndex)
{
	double scale = 0.90;
	double scaleAdjust = scale / squareNum;
	glm::vec2 center(0.0, 0.0);
	int vertexIndex = startVertexIndex;
	for (int i = 0; i < squareNum; i++)
	{
		glm::vec3 currentColor = 0 == i % 2 ? WindowSquareColor : BLACK;
		for (int j = 0; j < 4; j++)
		{
			double currentAngle = getSquareAngle(j);
			vertices[vertexIndex] = glm::vec2(cos(currentAngle), sin(currentAngle)) * glm::vec2(scale, scale) + center;
			colors[vertexIndex] = currentColor;
			vertexIndex++;
		}
		scale -= scaleAdjust;
	}
}

void Init()
{
	// 定义顶点位置数组和颜色数组
	glm::vec2 vertices[SQUARE_NUM * 4]; // 存储所有正方形顶点的位置坐标
	glm::vec3 colors[SQUARE_NUM * 4];	// 存储所有正方形顶点的颜色信息

	// 创建多个正方形的顶点数据
	generateSquarePoints(vertices, colors, SQUARE_NUM, 0);

	// 分配并绑定顶点数组对象(VAO)
	glGenVertexArrays(1, &square_vao);
	glBindVertexArray(square_vao);

	/* 
	 * 使用单个VBO存储多种顶点属性数据的方法：
	 * 与实验1.2中为每种属性创建单独VBO不同，这里使用一个VBO存储所有数据
	 */

	// 1. 创建并绑定顶点缓存对象(VBO)
	GLuint vbo;
	glGenBuffers(1, &vbo);				// 生成一个VBO
	glBindBuffer(GL_ARRAY_BUFFER, vbo); // 绑定VBO为当前数组缓冲区

	// 2. 使用glBufferData()分配缓存空间
	// 参数说明：
	// - GL_ARRAY_BUFFER: 缓冲区类型，表示顶点属性数据
	// - sizeof(vertices) + sizeof(colors): 总空间大小 = 顶点数据大小 + 颜色数据大小
	// - NULL: 暂不传入数据，只分配空间
	// - GL_STATIC_DRAW: 数据使用模式，表示数据不会经常改变
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(colors), NULL, GL_STATIC_DRAW);

	/* 
	 * 3. 使用glBufferSubData()函数分别传输不同类型的顶点数据
	 * glBufferSubData(target, offset, size, data)参数说明：
	 * - target: 缓冲区对象类型，这里是GL_ARRAY_BUFFER
	 * - offset: 数据在缓冲区中的起始偏移量（字节为单位）
	 * - size: 要传输的数据大小（字节为单位）
	 * - data: 指向源数据的指针
	 */

	// 传输顶点位置数据到VBO的开始位置
	// offset = 0: 从缓冲区开始位置存储顶点坐标数据
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

	// 传输颜色数据到VBO中顶点数据之后的位置
	// offset = sizeof(vertices): 从顶点数据结束的位置开始存储颜色数据
	// 这样在VBO中的数据布局为：[顶点数据][颜色数据]
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeof(colors), colors);

	// 读取并编译着色器程序
	std::string vshader, fshader;
	vshader = "shaders/vshader.glsl";
	fshader = "shaders/fshader.glsl";

	// 创建着色器程序对象并使用
	program = InitShader(vshader.c_str(), fshader.c_str());
	glUseProgram(program);

	/* 
	 * 4. 配置顶点属性指针，告诉OpenGL如何解释VBO中的数据
	 */

	// 配置顶点位置属性
	GLuint pLocation = glGetAttribLocation(program, "vPosition"); // 获取顶点着色器中vPosition的位置
	glEnableVertexAttribArray(pLocation);						  // 启用顶点属性数组
	// 设置顶点位置属性指针：
	// - pLocation: 属性位置
	// - 2: 每个顶点有2个分量(x, y)
	// - GL_FLOAT: 数据类型为浮点数
	// - GL_FALSE: 不需要标准化
	// - 0: 步长为0（紧密排列）
	// - BUFFER_OFFSET(0): 数据在VBO中的偏移量为0（从开始位置读取）
	glVertexAttribPointer(pLocation, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	// 配置顶点颜色属性
	GLuint cLocation = glGetAttribLocation(program, "vColor"); // 获取顶点着色器中vColor的位置
	glEnableVertexAttribArray(cLocation);					   // 启用颜色属性数组
	// 设置顶点颜色属性指针：
	// - cLocation: 属性位置
	// - 3: 每个颜色有3个分量(r, g, b)
	// - GL_FLOAT: 数据类型为浮点数
	// - GL_FALSE: 不需要标准化
	// - 0: 步长为0（紧密排列）
	// - BUFFER_OFFSET(sizeof(vertices)): 颜色数据在VBO中的偏移量为顶点数据的大小
	glVertexAttribPointer(cLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(vertices)));

	// 设置清屏颜色为黑色
	glClearColor(0.0, 0.0, 0.0, 1.0);
}

void Display()
{
	Init(); // 重绘时写入新的颜色数据
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(program);

	// 创建顶点数组对象
	glBindVertexArray(square_vao); // 绑定顶点数组对象

	for (int i = 0; i < SQUARE_NUM; i++)
	{
		glDrawArrays(GL_TRIANGLE_FAN, (i * 4), 4);
	}
}

void printHelp()
{
	printf("%s\n\n", "Interaction");
	printf("Keys to update the background color:\n");
	printf("'r' - red\n'b' - blue\n'w' - white\n");
	printf("Mouse click to update color:\n");
	printf("'left' - green\n'right' - yellow\n");
	printf("Mouse sroll to rotate:\n");
	printf("'up' - clockwise\n");
	printf("'down' - anticlockwise\n");
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mode)
{
	// @TODO: 课堂练习#1 - Step1  在窗口中添加鼠标点击，设置形状颜色
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		WindowSquareColor = GREEN;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		WindowSquareColor = YELLOW;
	}
}
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	// @TODO: 课堂练习#1 - Step2  在窗口中添加键盘事件，更换形状颜色
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	else if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		WindowSquareColor = RED;
	}
	else if (key == GLFW_KEY_B && action == GLFW_PRESS)
	{
		WindowSquareColor = BLUE;
	}
	else if (key == GLFW_KEY_W && action == GLFW_PRESS)
	{
		WindowSquareColor = WHITE;
	}
}
void sroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
	// @TODO: 课堂练习#2 控制正方形的旋转动画
	// 根据滚轮方向控制旋转：向上滚动顺时针，向下滚动逆时针
	if (yoffset > 0)
	{
		// 向上滚动，顺时针旋转
		offsetAngle += delta;
	}
	else if (yoffset < 0)
	{
		// 向下滚动，逆时针旋转
		offsetAngle -= delta;
	}
}

int main(int argc, char **argv)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// 创建窗口。
	GLFWwindow *window = glfwCreateWindow(width, height, "Interaction", NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	// @TODO: 课堂练习#1 - Step1  将mouse_button_callback与window绑定
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// @TODO: 课堂练习#1 - Step2 将key_callback与window绑定
	glfwSetKeyCallback(window, key_callback);

	// @TODO: 课堂练习#2 将sroll_callback与window绑定
	glfwSetScrollCallback(window, sroll_callback);

	// 配置glad。
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	Init();
	printHelp();
	// 循环渲染。
	while (!glfwWindowShouldClose(window))
	{
		Display();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwTerminate();
	return 0;
}
