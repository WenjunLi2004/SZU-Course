<?php
include("conn.php");

// 设置字符集（重要）
mysqli_set_charset($conn, "utf8mb4");

// 显示信息
echo "下面为查询 id=1 的新闻数据.<br>";

// 查询语句
$query = "SELECT * FROM news WHERE id = 1";

$res = mysqli_query($conn, $query);
if (!$res) {
    die("查询失败：" . mysqli_error($conn));
}

if (mysqli_num_rows($res) > 0) {
    $dbrow = mysqli_fetch_assoc($res);

    // 提取字段
    $id = $dbrow['id'];
    $title = $dbrow['title'];
    $content = $dbrow['content'];
    $add_time = $dbrow['add_time'];

    // 替换换行符
    $content = nl2br(htmlspecialchars($content, ENT_QUOTES, 'UTF-8'));

    echo "<strong>ID：</strong>{$id}<br>";
    echo "<strong>标题：</strong>" . htmlspecialchars($title, ENT_QUOTES, 'UTF-8') . "<br>";
    echo "<strong>时间：</strong>{$add_time}<br><br>";
    echo "<strong>内容：</strong><br>{$content}";
} else {
    echo "无相关数据";
}

// 关闭连接
mysqli_close($conn);
?>
