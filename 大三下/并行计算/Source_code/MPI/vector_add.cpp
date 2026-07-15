#include <mpi.h>
#include <iostream>
#include <string>

using namespace std;

// 修改了 vec_name 的类型为 string
void Read_vector(double local_a[], int local_n, int n, const string &vec_name, int my_rank, MPI_Comm comm)
{
    double *a = nullptr;

    // 只有核心 0 负责准备完整数据
    if (my_rank == 0)
    {
        a = new double[n]; // 【修复1】正确的 C++ 数组分配语法

        // 为了避免手动输入 1000 个数，这里帮你写个简单的自动赋值
        // 如果想手动输入，可以把测试数据量 n 改成 8 左右
        for (int i = 0; i < n; i++)
        {
            a[i] = i * 1.5; // 模拟生成一些数据
        }
        cout << "Generated vector " << vec_name << " successfully." << endl;
    }

    // 【完美！】没有任何 if-else，所有人整齐划一地调用 Scatter 发牌/接牌
    MPI_Scatter(a, local_n, MPI_DOUBLE, local_a, local_n, MPI_DOUBLE, 0, comm);

    // 用完大数组 a 之后，核心 0 记得把它释放掉，避免内存泄漏
    if (my_rank == 0)
    {
        delete[] a;
    }
}

int main(int argc, char **argv)
{
    int comm_sz; /* 进程的数量     */
    int my_rank; /* 我的进程序列号 */

    MPI_Init(&argc, &argv); // 习惯性把参数传进去
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    const int n = 16;          // 测试时建议用小一点的数，并且最好能被核心数(如4)整除
    int local_n = n / comm_sz; // 【修复2】加上了 int 声明

    // 【优化】局部数组大小刚刚好就行，不需要 +1
    double *local_a = new double[local_n];
    double *local_b = new double[local_n];
    double *local_c = new double[local_n];

    // 【核心优化】结果数组 c 只有 0 号核心需要用到，其他核心给个 nullptr，极大地节约了内存！
    double *c = nullptr;
    if (my_rank == 0)
    {
        c = new double[n];
    }

    // 读取(或生成)两个向量
    Read_vector(local_a, local_n, n, "A", my_rank, MPI_COMM_WORLD);
    Read_vector(local_b, local_n, n, "B", my_rank, MPI_COMM_WORLD);

    // 每个核心各自闷头计算自己分到的那一块
    for (int i = 0; i < local_n; i++)
    {
        local_c[i] = local_a[i] + local_b[i];
    }

    // 【完美！】原样回收，拼装成完整的 c
    MPI_Gather(local_c, local_n, MPI_DOUBLE, c, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // 只有核心 0 拥有完整的 c 并负责打印
    if (my_rank == 0)
    {
        cout << "\nThe result vector C is:" << endl;
        for (int i = 0; i < n; i++)
        {
            cout << c[i] << " ";
        }
        cout << endl;
    }

    // 【修复3】好借好还，再借不难。释放所有动态申请的内存
    delete[] local_a;
    delete[] local_b;
    delete[] local_c;
    if (my_rank == 0)
    {
        delete[] c;
    }

    // 【修复4】打卡下班
    MPI_Finalize();
    return 0;
}