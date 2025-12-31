<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<title>添加新闻输入页面</title>
<style>
body {
    font-family: Arial, sans-serif;
}
form {
    width: 60%;
    margin: 30px auto;
}
.form-group {
    margin-bottom: 15px;
}
label {
    display: block;
    margin-bottom: 5px;
}
input[type="text"], textarea {
    width: 100%;
    padding: 8px;
    box-sizing: border-box;
}
button {
    padding: 8px 16px;
}
</style>
</head>
<body>
<table width="70%" height="30" border="0" align="center" cellpadding="0" cellspacing="0">
<tr>
<td align="center">请填写要添加新闻的信息</td>
</tr>
</table>
<form action="save_news.php" method="post">
<!--这里是一个表单,意思是以post方式把下面输入的数据传到save_news.php页面. ,表单以</form>结束-->
<table width="70%" border="0" align="center" cellpadding="0" cellspacing="0">
<tr>
<td width="30%" align="right">新闻标题</td>
<td width="70%" align="left"><input type="text" name="title" size="30"/></td>
</tr>
<tr>
<td width="30%" align="right">新闻话题</td>
<td width="70%" align="left"><input type="text" name="topic" size="30"/></td>
</tr>
<tr>
<td align="right">新闻内容</td>
<td align="left"><textarea name="content" cols="30" rows="5"></textarea></td>
</tr>
</table>
<table width="70%" height="30" border="0" align="center" cellpadding="0" cellspacing="0">
<tr>
<td align="center"><input type="submit" name="submit1" value="确定添加"/></td>
</tr>
</table>
</form>
</body>
</html>
