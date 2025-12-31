<?php
include ("conn.php");
// mysql_query("set names gb2312");
// error_reporting(0); 
// 设置字符集和错误报告
mysqli_set_charset($conn, "utf8");
error_reporting(E_ALL);
ini_set('display_errors', 1);

//下面先接收从add_news.php传过来的新闻标题与新闻内容.
//PHP变量是以$开头的,如$a,$b 变量,与C,C++一样都是以";"分号结果一句子;注释也与C,C++一样.
// 因为add_news.php表单定义的传输方式为POST所以这里要对应用POST接收,如果定义为GET则要用GET接收.
// 验证POST数据是否存在
if (!isset($_POST['title']) || !isset($_POST['topic']) || !isset($_POST['content'])) {
   die("缺少必要的表单数据");
}

$title = mysqli_real_escape_string($conn, trim($_POST['title']));
$topic = mysqli_real_escape_string($conn, trim($_POST['topic']));
$content = mysqli_real_escape_string($conn, trim($_POST['content']));
// 验证数据不为空
if (empty($title) || empty($topic) || empty($content)) {
   die("标题、主题和内容不能为空");
}
//下面用一if语句检测系统的香港时区的时间,我们用的PHP一般以香港时间为准的,
if(function_exists('date_default_timezone_set')) { 
   date_default_timezone_set('Hongkong');//该函数为PHP5.1内置. 
} 
$add_time=date("Y-m-d");
   //这句话把获取到的系统当前时间赋给变量$add_time
   
$sql = "INSERT INTO news (title,topic,content,add_time)

VALUES ('$title','$topic','$content','$add_time')";

// 执行查询
$result = mysqli_query($conn, $sql);  //如果添加成功,返回真给$result ,否则为false.

if($result)
{
echo "添加新闻成功,<a href='add_news.php'>返回继续</a>";
}
else
{
echo "添加新闻失败,<a href='add_news.php'>请返回</a>";
}
// 关闭连接
mysqli_close($conn);
?>