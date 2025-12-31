<?php
// del_news.php - 删除指定 id 的新闻
header('Content-Type: text/html; charset=utf-8');
include("conn.php");

// 获取 id 参数并验证
$id = isset($_GET['id']) ? intval($_GET['id']) : 0;
if ($id <= 0) {
    echo "<script>alert('参数错误：未指定有效ID'); history.back();</script>";
    exit;
}

// 执行删除
$stmt = mysqli_prepare($conn, "DELETE FROM news WHERE id = ?");
mysqli_stmt_bind_param($stmt, "i", $id);
$ok = mysqli_stmt_execute($stmt);

if ($ok) {
    echo "<script>alert('删除成功！'); window.location.href='news.php';</script>";
} else {
    echo "<script>alert('删除失败：" . mysqli_error($conn) . "'); history.back();</script>";
}

mysqli_stmt_close($stmt);
mysqli_close($conn);
?>
