<?php
include("conn.php");

// 设置编码
mysqli_set_charset($conn, "utf8mb4");

echo "下面为查询标题中包含“李文俊”的新闻数据：<br>";

$sql = "SELECT * FROM news WHERE title LIKE '%李文俊%'";
$res = mysqli_query($conn, $sql);

if (mysqli_num_rows($res) > 0) {
    while ($row = mysqli_fetch_assoc($res)) {
        $id = $row['id'];
        $title = htmlspecialchars($row['title']);
        $content = nl2br(htmlspecialchars($row['content']));
        $add_time = $row['add_time'];

        echo "<p><strong>ID：</strong>$id<br>";
        echo "<strong>标题：</strong>$title<br>";
        echo "<strong>时间：</strong>$add_time<br>";
        echo "<strong>内容：</strong>$content</p>";
        echo "<hr>";
    }
} else {
    echo "无相关数据";
}
?>
