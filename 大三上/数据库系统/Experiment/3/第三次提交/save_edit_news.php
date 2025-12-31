<?php
// save_edit_news.php - 使用 mysqli 的安全更新示例
header('Content-Type: text/html; charset=utf-8');
include 'conn.php';

// 只接受 POST 请求
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    echo "<script>alert('非法请求'); window.location.href='news.php';</script>";
    exit;
}

// 获取并验证参数
$id = isset($_POST['id']) ? intval($_POST['id']) : 0;
$title = isset($_POST['title']) ? trim($_POST['title']) : '';
$content = isset($_POST['content']) ? trim($_POST['content']) : '';

if ($id <= 0) {
    echo "<script>alert('参数错误：ID 不合法'); history.back();</script>";
    exit;
}
if ($title === '' || $content === '') {
    echo "<script>alert('标题或内容不能为空'); history.back();</script>";
    exit;
}

// 使用预处理语句更新数据，防止 SQL 注入
$stmt = mysqli_prepare($conn, "UPDATE news SET title = ?, content = ? WHERE id = ?");
if (!$stmt) {
    // 准备语句失败
    die('数据库错误（prepare）: ' . mysqli_error($conn));
}

mysqli_stmt_bind_param($stmt, 'ssi', $title, $content, $id);
$ok = mysqli_stmt_execute($stmt);

if ($ok) {
    // 更新成功，跳回列表或显示成功
    echo "<script>alert('修改成功'); window.location.href='read_2.php';</script>";
} else {
    // 更新失败，显示错误（生产环境请不要泄露详细错误）
    $err = mysqli_error($conn);
    echo "<script>alert('修改失败: " . htmlspecialchars($err, ENT_QUOTES) . "'); history.back();</script>";
}

mysqli_stmt_close($stmt);
mysqli_close($conn);
