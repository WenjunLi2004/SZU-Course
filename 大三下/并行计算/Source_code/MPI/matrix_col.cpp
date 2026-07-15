#include "mpi.h"
#include <iostream>
#include <iomanip>

using namespace std;

int main(int argc, char **argv)
{
    int rank, size;
    const int n = 4; // 4x4 矩阵
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // 0号核心生成一个 4x4 的测试矩阵
    double *matrix = NULL;
    if (rank == 0)
    {
        matrix = new double[n * n];
        for (int i = 0; i < n * n; i++)
        {
            matrix[i] = i + 1.0; // 填入 1.0, 2.0, ..., 16.0
        }
        cout << "Rank 0 的原始矩阵:" << endl;
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
                cout << setw(4) << matrix[i * n + j] << " ";
            cout << endl;
        }
        cout << "--------------------" << endl;
    }

    // ==========================================
    // 🌟 第一步：定制“跳跃提取”的手术刀
    // ==========================================
    MPI_Datatype MPI_COLUMN;

    // 参数：抓 n 次，每次抓 1 个，每次跨越 n 个步长，基础类型是 MPI_DOUBLE
    MPI_Type_vector(n, 1, n, MPI_DOUBLE, &MPI_COLUMN);

    // 🌟 第二步：向系统注册这把手术刀
    MPI_Type_commit(&MPI_COLUMN);

    // ==========================================
    // 🚀 第三步：通信！(见证奇迹的时刻)
    // ==========================================
    if (rank == 0)
    {
        // 我们想发第 1 列 (索引为 1)。
        // 所以我们把指针指在 matrix[1] 作为起跑线，然后告诉 MPI 用定制刀法去切！
        MPI_Send(&matrix[1], 1, MPI_COLUMN, 1, 0, MPI_COMM_WORLD);
        cout << "Rank 0 已直接发送了第 1 列数据！" << endl;
    }
    else if (rank == 1)
    {
        // 1 号核心只需要准备一个能装下 n 个元素的普通一维数组来接盘
        double *recv_col = new double[n];

        // 注意：接收方不需要知道发件人是怎么跳跃提取的！
        // 接收方只管接连续的 n 个 MPI_DOUBLE 就行了！
        MPI_Recv(recv_col, n, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        cout << "Rank 1 成功接收！数据为: ";
        for (int i = 0; i < n; i++)
            cout << recv_col[i] << " ";
        cout << endl;

        delete[] recv_col;
    }

    // ==========================================
    // 🌟 第四步：销毁图纸
    // ==========================================
    MPI_Type_free(&MPI_COLUMN);
    if (rank == 0)
        delete[] matrix;

    MPI_Finalize();
    return 0;
}