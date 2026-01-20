#include "TriMesh.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

glm::vec3 basic_colors[8] = {
	glm::vec3(1.0, 1.0, 1.0), // White
	glm::vec3(1.0, 1.0, 0.0), // Yellow
	glm::vec3(0.0, 1.0, 0.0), // Green
	glm::vec3(0.0, 1.0, 1.0), // Cyan
	glm::vec3(1.0, 0.0, 1.0), // Magenta
	glm::vec3(1.0, 0.0, 0.0), // Red
	glm::vec3(0.0, 0.0, 0.0), // Black
	glm::vec3(0.0, 0.0, 1.0)  // Blue
};

glm::vec3 cube_vertices[8] = {
	glm::vec3(-0.5, -0.5, -0.5),
	glm::vec3(0.5, -0.5, -0.5),
	glm::vec3(-0.5, 0.5, -0.5),
	glm::vec3(0.5, 0.5, -0.5),
	glm::vec3(-0.5, -0.5, 0.5),
	glm::vec3(0.5, -0.5, 0.5),
	glm::vec3(-0.5, 0.5, 0.5),
	glm::vec3(0.5, 0.5, 0.5)
};

TriMesh::TriMesh()
{

}

TriMesh::~TriMesh()
{

}

std::vector<glm::vec3> TriMesh::getVertexPositions()
{
	return vertex_positions;
}

std::vector<glm::vec3> TriMesh::getVertexColors()
{
	return vertex_colors;
}

std::vector<vec3i> TriMesh::getFaces()
{
	return faces;
}

std::vector<glm::vec3> TriMesh::getPoints()
{
	return points;
}

std::vector<glm::vec3> TriMesh::getColors()
{
	return colors;
}

void TriMesh::cleanData()
{
	vertex_positions.clear();
	vertex_colors.clear();

	faces.clear();

	points.clear();
	colors.clear();
}

void TriMesh::storeFacesPoints() {

	// @TODO: Task-2修改此函数在points和colors容器中存储每个三角面片的各个点和颜色信息
	// 根据每个三角面片的顶点下标存储要传入GPU的数据
	
	// 清空points和colors容器
	points.clear();
	colors.clear();
	
	// 遍历每个面
	for (int i = 0; i < faces.size(); i++) {
		// 获取当前面的三个顶点索引
		vec3i face = faces[i];
		
		// 将三个顶点的位置信息添加到points中
		points.push_back(vertex_positions[face.x]);
		points.push_back(vertex_positions[face.y]);
		points.push_back(vertex_positions[face.z]);
		
		// 将三个顶点的颜色信息添加到colors中
		colors.push_back(vertex_colors[face.x]);
		colors.push_back(vertex_colors[face.y]);
		colors.push_back(vertex_colors[face.z]);
	}
}

void TriMesh::generateCube()
{
	// @TODO: Task2-请在此添加代码完成函数
	// 清空之前的数据
	cleanData();

	// @TODO: Task1-修改此函数，存储立方体的各个面信息
    // vertex_positions和vertex_colors先保存每个顶点的数据
	
	// 将立方体的8个顶点坐标存储到vertex_positions中
	for (int i = 0; i < 8; i++) {
		vertex_positions.push_back(cube_vertices[i]);
		vertex_colors.push_back(basic_colors[i]);
	}
	
	// 定义立方体的12个三角形面（6个面，每个面2个三角形）
	// 前面 (z = -0.5)
	faces.push_back(vec3i(0, 1, 2));
	faces.push_back(vec3i(1, 3, 2));
	
	// 后面 (z = 0.5)
	faces.push_back(vec3i(4, 6, 5));
	faces.push_back(vec3i(5, 6, 7));
	
	// 左面 (x = -0.5)
	faces.push_back(vec3i(0, 2, 4));
	faces.push_back(vec3i(2, 6, 4));
	
	// 右面 (x = 0.5)
	faces.push_back(vec3i(1, 5, 3));
	faces.push_back(vec3i(3, 5, 7));
	
	// 底面 (y = -0.5)
	faces.push_back(vec3i(0, 4, 1));
	faces.push_back(vec3i(1, 4, 5));
	
	// 顶面 (y = 0.5)
	faces.push_back(vec3i(2, 3, 6));
	faces.push_back(vec3i(3, 7, 6));
	
	// 调用storeFacesPoints来生成渲染所需的点和颜色数据
	storeFacesPoints();
}

void TriMesh::readOff(const std::string& filename)
{
	// 清除数据
	cleanData();

	// 打开文件流
	std::ifstream fin(filename);
	std::string line;
	if (!fin.is_open())
	{
		std::cout << "Error: Cannot open file " << filename << std::endl;
		return;
	}

	// 读取OFF字符串
	std::getline(fin, line);
	// 读取顶点数、面数、边数
	int numVertices, numFaces, numEdges;
	fin >> numVertices >> numFaces >> numEdges;

	// 读取顶点
	for (int i = 0; i < numVertices; i++)
	{
		glm::vec3 vertex;
		fin >> vertex.x >> vertex.y >> vertex.z;
		vertex_positions.push_back(vertex);
		vertex_colors.push_back(glm::vec3(1.0, 0.5, 0.5));
	}

	// 读取面
	for (int i = 0; i < numFaces; i++)
	{
		int num, v1, v2, v3;
		fin >> num >> v1 >> v2 >> v3;
		faces.push_back(vec3i(v1, v2, v3));
	}

	// 调用函数生成points和colors
	storeFacesPoints();
}