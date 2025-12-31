<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="utf-8" />
<title>修改新闻</title>
<style>
form { padding: 0; margin: 0; }
</style>
</head>
<body>

<?php
include("conn.php");
mysqli_set_charset($conn, "utf8mb4");

// 安全获取 ID
$id = isset($_GET['id']) ? intval($_GET['id']) : 0;

$query = "SELECT * FROM news WHERE id=$id";
$res = mysqli_query($conn, $query) or die(mysqli_error($conn));
$dbrow = mysqli_fetch_array($res);

if (!$dbrow) {
    die("未找到该新闻记录");
}

$title = htmlspecialchars($dbrow['title'], ENT_QUOTES);
$content = htmlspecialchars($dbrow['content'], ENT_QUOTES);
?>

<table width="70%" border="0" align="center" cellpadding="0" cellspacing="0">
<tr><td align="center">请修改新闻信息</td></tr>
</table>

<form action="save_edit_news.php" method="post">
<table width="70%" border="0" align="center" cellpadding="0" cellspacing="0">
<tr>
<td width="30%" align="right">新闻标题</td>
<td width="70%" align="left"><input type="text" name="title" size="30" value="<?php echo $title; ?>" /></td>
</tr>
<tr>
<td align="right">新闻内容</td>
<td align="left"><textarea name="content" cols="30" rows="5"><?php echo $content; ?></textarea></td>
</tr>
</table>

<table width="70%" align="center">
<tr><td align="center">
<input type="hidden" name="id" value="<?php echo $id; ?>" />
<input type="submit" value="确定修改" />
</td></tr>
</table>
</form>
</body>
</html>
