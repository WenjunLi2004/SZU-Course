#include "Angel.h"
#include "TriMesh.h"
#include "Camera.h"

#include <vector>
#include <string>

int WIDTH = 600;
int HEIGHT = 600;
int mainWindow;

struct openGLObject
{
	// 顶点数组对象
	GLuint vao;
	// 顶点缓存对象
	GLuint vbo;

	// 着色器程序
	GLuint program;
	// 着色器文件
	std::string vshader;
	std::string fshader;
	// 着色器变量
	GLuint pLocation;
	GLuint cLocation;

	// 投影变换变量
	GLuint modelLocation;
	GLuint viewLocation;
	GLuint projectionLocation;

	// 阴影变量
	GLint shadowLocation;
};

// 全局变量光源位置
glm::vec3 light_position = glm::vec3(0.0, 1.5, 1.0);

// 全局变量键盘控制光源移动的参数
float move_step_size = 0.2;

openGLObject tri_object;
openGLObject plane_object;

TriMesh* triangle = new TriMesh();
TriMesh *plane = new TriMesh();
Camera* camera = new Camera();

// ---------------- 新增：鼠标交互状态 ----------------
// 左键拖拽光源、右键拖拽相机
bool draggingLeft = false;
bool draggingRight = false;
double lastMouseX = 0.0, lastMouseY = 0.0;
// 阴影状态：点击后计算缓存的阴影模型矩阵
bool shadowReady = false;
bool shadowDirty = false;
glm::mat4 cachedShadowModel(1.0f);

// 鼠标位置回调：左键拖拽更新光源 X/Z；右键拖拽更新相机旋转
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	// 记录上一帧位置用于相机拖拽
	double dx = xpos - lastMouseX;
	double dy = ypos - lastMouseY;
	lastMouseX = xpos;
	lastMouseY = ypos;

	if (draggingLeft) {
		// 将屏幕坐标映射到平面范围 [-halfX, halfX], [-halfZ, halfZ]
		float halfX = plane->getScale().x * 0.5f;
		float halfZ = plane->getScale().z * 0.5f;
		float nx = static_cast<float>(xpos / WIDTH * 2.0 - 1.0);
		float nz = static_cast<float>((HEIGHT - ypos) / (double)HEIGHT * 2.0 - 1.0);
		light_position.x = nx * halfX;
		light_position.z = nz * halfZ;
		// 不立即计算阴影，等点击释放时触发
	}
	if (draggingRight) {
		// 鼠标拖拽调整相机角度（简单比例）
		camera->rotateAngle += static_cast<float>(dx) * 0.2f;
		camera->upAngle     -= static_cast<float>(dy) * 0.2f;
	}
}

// 鼠标按键回调：左键按下/释放开始/结束拖拽，并在释放后标记需要计算阴影
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			draggingLeft = true;
			shadowDirty = true; // 点击后立即计算一次阴影
		}
		else if (action == GLFW_RELEASE) {
			draggingLeft = false;
			shadowDirty = true; // 释放时再次计算阴影，捕捉最终位置
		}
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (action == GLFW_PRESS) {
			draggingRight = true;
		}
		else if (action == GLFW_RELEASE) {
			draggingRight = false;
		}
	}
}

void bindObjectAndData(TriMesh* mesh, openGLObject& object, const std::string &vshader, const std::string &fshader) {

	// 创建顶点数组对象
    glGenVertexArrays(1, &object.vao);  
	glBindVertexArray(object.vao);  

	// 创建并初始化顶点缓存对象
	glGenBuffers(1, &object.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, object.vbo);
	glBufferData(GL_ARRAY_BUFFER, 
		mesh->getPoints().size() * sizeof(glm::vec3) + mesh->getColors().size() * sizeof(glm::vec3),
		NULL, 
		GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, 0, mesh->getPoints().size() * sizeof(glm::vec3), &mesh->getPoints()[0]);
	glBufferSubData(GL_ARRAY_BUFFER, mesh->getPoints().size() * sizeof(glm::vec3), mesh->getColors().size() * sizeof(glm::vec3), &mesh->getColors()[0]);

	object.vshader = vshader;
	object.fshader = fshader;
	object.program = InitShader(object.vshader.c_str(), object.fshader.c_str());

	// 从顶点着色器中初始化顶点的位置
	object.pLocation = glGetAttribLocation(object.program, "vPosition");
	glEnableVertexAttribArray(object.pLocation);
	glVertexAttribPointer(object.pLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	// 从顶点着色器中初始化顶点的颜色
	object.cLocation = glGetAttribLocation(object.program, "vColor");
	glEnableVertexAttribArray(object.cLocation);
	glVertexAttribPointer(object.cLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(mesh->getPoints().size() * sizeof(glm::vec3)));

	// 获得矩阵位置
	object.modelLocation = glGetUniformLocation(object.program, "model");
	object.viewLocation = glGetUniformLocation(object.program, "view");
	object.projectionLocation = glGetUniformLocation(object.program, "projection");

	// 获得阴影标识的位置
	object.shadowLocation = glGetUniformLocation(object.program, "isShadow");
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
	WIDTH = width; HEIGHT = height;
	camera->aspect = static_cast<float>(width) / static_cast<float>(height);
}

void init()
{
	std::string vshader, fshader;
	// 读取着色器并使用
	vshader = "shaders/vshader.glsl";
	fshader = "shaders/fshader.glsl";

	// 创建三角形，颜色设置为红色
	triangle->generateTriangle( glm::vec3( 1.0, 0.0, 0.0 ) );
	
	// 设置三角形的位置和旋转
	triangle->setRotation(glm::vec3(90, 0, 0));
	triangle->setTranslation(glm::vec3(0, 0.3, 0));
	triangle->setScale(glm::vec3(0.5, 0.5, 0.5));

	bindObjectAndData(triangle, tri_object, vshader, fshader);

	// 创建正方形平面，设置为黄绿色
	plane->generateSquare(glm::vec3(0.6, 0.8, 0.0));

	// 设置正方形的位置和旋转，注意这里我们将正方形平面下移了一点点距离，
	// 这是为了防止和阴影三角形重叠在同个平面上导致颜色交叉
	plane->setRotation(glm::vec3(90, 0, 0));
	plane->setTranslation(glm::vec3(0, -0.001, 0));
	plane->setScale(glm::vec3(3, 3, 3));

	bindObjectAndData(plane, plane_object, vshader, fshader);

	glClearColor(1.0, 1.0, 1.0, 1.0);
}

void display()
{
	// @TODO: Task2：根据光源位置，计算阴影投影矩阵
	// 计算阴影投影矩阵 - 将物体投影到y=0平面上
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	camera->updateCamera();

	// 计算视图变换矩阵
	camera->viewMatrix = camera->getViewMatrix();
	// 计算相机投影矩阵
	camera->projMatrix = camera->getProjectionMatrix(true);

	// 若需要，计算并缓存阴影矩阵（只在点击后）
	if (shadowDirty) {
		// 以平面三个世界坐标点构造平面方程
		glm::mat4 planeModel = plane->getModelMatrix();
		glm::vec3 p0 = glm::vec3(planeModel * glm::vec4(plane->getPoints()[0], 1.0f));
		glm::vec3 p1 = glm::vec3(planeModel * glm::vec4(plane->getPoints()[1], 1.0f));
		glm::vec3 p2 = glm::vec3(planeModel * glm::vec4(plane->getPoints()[2], 1.0f));
		glm::vec3 planeNormal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
		glm::vec3 planePoint = p0;
		glm::vec4 planeEq = glm::vec4(planeNormal, -glm::dot(planeNormal, planePoint));

		glm::vec4 L(light_position, 1.0f);
		float dotPL = glm::dot(planeEq, L);

		glm::mat4 shadowProjectionMatrix(0.0f);
		// 列主序正确填充（GLM 为列主序）
		// 第一列
		shadowProjectionMatrix[0][0] = dotPL - L.x * planeEq.x;
		shadowProjectionMatrix[0][1] = -L.y * planeEq.x;
		shadowProjectionMatrix[0][2] = -L.z * planeEq.x;
		shadowProjectionMatrix[0][3] = -L.w * planeEq.x;
		// 第二列
		shadowProjectionMatrix[1][0] = -L.x * planeEq.y;
		shadowProjectionMatrix[1][1] = dotPL - L.y * planeEq.y;
		shadowProjectionMatrix[1][2] = -L.z * planeEq.y;
		shadowProjectionMatrix[1][3] = -L.w * planeEq.y;
		// 第三列
		shadowProjectionMatrix[2][0] = -L.x * planeEq.z;
		shadowProjectionMatrix[2][1] = -L.y * planeEq.z;
		shadowProjectionMatrix[2][2] = dotPL - L.z * planeEq.z;
		shadowProjectionMatrix[2][3] = -L.w * planeEq.z;
		// 第四列
		shadowProjectionMatrix[3][0] = -L.x * planeEq.w;
		shadowProjectionMatrix[3][1] = -L.y * planeEq.w;
		shadowProjectionMatrix[3][2] = -L.z * planeEq.w;
		shadowProjectionMatrix[3][3] = dotPL - L.w * planeEq.w;

		cachedShadowModel = shadowProjectionMatrix * triangle->getModelMatrix();
		shadowReady = true;
		shadowDirty = false;
	}

	// 先绘制平面
	glBindVertexArray(plane_object.vao);
	glUseProgram(plane_object.program);
	glm::mat4 modelMatrix = plane->getModelMatrix();
	glUniformMatrix4fv(plane_object.modelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
	glUniformMatrix4fv(plane_object.viewLocation, 1, GL_FALSE, &camera->viewMatrix[0][0]);
	glUniformMatrix4fv(plane_object.projectionLocation, 1, GL_FALSE, &camera->projMatrix[0][0]);
	glUniform1i(plane_object.shadowLocation, 1);
	glDrawArrays(GL_TRIANGLES, 0, plane->getPoints().size());

	// 若阴影就绪，绘制阴影
	if (shadowReady) {
		glBindVertexArray(tri_object.vao);
		glUseProgram(tri_object.program);
		glUniform1i(tri_object.shadowLocation, 0); // 黑色阴影
		glUniformMatrix4fv(tri_object.modelLocation, 1, GL_FALSE, &cachedShadowModel[0][0]);
		glUniformMatrix4fv(tri_object.viewLocation, 1, GL_FALSE, &camera->viewMatrix[0][0]);
		glUniformMatrix4fv(tri_object.projectionLocation, 1, GL_FALSE, &camera->projMatrix[0][0]);
		glDepthMask(GL_FALSE);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-1.0f, -1.0f);
		glDrawArrays(GL_TRIANGLES, 0, triangle->getPoints().size());
		glDisable(GL_POLYGON_OFFSET_FILL);
		glDepthMask(GL_TRUE);
	}

	// 绘制正常三角形
	glm::mat4 triModel = triangle->getModelMatrix();
	glBindVertexArray(tri_object.vao);
	glUseProgram(tri_object.program);
	glUniform1i(tri_object.shadowLocation, 1);
	glUniformMatrix4fv(tri_object.modelLocation, 1, GL_FALSE, &triModel[0][0]);
	glUniformMatrix4fv(tri_object.viewLocation, 1, GL_FALSE, &camera->viewMatrix[0][0]);
	glUniformMatrix4fv(tri_object.projectionLocation, 1, GL_FALSE, &camera->projMatrix[0][0]);
	glDrawArrays(GL_TRIANGLES, 0, triangle->getPoints().size());
}

void printHelp()
{
	std::cout << "Keyboard & Mouse Usage" << std::endl;
	std::cout <<
		"[Window]" << std::endl <<
		"ESC:\t\tExit" << std::endl <<
		"h:\t\tPrint help message" << std::endl <<
		std::endl <<
		"[Light]" << std::endl <<
		"Mouse Left Drag:\tMove light on X/Z" << std::endl <<
		"Click Left:\t\tRecompute & draw shadow" << std::endl <<
		"x/(shift+x):\t\tmove the light along X positive/negative axis" << std::endl <<
		"y/(shift+y):\t\tmove the light along Y positive/negative axis" << std::endl <<
		"z/(shift+z):\t\tmove the light along Z positive/negative axis" << std::endl <<
		"a/(shift+a):\t\tIncrease/Decrease move_step_size" << std::endl <<
		std::endl <<
		"[Camera]" << std::endl <<
		"Mouse Right Drag:\tOrbit camera (yaw/pitch)" << std::endl <<
		"SPACE:\t\tReset camera parameters" << std::endl <<
		"u/(shift+u):\t\tIncrease/Decrease the rotate angle" << std::endl <<
		"i/(shift+i):\t\tIncrease/Decrease the up angle" << std::endl <<
		"o/(shift+o):\t\tIncrease/Decrease the scale" << std::endl;
}

void mainWindow_key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	// 键盘事件处理
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	else if (key == GLFW_KEY_H && action == GLFW_PRESS) {
		printHelp();
	}
	else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		light_position = glm::vec3(0.0f, 1.5f, 1.0f);
		move_step_size = 0.2f;
		shadowDirty = true; // 重算阴影
	}
	else if (key == GLFW_KEY_X && action == GLFW_PRESS && mode == 0x0000) {
		light_position.x += move_step_size;
		shadowDirty = true;
	}
	else if (key == GLFW_KEY_X && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT) {
		light_position.x -= move_step_size;
		shadowDirty = true;
	}
	else if (key == GLFW_KEY_Y && action == GLFW_PRESS && mode == 0x0000) {
		light_position.y += move_step_size;
		shadowDirty = true;
	}
	else if (key == GLFW_KEY_Y && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT) {
		light_position.y -= move_step_size;
		if (light_position.y <= 1.0f) {
			light_position.y = 1.0f; // 保持在平面上方，避免穿透
		}
		shadowDirty = true;
	}
	else if (key == GLFW_KEY_Z && action == GLFW_PRESS && mode == 0x0000) {
		light_position.z += move_step_size;
		shadowDirty = true;
	}
	else if (key == GLFW_KEY_Z && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT) {
		light_position.z -= move_step_size;
		shadowDirty = true;
	}
	else if (key == GLFW_KEY_A && action == GLFW_PRESS && mode == 0x0000) {
		move_step_size += 0.1f;
	}
	else if (key == GLFW_KEY_A && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT) {
		move_step_size -= 0.1f;
		if (move_step_size < 0.05f) move_step_size = 0.05f;
	}
	else {
		// 其他键交给相机处理（u/i/o/space等）
		camera->keyboard(key, action, mode);
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

	GLFWwindow* mainwindow = glfwCreateWindow(600, 600, "李文俊-2023150001-实验3.2", NULL, NULL);
	if (mainwindow == NULL)
	{
		std::cout << "Failed to create GLFW window!" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(mainwindow);
	glfwSetFramebufferSizeCallback(mainwindow, framebuffer_size_callback);
	glfwSetKeyCallback(mainwindow,mainWindow_key_callback);
	// 新增鼠标回调
	glfwSetCursorPosCallback(mainwindow, cursor_position_callback);
	glfwSetMouseButtonCallback(mainwindow, mouse_button_callback);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
	glEnable(GL_DEPTH_TEST);
	init();
    printHelp();
	while (!glfwWindowShouldClose(mainwindow))
    {
        
        display();
        glfwSwapBuffers(mainwindow);
        glfwPollEvents();
		
    }
    glfwTerminate();
    return 0;
}
