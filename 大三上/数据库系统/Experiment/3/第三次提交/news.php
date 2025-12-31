<?php
// news.php - 显示所有新闻，可修改或删除
header('Content-Type: text/html; charset=utf-8');
include("conn.php");

echo "<h3>下面是所有新闻数据：</h3>";

$query = "SELECT * FROM news ORDER BY id ASC";
$res = mysqli_query($conn, $query);

if (!$res) {
    die("查询失败：" . mysqli_error($conn));
}

$row = mysqli_num_rows($res);
if ($row > 0) {
    while ($dbrow = mysqli_fetch_assoc($res)) {
        $id = $dbrow['id'];
        $title = htmlspecialchars($dbrow['title'], ENT_QUOTES, 'UTF-8');
        $add_time = $dbrow['add_time'];

        echo $id . " —— ";
        echo "<a href='edit_news.php?id=$id'>$title</a> ";
        echo " | ";
        echo "<a href='del_news.php?id=$id' onclick=\"return confirm('确定要删除这条新闻吗？');\">
              <font color='red'>删除</font></a>";
        echo "<br>";
    }
} else {
    echo "无相关数据。";
}

mysqli_free_result($res);
mysqli_close($conn);
?>
