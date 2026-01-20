#include "Angel.h"
#include "TriMesh.h"
#include "Camera.h"
#include "MeshPainter.h"

#include <vector>
#include <string>

int WIDTH = 600;
int HEIGHT = 600;
int mainWindow;

Camera* camera = new Camera();
Light* light = new Light();
MeshPainter *painter = new MeshPainter();

// 这个用来回收和删除我们创建的物体对象
std::vector<TriMesh *> meshList;


void framebuffer_size_callback(GLFWwindow * window, int width, int height)
{
	glViewport(0, 0, width, height);
}


void init()
{
	std::string vshader, fshader;
	// 读取着色器并使用
	vshader = "shaders/vshader.glsl";
	fshader = "shaders/fshader.glsl";

	// 设置光源位置
	light->setTranslation(glm::vec3(0.0, 0.0, 2.0));
	light->setAmbient(glm::vec4(1.0, 1.0, 1.0, 1.0)); // 环境光
	light->setDiffuse(glm::vec4(1.0, 1.0, 1.0, 1.0)); // 漫反射
	light->setSpecular(glm::vec4(1.0, 1.0, 1.0, 1.0)); // 镜面反射
	light->setAttenuation(1.0, 0.045, 0.0075); // 衰减系数


	openGLObject mesh_object;
	TriMesh* cylinder = new TriMesh();	
	// 创建圆柱体
	cylinder->generateCylinder(100, 0.1, 0.3);
	// 设置物体的旋转位移
	cylinder->setTranslation(glm::vec3(-0.5, 0.0, 0.0));
	cylinder->setRotation(glm::vec3(90.0, 0.0, 0.0));
	cylinder->setScale(glm::vec3(1.0, 1.0, 1.0));
	// 设置材质（不过本次实验中不会用到光照，感兴趣的图像结合ppt的扩展内容将着色器进行修改）
	cylinder->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 环境光
	cylinder->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0)); // 漫反射
	cylinder->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0)); // 镜面反射
	cylinder->setShininess(1.0); //高光系数
	// 加到painter中
	painter->addMesh(cylinder, "mesh_a", "./assets/cylinder10.jpg", vshader, fshader); 	// 指定纹理与着色器
	// 我们创建的这个加入一个容器内，为了程序结束时将这些数据释放
	meshList.push_back(cylinder);

	TriMesh* disk = new TriMesh();
    // 生成圆盘几何与纹理坐标：半径设为0.25，对应 assets/disk.jpg
    // TriMesh::generateDisk 内部采用三角扇并将平面坐标映射到UV中心(0.5,0.5)
    disk->generateDisk(100, 0.25f);
	disk->setTranslation(glm::vec3(0.0, 0.0, 0.0));
	disk->setRotation(glm::vec3(0.0, 0.0, 0.0));
	disk->setScale(glm::vec3(1.0, 1.0, 1.0));
	disk->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0));
	disk->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0));
	disk->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0));
	disk->setShininess(1.0);
	painter->addMesh(disk, "mesh_b", "./assets/disk.jpg", vshader, fshader);
	meshList.push_back(disk);


 
	TriMesh* cone = new TriMesh();
    // 生成圆锥侧面：底半径0.25，高度0.25（与圆盘尺寸相当），贴图 assets/cone.jpg
    // 圆锥在模型坐标系下沿Z轴生成；这里绕X轴旋转-90°使其朝向+Y方向“朝上”
    cone->generateCone(100, 0.25f, 0.25f);
    cone->setTranslation(glm::vec3(0.5, 0.0, 0.0));
    cone->setRotation(glm::vec3(-90.0, 0.0, 0.0));
	cone->setScale(glm::vec3(1.0, 1.0, 1.0));
	cone->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0));
	cone->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0));
	cone->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0));
	cone->setShininess(1.0);
	painter->addMesh(cone, "mesh_c", "./assets/cone.jpg", vshader, fshader);
	meshList.push_back(cone);
	


	glClearColor(1.0, 1.0, 1.0, 1.0);
}


void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	painter->drawMeshes(light, camera);
}


void printHelp()
{
	std::cout << "================================================" << std::endl;
	std::cout << "Use mouse to controll the light position (drag)." << std::endl;
	std::cout << "================================================" << std::endl << std::endl;

	std::cout << "Keyboard Usage" << std::endl;
	std::cout <<
		"[Window]" << std::endl <<
		"ESC:		Exit" << std::endl <<
		"h:		Print help message" << std::endl <<

		std::endl <<
		"[Camera]" << std::endl <<
		"SPACE:		Reset camera parameters" << std::endl <<
		"u/U:		Increase/Decrease the rotate angle" << std::endl <<
		"i/I:		Increase/Decrease the up angle" << std::endl <<
		"o/O:		Increase/Decrease the camera radius" << std::endl << std::endl;
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	float tmp;
	glm::vec4 ambient;
	if (action == GLFW_PRESS) {
		switch (key)
		{
		case GLFW_KEY_ESCAPE: exit(EXIT_SUCCESS); break;
		case GLFW_KEY_H: printHelp(); break;
		default:
			camera->keyboard(key, action, mode);
			break;
		}
	}
}


void cleanData() {
	// 释放内存
	delete camera;
	camera = NULL;

	delete light;
	light = NULL;

	painter->cleanMeshes();

	delete painter;
	painter = NULL;
	
	for (int i=0; i<meshList.size(); i++) {
		delete meshList[i];
	}
	meshList.clear();
}


int main(int argc, char **argv)
{
	// 初始化GLFW库，必须是应用程序调用的第一个GLFW函数
	glfwInit();

	// 配置GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// 配置窗口属性
	GLFWwindow* window = glfwCreateWindow(600, 600, "2023150001_李文俊_实验4.1", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	// Init mesh, shaders, buffer
	init();
	// 输出帮助信息
	printHelp();
	// 启用深度测试
	glEnable(GL_DEPTH_TEST);
	while (!glfwWindowShouldClose(window))
	{
		display();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	cleanData();
	return 0;
}
