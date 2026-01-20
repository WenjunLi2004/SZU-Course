#include "Camera.h"

Camera::Camera() { updateCamera(); };
Camera::~Camera() {}

glm::mat4 Camera::getViewMatrix() {
    return this->lookAt(eye, at, up);
}

glm::mat4 Camera::getProjectionMatrix(bool isOrtho) {
    if (isOrtho) return this->ortho(-scale, scale, -scale, scale, this->zNear, this->zFar);
    else return this->perspective(fov, aspect, this->zNear, this->zFar);
}

glm::mat4 Camera::lookAt(const glm::vec4& eye, const glm::vec4& at, const glm::vec4& up)
{

	glm::vec3 n = glm::normalize(glm::vec3(eye - at));
	glm::vec3 u = glm::normalize(glm::cross(glm::vec3(up), n));
	glm::vec3 v = glm::normalize(glm::cross(n, u));

	glm::mat4 viewMatrix = glm::mat4(1.0f);
	viewMatrix[0][0] = u.x;
	viewMatrix[1][0] = u.y;
	viewMatrix[2][0] = u.z;
	viewMatrix[0][1] = v.x;
	viewMatrix[1][1] = v.y;
	viewMatrix[2][1] = v.z;
	viewMatrix[0][2] = n.x;
	viewMatrix[1][2] = n.y;
	viewMatrix[2][2] = n.z;

	glm::mat4 T = glm::mat4(1.0f);
	T[3][0] = -eye.x;
	T[3][1] = -eye.y;
	T[3][2] = -eye.z;
	
	return 	viewMatrix * T;
}

glm::mat4 Camera::ortho(const GLfloat left, const GLfloat right,
	const GLfloat bottom, const GLfloat top,
	const GLfloat zNear, const GLfloat zFar)
{
	glm::mat4 orthoMatrix(1.0f);

	// 正交投影矩阵的计算
	orthoMatrix[0][0] = 2.0f / (right - left);
	orthoMatrix[1][1] = 2.0f / (top - bottom);
	orthoMatrix[2][2] = -2.0f / (zFar - zNear);

	orthoMatrix[3][0] = -(right + left) / (right - left);
	orthoMatrix[3][1] = -(top + bottom) / (top - bottom);
	orthoMatrix[3][2] = -(zFar + zNear) / (zFar - zNear);

	return orthoMatrix;
}

glm::mat4 Camera::perspective(const GLfloat fov, const GLfloat aspect,
	const GLfloat zNear, const GLfloat zFar)
{
	glm::mat4 perspectiveMatrix(0.0f);

	GLfloat top = zNear * tan(glm::radians(fov / 2.0f));
	GLfloat right = top * aspect;

	perspectiveMatrix[0][0] = zNear / right;
	perspectiveMatrix[1][1] = zNear / top;
	perspectiveMatrix[2][2] = -(zFar + zNear) / (zFar - zNear);
	perspectiveMatrix[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);
	perspectiveMatrix[2][3] = -1.0f;

	return perspectiveMatrix;
}

glm::mat4 Camera::frustum(const GLfloat left, const GLfloat right,
	const GLfloat bottom, const GLfloat top,
	const GLfloat zNear, const GLfloat zFar) {
	// 任意视锥矩阵
	glm::mat4 c = glm::mat4(1.0f);
	c[0][0] = 2.0 * zNear / (right - left);
	c[0][2] = (right + left) / (right - left);
	c[1][1] = 2.0 * zNear / (top - bottom);
	c[1][2] = (top + bottom) / (top - bottom);
	c[2][2] = -(zFar + zNear) / (zFar - zNear);
	c[2][3] = -2.0 * zFar * zNear / (zFar - zNear);
	c[3][2] = -1.0;
	c[3][3] = 0.0;

	c = glm::transpose(c);
	return c;
}

void Camera::updateCamera() {
	// 使用相对于 at 的角度控制相机时, 注意 upAngle > 90 时, 相机坐标系的 u 向量反向, 要将 up 的 y 轴改为负方向才不会发生该问题, 也可直接控制相机自身的俯仰角. 
	// 保存 up 、eye-at 等向量, 修改这些向量方向来控制角度. 
	up = glm::vec4(0.0, 1.0, 0.0, 0.0);
	if (upAngle > 90) up.y = -1;
	else if (upAngle < -90) up.y = -1;

	float eyex = radius * cos(upAngle * M_PI / 180.0) * sin(rotateAngle * M_PI / 180.0),
		eyey = radius * sin(upAngle * M_PI / 180.0),
		eyez = radius * cos(upAngle * M_PI / 180.0) * cos(rotateAngle * M_PI / 180.0);

	eye = glm::vec4(eyex, eyey, eyez, 1.0);
	at = glm::vec4(0.0, 0.0, 0.0, 1.0);
}

void Camera::keyboard(int key, int action, int mode)
{
	if (key == GLFW_KEY_U && action == GLFW_PRESS && mode == 0x0000)
	{
		rotateAngle += 5.0;
	}
	else if (key == GLFW_KEY_U && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		rotateAngle -= 5.0;
	}
	else if (key == GLFW_KEY_I && action == GLFW_PRESS && mode == 0x0000)
	{
		upAngle += 5.0;
		if (upAngle > 180)
			upAngle = 180;
	}
	else if (key == GLFW_KEY_I && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		upAngle -= 5.0;
		if (upAngle < -180)
			upAngle = -180;
	}
	else if (key == GLFW_KEY_O && action == GLFW_PRESS && mode == 0x0000)
	{
		radius += 0.1;
	}
	else if (key == GLFW_KEY_O && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		radius -= 0.1;
	}
    else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && mode == 0x0000)
    {
        radius = 4.0;
        rotateAngle = 0.0;
        upAngle = 0.0;
        fov = 45.0;
        aspect = 1.0;
        scale = 1.5;
    }
}
