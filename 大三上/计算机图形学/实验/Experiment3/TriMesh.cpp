#include "TriMesh.h"

// 基础颜色
const glm::vec3 basic_colors[8] = {
	glm::vec3(1.0, 1.0, 1.0),  
	glm::vec3(1.0, 1.0, 0.0),  
	glm::vec3(0.0, 1.0, 0.0),  
	glm::vec3(0.0, 1.0, 1.0),  
	glm::vec3(1.0, 0.0, 1.0),  
	glm::vec3(1.0, 0.0, 0.0),  
	glm::vec3(0.0, 0.0, 0.0),  
	glm::vec3(0.0, 0.0, 1.0)  
};

// 立方体的各个点
const glm::vec3 cube_vertices[8] = {
	glm::vec3(-0.5, -0.5, -0.5),
	glm::vec3(0.5, -0.5, -0.5),
	glm::vec3(-0.5,  0.5, -0.5),
	glm::vec3(0.5,  0.5, -0.5),
	glm::vec3(-0.5, -0.5,  0.5),
	glm::vec3(0.5, -0.5,  0.5),
	glm::vec3(-0.5,  0.5,  0.5),
	glm::vec3(0.5,  0.5,  0.5)
};

// 三角形
const glm::vec3 triangle_vertices[3] = {
	glm::vec3(-0.5, -0.5, 0.0),
	glm::vec3(0.5, -0.5, 0.0),
	glm::vec3(0.0, 0.5, 0.0)
};

// 正方形平面
const glm::vec3 square_vertices[4] = {
	glm::vec3(-0.7, -0.7, 0.0),
	glm::vec3(0.7, -0.7, 0.0),
	glm::vec3(0.7, 0.7, 0.0),
	glm::vec3(-0.7, 0.7, 0.0),
};

TriMesh::TriMesh() {
	scale = glm::vec3(1.0);
	rotation = glm::vec3(0.0);
	translation = glm::vec3(0.0);
}

TriMesh::~TriMesh() {}

std::vector<glm::vec3> TriMesh::getVertexPositions() {
	return vertex_positions;
}

std::vector<glm::vec3> TriMesh::getVertexColors() {
	return vertex_colors;
}

std::vector<glm::vec3> TriMesh::getVertexNormals() {
	return vertex_normals;
}

std::vector<vec3i> TriMesh::getFaces() {
	return faces;
}

std::vector<glm::vec3> TriMesh::getPoints() {
	return points;
}

std::vector<glm::vec3> TriMesh::getColors() {
	return colors;
}

std::vector<glm::vec3> TriMesh::getNormals() {
	return normals;
}

void TriMesh::computeTriangleNormals() {
	face_normals.resize(faces.size());  

	for (size_t i = 0; i < faces.size(); i++) {
		auto& face = faces[i];

		// 计算每个面片的法向量并归一化
		glm::vec3 a(vertex_positions[face.y] - vertex_positions[face.x]);
		glm::vec3 b(vertex_positions[face.z] - vertex_positions[face.x]);
		glm::vec3 norm = glm::normalize(cross(a, b));
		face_normals[i] = norm;
	}
}

void TriMesh::computeVertexNormals() {
	if (face_normals.size() == 0 && faces.size() > 0) computeTriangleNormals();  // 计算面片的法向量

	// resize() 函数会给 vertex_normals 分配一个和 vertex_positions 一样大的空间
	vertex_normals.resize(vertex_positions.size(), glm::vec3(0, 0, 0));  // 初始化法向量为零向量

	// 求法向量均值
	for (size_t i = 0; i < faces.size(); i++) {
		auto& face = faces[i];
		vertex_normals[face.x] += face_normals[i];
		vertex_normals[face.y] += face_normals[i];
		vertex_normals[face.z] += face_normals[i];
	}

	// 对累加的法向量归一化
	for (size_t i = 0; i < vertex_normals.size(); i++)
		vertex_normals[i] = glm::normalize(vertex_normals[i]);
}

glm::vec3 TriMesh::getTranslation() {
	return translation;
}

glm::vec3 TriMesh::getRotation() {
	return rotation;
}

glm::vec3 TriMesh::getScale() {
	return scale;
}

glm::mat4 TriMesh::getModelMatrix() {
	glm::mat4 model = glm::mat4(1.0f);
	glm::vec3 trans = getTranslation();
	model = glm::translate(model, getTranslation());
	model = glm::rotate(model, glm::radians(getRotation()[2]), glm::vec3(0.0, 0.0, 1.0));
	model = glm::rotate(model, glm::radians(getRotation()[1]), glm::vec3(0.0, 1.0, 0.0));
	model = glm::rotate(model, glm::radians(getRotation()[0]), glm::vec3(1.0, 0.0, 0.0));
	model = glm::scale(model, getScale());
	return model;
}

void TriMesh::setTranslation(glm::vec3 translation) {
	this->translation = translation;
}

void TriMesh::setRotation(glm::vec3 rotation) {
	this->rotation= rotation;
}

void TriMesh::setScale(glm::vec3 scale) {
	this->scale = scale;
}

glm::vec4 TriMesh::getAmbient() { return ambient; };
glm::vec4 TriMesh::getDiffuse() { return diffuse; };
glm::vec4 TriMesh::getSpecular() { return specular; };
float TriMesh::getShininess() { return shininess; };

void TriMesh::setAmbient(glm::vec4 _ambient) { ambient = _ambient; };
void TriMesh::setDiffuse(glm::vec4 _diffuse) { diffuse = _diffuse; };
void TriMesh::setSpecular(glm::vec4 _specular) { specular = _specular; };
void TriMesh::setShininess(float _shininess) { shininess = _shininess; };

// 清空 vector
void TriMesh::cleanData() {
	vertex_positions.clear();
	vertex_colors.clear();
	vertex_normals.clear();
	
	faces.clear();
	face_normals.clear();

	points.clear();
	colors.clear();
	normals.clear();
}

void TriMesh::storeFacesPoints() {
	if (vertex_normals.size() == 0) computeVertexNormals();  // 计算法向量

	// 根据每个三角面片的顶点下标存储要传入 GPU 的数据
	for (int i = 0; i < faces.size(); i++) {
		// 坐标
		points.push_back(vertex_positions[faces[i].x]);
		points.push_back(vertex_positions[faces[i].y]);
		points.push_back(vertex_positions[faces[i].z]);

		// 颜色
		colors.push_back(vertex_colors[faces[i].x]);
		colors.push_back(vertex_colors[faces[i].y]);
		colors.push_back(vertex_colors[faces[i].z]);

		// 法向量
		if (vertex_normals.size() != 0) {
			normals.push_back(vertex_normals[faces[i].x]);
			normals.push_back(vertex_normals[faces[i].y]);
			normals.push_back(vertex_normals[faces[i].z]);
		}
	}
}

// 立方体生成 12 个三角形的顶点索引
void TriMesh::generateCube() {
	cleanData();  // 创建顶点前清空 vector

	// 存储立方体的各个面信息
	for (int i = 0; i < 8; i++) {
		vertex_positions.push_back(cube_vertices[i]);
		vertex_colors.push_back(basic_colors[i]);
	}

	// 每个三角面片的顶点下标
	faces.push_back(vec3i(0, 3, 1));
	faces.push_back(vec3i(0, 2, 3));
	faces.push_back(vec3i(1, 5, 4));
	faces.push_back(vec3i(1, 4, 0));
	faces.push_back(vec3i(4, 2, 0));
	faces.push_back(vec3i(4, 6, 2));
	faces.push_back(vec3i(5, 6, 4));
	faces.push_back(vec3i(5, 7, 6));
	faces.push_back(vec3i(2, 6, 7));
	faces.push_back(vec3i(2, 7, 3));
	faces.push_back(vec3i(1, 7, 5));
	faces.push_back(vec3i(1, 3, 7));

	storeFacesPoints();

	normals.clear();

	// 正方形的法向量不能用之前顶点法向量的方法计算, 因为每个四边形平面是正交的, 不是连续曲面
	for (int i = 0; i < faces.size(); i++) {
		normals.push_back( face_normals[i] );
		normals.push_back( face_normals[i] );
		normals.push_back( face_normals[i] );
	}
}

void TriMesh::generateTriangle(glm::vec3 color) {
	cleanData();  // 创建顶点前清空 vector

	for (int i = 0; i < 3; i++) {
		vertex_positions.push_back(triangle_vertices[i]);
		vertex_colors.push_back(color);
	}

	// 每个三角面片的顶点下标
	faces.push_back(vec3i(0, 1, 2));

	storeFacesPoints();
}

void TriMesh::generateSquare(glm::vec3 color) {
	cleanData();  // 创建顶点前清空 vector 

	for (int i = 0; i < 4; i++) {
		vertex_positions.push_back(square_vertices[i]);
		vertex_colors.push_back(color);
	}

	// 每个三角面片的顶点下标
	faces.push_back(vec3i(0, 1, 2)), faces.push_back(vec3i(0, 2, 3));
	storeFacesPoints();
}

void TriMesh::readOff(const std::string& filename) {
    if (filename.empty()) return;

    // fin打开文件读取文件信息
    std::ifstream fin;
    fin.open(filename);  // 读取 OFF 文件中三维模型的信息
    if (!fin) {
        printf("File open failed. \n");
        return;
    }
    else {
        printf("File open successfully. \n");

		cleanData();

        std::string str; fin >> str;  // 读取OFF字符串
        // 读取文件中顶点数、面片数、边数
        int nVertices, nFaces, nEdges; fin >> nVertices >> nFaces >> nEdges;
        // 根据顶点数, 读取每个顶点坐标
        for (int i = 0; i < nVertices; i++) {
            glm::vec3 tmp_node; fin >> tmp_node.x >> tmp_node.y >> tmp_node.z;
            vertex_positions.push_back(tmp_node);
			vertex_colors.push_back(tmp_node);
        }

        // 根据面片数, 读取每个面片信息, 用构建的 vec3i 结构体保存
        for (int i = 0; i < nFaces; i++) {
            // num 记录此面片由几个顶点构成, a、b、c 为构成该面片顶点序号
            int num, a, b, c; fin >> num >> a >> b >> c;
            
            faces.push_back(vec3i(a, b, c));
        }
    }
    fin.close();

    storeFacesPoints();
};

// 求 y = 0 平面上的阴影投影矩阵
glm::mat4 Light::getShadowProjectionMatrix() {
	// 获取光源的模型矩阵
	glm::mat4 modelMatrix = this->getModelMatrix();

	// 将光源位置乘以模型矩阵得到变换后的光源位置
	glm::vec4 light_pos = modelMatrix * glm::vec4(this->translation, 1.0);

	// 提取变换后的光源位置的坐标
	float newX, newY, newZ;
	newX = light_pos[0];
	newY = light_pos[1];
	newZ = light_pos[2];

	// 构建阴影投影矩阵
	return glm::mat4(
		-newZ, 0.0, 0.0, 0.0,
		0, -newZ, 0.0, 0.0,
		newX, newY, 0.0, 1.0,
		0.0, 0.0, 0.0, -newZ
	);
}
