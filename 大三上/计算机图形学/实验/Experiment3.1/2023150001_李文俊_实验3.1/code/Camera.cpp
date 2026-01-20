/**
 * 相机类实现文件
 * 实现了OpenGL中相机的观察变换和投影变换
 * 包括lookAt观察变换、正交投影、透视投影等核心功能
 */
#include "Camera.h"

// 构造函数：初始化相机并更新相机参数
Camera::Camera() { updateCamera(); };
Camera::~Camera() {};

/**
 * lookAt函数：计算相机观察变换矩阵
 * 
 * 理论背景：
 * 相机观察变换将世界坐标系中的顶点变换到相机坐标系中
 * 通过指定相机位置(eye)、观察目标(at)和上方向(up)来定位相机
 * 
 * 算法步骤：
 * 1. 计算视线方向向量 n = normalize(eye - at)
 * 2. 计算右方向向量 u = normalize(up × n)  
 * 3. 计算上方向向量 v = n × u
 * 4. 构造观察变换矩阵，包含旋转和平移部分
 * 
 * @param eye 相机位置（视点）
 * @param at  相机观察目标点（参考点）
 * @param up  相机上方向向量（VUP）
 * @return 4x4观察变换矩阵
 */
glm::mat4 Camera::lookAt(const glm::vec4& eye, const glm::vec4& at, const glm::vec4& up)
{	
	// 计算相机坐标系的三个轴向量
	// n: 视线方向（从观察目标指向相机位置，即相机的-z轴方向）
	glm::vec3 n = glm::normalize(glm::vec3(eye - at));  
	
	// u: 右方向（相机的x轴方向），通过up向量与n向量叉乘得到
	glm::vec3 u = glm::normalize(glm::cross(glm::vec3(up), n));  
	
	// v: 上方向（相机的y轴方向），通过n向量与u向量叉乘得到
	glm::vec3 v = glm::cross(n, u);  
	
	// 构造观察变换矩阵
	// 该矩阵将世界坐标系中的点变换到相机坐标系中
	// 矩阵形式：[u.x v.x n.x -u·eye]
	//          [u.y v.y n.y -v·eye]
	//          [u.z v.z n.z -n·eye]
	//          [ 0   0   0    1   ]
	glm::mat4 c = glm::mat4(1.0f);
	c[0][0] = u.x; c[0][1] = v.x; c[0][2] = n.x; c[0][3] = 0.0f;
	c[1][0] = u.y; c[1][1] = v.y; c[1][2] = n.y; c[1][3] = 0.0f;
	c[2][0] = u.z; c[2][1] = v.z; c[2][2] = n.z; c[2][3] = 0.0f;
	
	// 平移部分：将相机从原点移动到指定位置
	c[3][0] = -glm::dot(u, glm::vec3(eye)); 
	c[3][1] = -glm::dot(v, glm::vec3(eye)); 
	c[3][2] = -glm::dot(n, glm::vec3(eye)); 
	c[3][3] = 1.0f;
	
	return c;
}

/**
 * ortho函数：计算正交投影变换矩阵
 * 
 * 理论背景：
 * 正交投影的投影线垂直于观察平面，不会产生透视效果（近大远小）
 * 投影体是一个平行六面体，由6个裁剪平面定义
 * 需要将自定义的正交投影体变换到OpenGL标准视景体[-1,1]³中
 * 
 * 变换过程：
 * 1. 平移：将投影体中心移动到原点
 * 2. 缩放：将投影体缩放到标准大小
 * 
 * @param left   左裁剪平面
 * @param right  右裁剪平面  
 * @param bottom 下裁剪平面
 * @param top    上裁剪平面
 * @param zNear  近裁剪平面
 * @param zFar   远裁剪平面
 * @return 4x4正交投影矩阵
 */
glm::mat4 Camera::ortho(const GLfloat left, const GLfloat right,
	const GLfloat bottom, const GLfloat top,
	const GLfloat zNear, const GLfloat zFar)
{
	// 正交投影矩阵计算
	// 矩阵形式：[2/(r-l)    0       0     -(r+l)/(r-l)]
	//          [   0    2/(t-b)    0     -(t+b)/(t-b)]
	//          [   0       0   -2/(f-n)  -(f+n)/(f-n)]
	//          [   0       0       0           1      ]
	glm::mat4 c = glm::mat4(1.0f);
	
	// x方向缩放和平移
	c[0][0] = 2.0f / (right - left);
	c[3][0] = -(right + left) / (right - left);
	
	// y方向缩放和平移
	c[1][1] = 2.0f / (top - bottom);
	c[3][1] = -(top + bottom) / (top - bottom);
	
	// z方向缩放和平移（注意OpenGL中z轴指向屏幕内部，所以有负号）
	c[2][2] = -2.0f / (zFar - zNear);
	c[3][2] = -(zFar + zNear) / (zFar - zNear);
	
	return c;
}

/**
 * perspective函数：计算透视投影变换矩阵
 * 
 * 理论背景：
 * 透视投影模拟人眼观察效果，产生"近大远小"的透视效果
 * 投影体是一个截头椎体（Frustum），由视角、宽高比和远近裁剪平面定义
 * 
 * 参数转换：
 * 根据视角(fov)和宽高比(aspect)计算截头椎体的边界：
 * top = near * tan(fov/2)
 * right = top * aspect
 * 
 * 变换过程：
 * 1. 透视除法：将3D坐标投影到2D平面
 * 2. 规范化：将投影结果变换到标准视景体中
 * 
 * @param fov    视角（Field of View），以度为单位
 * @param aspect 宽高比（width/height）
 * @param zNear  近裁剪平面距离
 * @param zFar   远裁剪平面距离
 * @return 4x4透视投影矩阵
 */
glm::mat4 Camera::perspective(const GLfloat fov, const GLfloat aspect,
	const GLfloat zNear, const GLfloat zFar)
{
	glm::mat4 c = glm::mat4(1.0f);
	
	// 根据视角和宽高比计算截头椎体的边界
	GLfloat top = zNear * tan(glm::radians(fov) / 2.0f);
	GLfloat bottom = -top;
	GLfloat right = top * aspect;
	GLfloat left = -right;
	
	// 透视投影矩阵计算
	// 矩阵形式：[2n/(r-l)    0      (r+l)/(r-l)      0     ]
	//          [   0     2n/(t-b)  (t+b)/(t-b)      0     ]
	//          [   0        0     -(f+n)/(f-n)  -2fn/(f-n)]
	//          [   0        0         -1            0     ]
	
	// x方向投影
	c[0][0] = 2.0f * zNear / (right - left);
	c[2][0] = (right + left) / (right - left);
	
	// y方向投影  
	c[1][1] = 2.0f * zNear / (top - bottom);
	c[2][1] = (top + bottom) / (top - bottom);
	
	// z方向投影和透视除法
	c[2][2] = -(zFar + zNear) / (zFar - zNear);
	c[2][3] = -1.0f;  // 启用透视除法
	c[3][2] = -2.0f * zFar * zNear / (zFar - zNear);
	c[3][3] = 0.0f;
	
	return c;
}


/**
 * frustum函数：计算任意截头椎体的透视投影矩阵
 * 
 * 理论背景：
 * 这是透视投影的通用形式，允许指定任意的左右、上下边界
 * 与perspective函数不同，frustum可以创建非对称的投影体
 * 常用于立体视觉、阴影映射等高级渲染技术
 * 
 * @param left   左裁剪平面在近平面上的x坐标
 * @param right  右裁剪平面在近平面上的x坐标  
 * @param bottom 下裁剪平面在近平面上的y坐标
 * @param top    上裁剪平面在近平面上的y坐标
 * @param zNear  近裁剪平面距离
 * @param zFar   远裁剪平面距离
 * @return 4x4透视投影矩阵
 */
glm::mat4 Camera::frustum(const GLfloat left, const GLfloat right,
	const GLfloat bottom, const GLfloat top,
	const GLfloat zNear, const GLfloat zFar)
{
	// 任意截头椎体投影矩阵计算
	// 与perspective函数类似，但允许非对称的投影体
	glm::mat4 c = glm::mat4(1.0f);
	c[0][0] = 2.0 * zNear / (right - left);
	c[0][2] = (right + left) / (right - left);
	c[1][1] = 2.0 * zNear / (top - bottom);
	c[1][2] = (top + bottom) / (top - bottom);
	c[2][2] = -(zFar + zNear) / (zFar - zNear);
	c[2][3] = -2.0 * zFar * zNear / (zFar - zNear);
	c[3][2] = -1.0;
	c[3][3] = 0.0;
	c = glm::transpose(c);  // 转置以适应OpenGL的列主序矩阵
	return c;
}


/**
 * updateCamera函数：根据球坐标参数更新相机位置
 * 
 * 球坐标系统：
 * - radius: 相机到观察目标的距离
 * - rotateAngle: 绕Y轴的旋转角度（方位角）
 * - upAngle: 从Y轴正方向的角度（仰角）
 * 
 * 坐标转换公式：
 * x = radius * sin(90° - upAngle) * sin(rotateAngle)
 * y = radius * cos(90° - upAngle)  
 * z = radius * sin(90° - upAngle) * cos(rotateAngle)
 * 
 * 这种方式可以让相机围绕目标对象做球形轨迹运动
 */
void Camera::updateCamera()
{
	// 使用球坐标系计算相机位置
	// upAngle是从Y轴正方向的角度，rotateAngle是绕Y轴的旋转角度
	float eyex = radius * sin(glm::radians(90.0f - upAngle)) * sin(glm::radians(rotateAngle));
	float eyey = radius * cos(glm::radians(90.0f - upAngle));
	float eyez = radius * sin(glm::radians(90.0f - upAngle)) * cos(glm::radians(rotateAngle));
	
	// 设置相机参数
	eye = glm::vec4(eyex, eyey, eyez, 1.0);  // 相机位置
	at = glm::vec4(0.0, 0.0, 0.0, 1.0);     // 观察目标（原点）
	up = glm::vec4(0.0, 1.0, 0.0, 0.0);     // 上方向（世界坐标系Y轴）
	
	// 处理相机翻转情况：当仰角超过±90度时，需要翻转上方向
	if (upAngle > 90){
		up.y = -1;  // 相机倒置时，上方向向下
	}
	else if (upAngle < -90){
		up.y = -1;  // 相机倒置时，上方向向下
	}
}


void Camera::keyboard(int key, int action, int mode)
{

	// 键盘事件处理
	if (key == GLFW_KEY_X && action == GLFW_PRESS && mode == 0x0000)
	{
		rotateAngle += 5.0;
	}
	else if(key == GLFW_KEY_X && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		rotateAngle -= 5.0;
	}
	else if(key == GLFW_KEY_Y && action == GLFW_PRESS && mode == 0x0000)
	{
		upAngle += 5.0;
		if (upAngle > 180)
			upAngle = 180;
	}
	else if(key == GLFW_KEY_Y && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		upAngle -= 5.0;
		if (upAngle < -180)
			upAngle = -180;
	}
	else if(key == GLFW_KEY_R && action == GLFW_PRESS && mode == 0x0000)
	{
		radius += 0.1;
	}
	else if(key == GLFW_KEY_R && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		radius -= 0.1;
	}
	else if(key == GLFW_KEY_F && action == GLFW_PRESS && mode == 0x0000)
	{
		fov += 5.0;
	}
	else if(key == GLFW_KEY_F && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		fov -= 5.0;
	}
	else if(key == GLFW_KEY_A && action == GLFW_PRESS && mode == 0x0000)
	{
		aspect += 0.1;
	}
	else if(key == GLFW_KEY_A && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		aspect -= 0.1;
	}
	else if(key == GLFW_KEY_S && action == GLFW_PRESS && mode == 0x0000)
	{
		scale += 0.1;
	}
	else if(key == GLFW_KEY_S && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		scale -= 0.1;
	}
	else if(key == GLFW_KEY_SPACE && action == GLFW_PRESS && mode == 0x0000)
	{
		// 重置所有参数到默认值
		radius = 4.0;
		rotateAngle = 0.0;
		upAngle = 0.0;
		fov = 45.0;
		aspect = 1.0;
		scale = 1.5;
	}
}
