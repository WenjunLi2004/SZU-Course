/*
 *        Computer Graphics Course - Shenzhen University
 *    Mid-term Assignment - Tetris implementation sample code
 * ============================================================
 *
 * - 本代码仅仅是参考代码，具体要求请参考作业说明，按照顺序逐步完成。
 * - 关于配置OpenGL开发环境、编译运行，请参考第一周实验课程相关文档。
 *
 * - 已实现功能如下：
 * - 1) 绘制棋盘格和7种方块（O/I/S/Z/L/J/T），随机初始方向与颜色
 * - 2) 自动向下移动；方向键控制：左/右移动，上键旋转，下键加速
 * - 3) 边界与叠加碰撞检测（方块与方块及边界）
 * - 4) 满行消除，上方方块整体下落并保持颜色
 * - 5) 游戏结束检测（顶部无法生成），“q”退出，“r”重新开始
 *
 */

#include "include/Angel.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <ctime>
#include <algorithm>

int starttime;			// 控制方块向下移动时间
int rotation = 0;		// 控制当前窗口中的方块旋转
glm::vec2 tile[4];			// 表示当前窗口中的方块
bool gameover = false;	// 游戏结束控制变量
int xsize = 400;		// 窗口大小（尽量不要变动窗口大小！）
int ysize = 720;

// 单个网格大小
int tile_width = 32;

// 网格布大小
const int board_width = 10;
const int board_height = 20;

// 网格三角面片的顶点数量
const int points_num = board_height * board_width * 6;

// 我们用画直线的方法绘制网格
// 包含竖线 board_width+1 条
// 包含横线 board_height+1 条
// 一条线2个顶点坐标
// 网格线的数量
const int board_line_num =  (board_width + 1) + (board_height + 1);


// 一个二维数组表示所有可能出现的方块和方向。
glm::vec2 allRotationsLshape[4][4] =
						  {{glm::vec2(0, 0), glm::vec2(-1,0), glm::vec2(1, 0), glm::vec2(-1,-1)},	//   "L"
						   {glm::vec2(0, 1), glm::vec2(0, 0), glm::vec2(0,-1), glm::vec2(1, -1)},   //
						   {glm::vec2(1, 1), glm::vec2(-1,0), glm::vec2(0, 0), glm::vec2(1,  0)},   //
						   {glm::vec2(-1,1), glm::vec2(0, 1), glm::vec2(0, 0), glm::vec2(0, -1)}};

// 7种俄罗斯方块的所有旋转（O, I, S, Z, L, J, T），每个形状4种旋转
glm::vec2 allShapes[7][4][4] = {
    // O
    {
        {glm::vec2(0,0), glm::vec2(1,0), glm::vec2(0,-1), glm::vec2(1,-1)},
        {glm::vec2(0,0), glm::vec2(1,0), glm::vec2(0,-1), glm::vec2(1,-1)},
        {glm::vec2(0,0), glm::vec2(1,0), glm::vec2(0,-1), glm::vec2(1,-1)},
        {glm::vec2(0,0), glm::vec2(1,0), glm::vec2(0,-1), glm::vec2(1,-1)}
    },
    // I
    {
        {glm::vec2(0,0), glm::vec2(-1,0), glm::vec2(1,0), glm::vec2(2,0)},
        {glm::vec2(0,0), glm::vec2(0,-1), glm::vec2(0,1), glm::vec2(0,2)},
        {glm::vec2(0,0), glm::vec2(-1,0), glm::vec2(1,0), glm::vec2(2,0)},
        {glm::vec2(0,0), glm::vec2(0,-1), glm::vec2(0,1), glm::vec2(0,2)}
    },
    // S
    {
        {glm::vec2(0,0), glm::vec2(-1,0), glm::vec2(0,-1), glm::vec2(1,-1)},
        {glm::vec2(0,0), glm::vec2(0,1), glm::vec2(1,0), glm::vec2(1,-1)},
        {glm::vec2(0,0), glm::vec2(-1,0), glm::vec2(0,-1), glm::vec2(1,-1)},
        {glm::vec2(0,0), glm::vec2(0,1), glm::vec2(1,0), glm::vec2(1,-1)}
    },
    // Z（修正：避免旋转后变成O）
    {
        {glm::vec2(0,0), glm::vec2(-1,-1), glm::vec2(0,-1), glm::vec2(1,0)},
        {glm::vec2(0,0), glm::vec2(-1,1), glm::vec2(-1,0), glm::vec2(0,-1)},
        {glm::vec2(0,0), glm::vec2(1,1), glm::vec2(0,1), glm::vec2(-1,0)},
        {glm::vec2(0,0), glm::vec2(1,-1), glm::vec2(1,0), glm::vec2(0,1)}
    },
    // L（沿用示例给出的L）
    {
        {glm::vec2(0, 0), glm::vec2(-1,0), glm::vec2(1, 0), glm::vec2(-1,-1)},
        {glm::vec2(0, 1), glm::vec2(0, 0), glm::vec2(0,-1), glm::vec2(1, -1)},
        {glm::vec2(1, 1), glm::vec2(-1,0), glm::vec2(0, 0), glm::vec2(1,  0)},
        {glm::vec2(-1,1), glm::vec2(0, 1), glm::vec2(0, 0), glm::vec2(0, -1)}
    },
    // J（L的镜像）
    {
        {glm::vec2(0, 0), glm::vec2(-1,0), glm::vec2(1, 0), glm::vec2(1,-1)},
        {glm::vec2(0, 1), glm::vec2(0, 0), glm::vec2(0,-1), glm::vec2(-1, -1)},
        {glm::vec2(-1,1), glm::vec2(1, 0), glm::vec2(0, 0), glm::vec2(-1,  0)},
        {glm::vec2(1,1), glm::vec2(0, 1), glm::vec2(0, 0), glm::vec2(0, -1)}
    },
    // T
    {
        {glm::vec2(0,0), glm::vec2(-1,0), glm::vec2(1,0), glm::vec2(0,-1)},
        {glm::vec2(0,0), glm::vec2(0,1), glm::vec2(0,-1), glm::vec2(1,0)},
        {glm::vec2(0,0), glm::vec2(-1,0), glm::vec2(1,0), glm::vec2(0,1)},
        {glm::vec2(0,0), glm::vec2(0,1), glm::vec2(0,-1), glm::vec2(-1,0)}
    }
};

// 当前方块的类型与颜色
int currentShape = 4; // 默认L
glm::vec4 current_colour = glm::vec4(1.0, 0.5, 0.0, 1.0);

// 颜色调色板
glm::vec4 palette[7] = {
    glm::vec4(1.0, 0.0, 0.0, 1.0), // 红
    glm::vec4(1.0, 1.0, 0.0, 1.0), // 黄
    glm::vec4(0.0, 1.0, 0.0, 1.0), // 绿
    glm::vec4(0.0, 0.0, 1.0, 1.0), // 蓝
    glm::vec4(1.0, 0.5, 0.0, 1.0), // 橙
    glm::vec4(0.7, 0.2, 0.9, 1.0), // 紫
    glm::vec4(0.0, 1.0, 1.0, 1.0)  // 青
};

// 自动下落控制
double lastFallTime = 0.0;
double fallInterval = 0.6; // 秒

// 计分与状态
int score = 0;
bool paused = false;
GLFWwindow* g_window = nullptr;

void update_ui()
{
    std::string title = "Mid-Term Tetris - Score: " + std::to_string(score);
    if (paused) title += " [Paused]";
    if (g_window) glfwSetWindowTitle(g_window, title.c_str());
    // 速度随分数提升：每10行（1000分）降低0.05s，下限0.1s
    int level = score / 1000;
    fallInterval = std::max(0.1, 0.6 - 0.05 * level);
}
// 绘制窗口的颜色变量
glm::vec4 orange = glm::vec4(1.0, 0.5, 0.0, 1.0);
glm::vec4 white  = glm::vec4(1.0, 1.0, 1.0, 1.0);
glm::vec4 black  = glm::vec4(0.0, 0.0, 0.0, 1.0);

// 当前方块的位置（以棋盘格的左下角为原点的坐标系）
glm::vec2 tilepos = glm::vec2(5, 19);

// 布尔数组表示棋盘格的某位置是否被方块填充，即board[x][y] = true表示(x,y)处格子被填充。
// （以棋盘格的左下角为原点的坐标系）
bool board[board_width][board_height];

// 当棋盘格某些位置被方块填充之后，记录这些位置上被填充的颜色
glm::vec4 board_colours[points_num];

GLuint locxsize;
GLuint locysize;

GLuint vao[3];
GLuint vbo[6];
// 影子/HUD覆盖绘制专用全局VAO/VBO
GLuint overlay_vao = 0;
GLuint overlay_vbo_pos = 0;
GLuint overlay_vbo_col = 0;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    xsize = width;
    ysize = height;
}

// 修改棋盘格在pos位置的颜色为colour，并且更新对应的VBO
void changecellcolour(glm::vec2 pos, glm::vec4 colour)
{
	// 每个格子是个正方形，包含两个三角形，总共6个定点，并在特定的位置赋上适当的颜色
	for (int i = 0; i < 6; i++)
		board_colours[(int)( 6 * ( board_width*pos.y + pos.x) + i)] = colour;

	glm::vec4 newcolours[6] = {colour, colour, colour, colour, colour, colour};

	glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);

	// 计算偏移量，在适当的位置赋上颜色
	int offset = 6 * sizeof(glm::vec4) * (int)( board_width * pos.y + pos.x);
	glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(newcolours), newcolours);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// 当前方块移动或者旋转时，更新VBO
void updatetile()
{
	glBindBuffer(GL_ARRAY_BUFFER, vbo[4]);

	// 每个方块包含四个格子
	for (int i = 0; i < 4; i++)
	{
		// 计算格子的坐标值
		GLfloat x = tilepos.x + tile[i].x;
		GLfloat y = tilepos.y + tile[i].y;

		glm::vec4 p1 = glm::vec4(tile_width + (x * tile_width), tile_width + (y * tile_width), .4, 1);
		glm::vec4 p2 = glm::vec4(tile_width + (x * tile_width), tile_width*2 + (y * tile_width), .4, 1);
		glm::vec4 p3 = glm::vec4(tile_width*2 + (x * tile_width), tile_width + (y * tile_width), .4, 1);
		glm::vec4 p4 = glm::vec4(tile_width*2 + (x * tile_width), tile_width*2 + (y * tile_width), .4, 1);

		// 每个格子包含两个三角形，所以有6个顶点坐标
		glm::vec4 newpoints[6] = {p1, p2, p3, p2, p3, p4};
		glBufferSubData(GL_ARRAY_BUFFER, i*6*sizeof(glm::vec4), 6*sizeof(glm::vec4), newpoints);
	}
	glBindVertexArray(0);
	
}

// 设置当前方块为下一个即将出现的方块。在游戏开始的时候调用来创建一个初始的方块，
// 在游戏结束的时候判断，没有足够的空间来生成新的方块。
// 前置声明：检查格子有效性（边界内且未被占用）
bool checkvalid(glm::vec2 cellpos);
// HUD/影子预览函数前置声明，供display调用
void draw_cells(const std::vector<glm::vec2>& cells, const glm::vec4& colour, float z = 0.6f);
void draw_digit(int d, int baseX, int baseY, const glm::vec4& col);
void draw_score_hud();
void draw_ghost();

void newtile()
{
    // 将新方块放于棋盘格的最上行中间位置并设置随机旋转方向与形状
    tilepos = glm::vec2(5 , 19);
    rotation = rand() % 4;
    currentShape = rand() % 7; // 7种方块
    current_colour = palette[currentShape];

    // 根据当前形状与旋转设置方块格子
    for (int i = 0; i < 4; i++)
        tile[i] = allShapes[currentShape][rotation][i];

    // 顶部生成区：允许最高单元超出棋盘一行
    int maxYOffset = (int)tile[0].y;
    for (int i = 1; i < 4; ++i) maxYOffset = std::max(maxYOffset, (int)tile[i].y);
    const int spawn_buffer = 1; // 允许最高块在棋盘之上1行
    tilepos.y = board_height - 1 + spawn_buffer - maxYOffset;

    // 检查能否生成（棋盘内的单元不可与占用格重叠；棋盘上方的单元放行）
    bool canSpawn = true;
    for (int i = 0; i < 4; i++){
        glm::vec2 p = tile[i] + tilepos;
        if(!checkvalid(p)) { canSpawn = false; break; }
    }
    if(!canSpawn){
        gameover = true;
        return;
    }

    updatetile();

    // 给新方块赋上颜色
    glm::vec4 newcolours[24];
    for (int i = 0; i < 24; i++)
        newcolours[i] = current_colour;

    glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(newcolours), newcolours);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
    lastFallTime = glfwGetTime();
}

// 游戏和OpenGL初始化
void init()
{
	// 初始化棋盘格，这里用画直线的方法绘制网格
	// 包含竖线 board_width+1 条
	// 包含横线 board_height+1 条
	// 一条线2个顶点坐标，并且每个顶点一个颜色值
	
	glm::vec4 gridpoints[board_line_num * 2];
	glm::vec4 gridcolours[board_line_num * 2];

	// 绘制网格线
	// 纵向线
	for (int i = 0; i < (board_width+1); i++)
	{
		gridpoints[2*i] = glm::vec4((tile_width + (tile_width * i)), tile_width, 0, 1);
		gridpoints[2*i + 1] = glm::vec4((tile_width + (tile_width * i)), (board_height+1) * tile_width, 0, 1);
	}

	// 水平线
	for (int i = 0; i < (board_height+1); i++)
	{
		gridpoints[ 2*(board_width+1) + 2*i ] = glm::vec4(tile_width, (tile_width + (tile_width * i)), 0, 1);
		gridpoints[ 2*(board_width+1) + 2*i + 1 ] = glm::vec4((board_width+1) * tile_width, (tile_width + (tile_width * i)), 0, 1);
	}

	// 将所有线赋成白色
	for (int i = 0; i < (board_line_num * 2); i++)
		gridcolours[i] = white;

	// 初始化棋盘格，并将没有被填充的格子设置成黑色
	glm::vec4 boardpoints[points_num];
	for (int i = 0; i < points_num; i++)
		board_colours[i] = black;

	// 对每个格子，初始化6个顶点，表示两个三角形，绘制一个正方形格子
	for (int i = 0; i < board_height; i++)
		for (int j = 0; j < board_width; j++)
		{
			glm::vec4 p1 = glm::vec4(tile_width + (j * tile_width), tile_width + (i * tile_width), .5, 1);
			glm::vec4 p2 = glm::vec4(tile_width + (j * tile_width), tile_width*2 + (i * tile_width), .5, 1);
			glm::vec4 p3 = glm::vec4(tile_width*2 + (j * tile_width), tile_width + (i * tile_width), .5, 1);
			glm::vec4 p4 = glm::vec4(tile_width*2 + (j * tile_width), tile_width*2 + (i * tile_width), .5, 1);
			boardpoints[ 6 * ( board_width * i + j ) + 0 ] = p1;
			boardpoints[ 6 * ( board_width * i + j ) + 1 ] = p2;
			boardpoints[ 6 * ( board_width * i + j ) + 2 ] = p3;
			boardpoints[ 6 * ( board_width * i + j ) + 3 ] = p2;
			boardpoints[ 6 * ( board_width * i + j ) + 4 ] = p3;
			boardpoints[ 6 * ( board_width * i + j ) + 5 ] = p4;
		}

	// 将棋盘格所有位置的填充与否都设置为false（没有被填充）
	for (int i = 0; i < board_width; i++)
		for (int j = 0; j < board_height; j++)
			board[i][j] = false;

	// 载入着色器
	std::string vshader, fshader;
	vshader = "shaders/vshader.glsl";
	fshader = "shaders/fshader.glsl";
	GLuint program = InitShader(vshader.c_str(), fshader.c_str());
	glUseProgram(program);

	locxsize = glGetUniformLocation(program, "xsize");
	locysize = glGetUniformLocation(program, "ysize");

	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	GLuint vColor = glGetAttribLocation(program, "vColor");

	
	glGenVertexArrays(3, &vao[0]);
	glBindVertexArray(vao[0]);		// 棋盘格顶点
	
	glGenBuffers(2, vbo);

	// 棋盘格顶点位置
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, (board_line_num * 2) * sizeof(glm::vec4), gridpoints, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// 棋盘格顶点颜色
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, (board_line_num * 2) * sizeof(glm::vec4), gridcolours, GL_STATIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);

	
	glBindVertexArray(vao[1]);		// 棋盘格每个格子

	glGenBuffers(2, &vbo[2]);

	// 棋盘格每个格子顶点位置
	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, points_num*sizeof(glm::vec4), boardpoints, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// 棋盘格每个格子顶点颜色
	glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
	glBufferData(GL_ARRAY_BUFFER, points_num*sizeof(glm::vec4), board_colours, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);

	
	glBindVertexArray(vao[2]);		// 当前方块

	glGenBuffers(2, &vbo[4]);

	// 当前方块顶点位置
	glBindBuffer(GL_ARRAY_BUFFER, vbo[4]);
	glBufferData(GL_ARRAY_BUFFER, 24*sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// 当前方块顶点颜色
	glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
	glBufferData(GL_ARRAY_BUFFER, 24*sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);

	// 额外的覆盖绘制VAO/VBO（影子与HUD使用，避免覆盖当前方块VBO）
	// 覆盖绘制（影子/HUD）专用VAO/VBO，避免与当前方块VBO冲突
	// 移除局部声明，改用全局变量
	glGenVertexArrays(1, &overlay_vao);
	glBindVertexArray(overlay_vao);
	glGenBuffers(1, &overlay_vbo_pos);
	glBindBuffer(GL_ARRAY_BUFFER, overlay_vbo_pos);
	glBufferData(GL_ARRAY_BUFFER, 24*sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	glGenBuffers(1, &overlay_vbo_col);
	glBindBuffer(GL_ARRAY_BUFFER, overlay_vbo_col);
	glBufferData(GL_ARRAY_BUFFER, 24*sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);

	glBindVertexArray(0);

	glClearColor(0, 0, 0, 0);

	newtile();
}

// 检查在cellpos位置的格子是否被填充或者是否在棋盘格的边界范围内
bool checkvalid(glm::vec2 cellpos)
{
    int x = (int)cellpos.x;
    int y = (int)cellpos.y;
    // 左右越界或落到棋盘下方无效
    if (x < 0 || x >= board_width || y < 0) return false;
    // 在棋盘上方（生成区）视为有效
    if (y >= board_height) return true;
    // 在棋盘内：不能与已占用格子重叠
    return !board[x][y];
}

// 在棋盘上有足够空间的情况下旋转当前方块
void rotate()
{
    int nextrotation = (rotation + 1) % 4;

    // 检查旋转后4个格子的有效性
    if (checkvalid((allShapes[currentShape][nextrotation][0]) + tilepos)
        && checkvalid((allShapes[currentShape][nextrotation][1]) + tilepos)
        && checkvalid((allShapes[currentShape][nextrotation][2]) + tilepos)
        && checkvalid((allShapes[currentShape][nextrotation][3]) + tilepos))
    {
        rotation = nextrotation;
        for (int i = 0; i < 4; i++)
            tile[i] = allShapes[currentShape][rotation][i];
        updatetile();
    }
}

// 检查棋盘格在row行有没有被填充满
void checkfullrow(int row)
{
    // 如果 row < 0，检查所有行；否则仅检查指定行
    int start = (row >= 0) ? row : 0;
    int end   = (row >= 0) ? row : (board_height - 1);
    int cleared = 0;

    for (int y = start; y <= end; ++y){
        bool full = true;
        for (int x = 0; x < board_width; ++x){
            if(!board[x][y]){ full = false; break; }
        }
        if(full){
            // 该行消除，并将其上的所有行下移一格
            for (int x = 0; x < board_width; ++x){
                board[x][y] = false;
                changecellcolour(glm::vec2(x, y), black);
            }
            for (int yy = y + 1; yy < board_height; ++yy){
                for (int x = 0; x < board_width; ++x){
                    board[x][yy-1] = board[x][yy];
                    // 拷贝颜色：读取上方格子的颜色到下方
                    glm::vec4 col = board_colours[ 6 * ( board_width * yy + x) ];
                    changecellcolour(glm::vec2(x, yy-1), col);
                }
            }
            // 顶部新出现的行置空/置黑
            for (int x = 0; x < board_width; ++x){
                board[x][board_height-1] = false;
                changecellcolour(glm::vec2(x, board_height-1), black);
            }
            // 处理连续多行消除：调整y以继续检查当前行
            y--; 
            cleared++;
        }
    }
    if (cleared > 0){
        score += cleared * 100;
        update_ui();
    }
}

// 放置当前方块，并且更新棋盘格对应位置顶点的颜色VBO
void settile()
{
    for (int i = 0; i < 4; i++)
    {
        int x = (tile[i] + tilepos).x;
        int y = (tile[i] + tilepos).y;
        // 仅写入棋盘内的单元，防止越界导致状态污染
        if (x >= 0 && x < board_width && y >= 0 && y < board_height){
            board[x][y] = true;
            changecellcolour(glm::vec2(x, y), current_colour);
        }
    }
}

// 给定位置(x,y)，移动方块。有效的移动值为(-1,0)，(1,0)，(0,-1)，分别对应于向
// 左，向下和向右移动。如果移动成功，返回值为true，反之为false
bool movetile(glm::vec2 direction)
{
    glm::vec2 newtilepos[4];
    for (int i = 0; i < 4; i++)
        newtilepos[i] = tile[i] + tilepos + direction;

    if (checkvalid(newtilepos[0])
        && checkvalid(newtilepos[1])
        && checkvalid(newtilepos[2])
        && checkvalid(newtilepos[3]))
        {
            tilepos.x = tilepos.x + direction.x;
            tilepos.y = tilepos.y + direction.y;
            updatetile();
            return true;
        }
    return false;
}

// 重新启动游戏
void restart()
{
    gameover = false;
    // 清空棋盘
    for (int i = 0; i < board_width; i++)
        for (int j = 0; j < board_height; j++){
            board[i][j] = false;
            changecellcolour(glm::vec2(i, j), black);
        }
    // 分数与暂停状态复位
    score = 0;
    paused = false;
    update_ui();
    // 重新生成方块
    newtile();
}

// 游戏渲染部分
void display()
{
    // 自动下落
    if(!gameover && !paused){
        double now = glfwGetTime();
        if(now - lastFallTime >= fallInterval){
            if (!movetile(glm::vec2(0, -1))) {
                settile();
                checkfullrow(-1); // 检查并消除满行
                newtile();
            }
            lastFallTime = now;
        }
    }

    glClear(GL_COLOR_BUFFER_BIT);

    glUniform1i(locxsize, xsize);
    glUniform1i(locysize, ysize);

    glBindVertexArray(vao[1]);
    glDrawArrays(GL_TRIANGLES, 0, points_num);

    // 影子预览（使用当前方块VAO，先画ghost）
    draw_ghost();

    // 当前方块
    glBindVertexArray(vao[2]);
    glDrawArrays(GL_TRIANGLES, 0, 24);

    // 网格线
    glBindVertexArray(vao[0]);
    glDrawArrays(GL_LINES, 0, board_line_num * 2 );

    // HUD分数（已移除窗口内显示）

    // 游戏结束分数面板已移除
}

// 画若干格子（不限定在棋盘内），用于HUD或影子预览
void draw_cells(const std::vector<glm::vec2>& cells, const glm::vec4& colour, float z)
{
    if (cells.empty()) return;
    size_t n = std::min<size_t>(cells.size(), 4);
    std::vector<glm::vec4> points(n * 6);
    for (size_t i = 0; i < n; ++i){
        float x = cells[i].x;
        float y = cells[i].y;
        glm::vec4 p1 = glm::vec4(tile_width + (x * tile_width), tile_width + (y * tile_width), z, 1);
        glm::vec4 p2 = glm::vec4(tile_width + (x * tile_width), tile_width*2 + (y * tile_width), z, 1);
        glm::vec4 p3 = glm::vec4(tile_width*2 + (x * tile_width), tile_width + (y * tile_width), z, 1);
        glm::vec4 p4 = glm::vec4(tile_width*2 + (x * tile_width), tile_width*2 + (y * tile_width), z, 1);
        points[i*6 + 0] = p1; points[i*6 + 1] = p2; points[i*6 + 2] = p3;
        points[i*6 + 3] = p2; points[i*6 + 4] = p3; points[i*6 + 5] = p4;
    }
    std::vector<glm::vec4> cols(n * 6, colour);

    glBindBuffer(GL_ARRAY_BUFFER, overlay_vbo_pos);
    glBufferSubData(GL_ARRAY_BUFFER, 0, points.size()*sizeof(glm::vec4), points.data());
    glBindBuffer(GL_ARRAY_BUFFER, overlay_vbo_col);
    glBufferSubData(GL_ARRAY_BUFFER, 0, cols.size()*sizeof(glm::vec4), cols.data());
    glBindVertexArray(overlay_vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(n * 6));
}

// 3x5 数码管样式数字（1表示画格）
const int DIGIT_W = 3;
const int DIGIT_H = 5;
int digit_bits[10][DIGIT_W*DIGIT_H] = {
    // 0
    {1,1,1,
     1,0,1,
     1,0,1,
     1,0,1,
     1,1,1},
    // 1
    {0,1,0,
     0,1,0,
     0,1,0,
     0,1,0,
     1,1,1},
    // 2
    {1,1,1,
     0,0,1,
     1,1,1,
     1,0,0,
     1,1,1},
    // 3
    {1,1,1,
     0,0,1,
     1,1,1,
     0,0,1,
     1,1,1},
    // 4
    {1,0,1,
     1,0,1,
     1,1,1,
     0,0,1,
     0,0,1},
    // 5
    {1,1,1,
     1,0,0,
     1,1,1,
     0,0,1,
     1,1,1},
    // 6
    {1,1,1,
     1,0,0,
     1,1,1,
     1,0,1,
     1,1,1},
    // 7
    {1,1,1,
     0,0,1,
     0,0,1,
     0,0,1,
     0,0,1},
    // 8
    {1,1,1,
     1,0,1,
     1,1,1,
     1,0,1,
     1,1,1},
    // 9
    {1,1,1,
     1,0,1,
     1,1,1,
     0,0,1,
     1,1,1}
};

void draw_digit(int d, int baseX, int baseY, const glm::vec4& col)
{
    if (d < 0 || d > 9) return;
    std::vector<glm::vec2> batch;
    batch.reserve(4);
    int count_in_batch = 0;
    for (int r = 0; r < DIGIT_H; ++r){
        for (int c = 0; c < DIGIT_W; ++c){
            if (digit_bits[d][r*DIGIT_W + c]){
                // y 从下到上递增，这里将top行摆在更高的y
                int x = baseX + c;
                int y = baseY + (DIGIT_H - 1 - r);
                batch.push_back(glm::vec2(x, y));
                count_in_batch++;
                if (count_in_batch == 4){
                    draw_cells(batch, col, 0.6f);
                    batch.clear();
                    count_in_batch = 0;
                }
            }
        }
    }
    if (!batch.empty()) draw_cells(batch, col, 0.6f);
}

void draw_score_hud()
{
    // 基础位置在棋盘右侧
    int hudX = board_width + 1;      // 从右侧空白开始
    int hudY = board_height - DIGIT_H - 1; // 顶部留1行空白
    // 绘制分数数字（最多显示到4-5位）
    std::string s = std::to_string(score);
    // 前缀：小字样"S"和"C"等较复杂，先直接画数字
    glm::vec4 numColor = white;
    int x = hudX;
    for (char ch : s){
        int d = ch - '0';
        draw_digit(d, x, hudY, numColor);
        x += DIGIT_W + 1; // 数字之间1格间隔
    }
    // HUD背景条（简单：用几块方格填充作为面板装饰）
    std::vector<glm::vec2> panel;
    for (int yy = 0; yy < DIGIT_H + 2; ++yy){
        panel.push_back(glm::vec2(hudX - 1, hudY + yy));
        if (panel.size() == 4){ draw_cells(panel, glm::vec4(0.2,0.2,0.2,1.0), 0.55f); panel.clear(); }
    }
    if (!panel.empty()) draw_cells(panel, glm::vec4(0.2,0.2,0.2,1.0), 0.55f);
}

void draw_ghost()
{
    // 计算从当前tilepos向下的落点
    glm::vec2 ghostPos = tilepos;
    auto canDown = [&](){
        return checkvalid(tile[0] + ghostPos + glm::vec2(0,-1)) &&
               checkvalid(tile[1] + ghostPos + glm::vec2(0,-1)) &&
               checkvalid(tile[2] + ghostPos + glm::vec2(0,-1)) &&
               checkvalid(tile[3] + ghostPos + glm::vec2(0,-1));
    };
    while (canDown()) ghostPos.y -= 1;
    // 如果与当前重合则也画，作为落点提示
    std::vector<glm::vec2> cells = {
        tile[0] + ghostPos,
        tile[1] + ghostPos,
        tile[2] + ghostPos,
        tile[3] + ghostPos
    };
    glm::vec4 ghostColor = glm::vec4(current_colour.r*0.5f, current_colour.g*0.5f, current_colour.b*0.5f, 1.0f);
    draw_cells(cells, ghostColor, 0.35f);
}

// 键盘响应事件
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (!(action == GLFW_PRESS || action == GLFW_REPEAT)) return;

    if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q){
        exit(EXIT_SUCCESS);
        return;
    }
    if (key == GLFW_KEY_R){
        restart();
        return;
    }

    if(!gameover)
    {
        switch(key)
        {
            case GLFW_KEY_UP:
                rotate();
                break;
            case GLFW_KEY_DOWN:
                if (!movetile(glm::vec2(0, -1))) {
                    settile();
                    checkfullrow(-1); // 立即消除满行
                    newtile();
                }
                break;
            case GLFW_KEY_LEFT:
                movetile(glm::vec2(-1, 0));
                break;
            case GLFW_KEY_RIGHT:
                movetile(glm::vec2(1, 0));
                break;
            case GLFW_KEY_SPACE:
                // 空格：硬降到底部
                while(movetile(glm::vec2(0, -1))){}
                settile();
                checkfullrow(-1);
                newtile();
                break;
            case GLFW_KEY_P:
                // P：暂停/恢复
                paused = !paused;
                lastFallTime = glfwGetTime();
                update_ui();
                break;
            default:
                break;
        }
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
	GLFWwindow* window = glfwCreateWindow(500, 900, "李文俊-2023150001-期中大作业", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window!" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    srand(static_cast<unsigned>(time(NULL)));

    init();
    lastFallTime = glfwGetTime();
    while (!glfwWindowShouldClose(window))
    { 
        display();
        glfwSwapBuffers(window);
        glfwPollEvents(); 
    }
    glfwTerminate();
    return 0;
}
