<?php
// conn.php
$servername = "localhost";
$username = "root";     // WAMP默认用户名
$password = "";         // WAMP默认密码为空
$dbname = "use";        // 你的数据库名

$conn = new mysqli($servername, $username, $password, $dbname);
if ($conn->connect_error) {
    die("数据库连接失败: " . $conn->connect_error);
}
$conn->set_charset("utf8mb4");

// 开启会话
if(!session_id()) session_start();

// =========== 新增代码开始 =========== 
// 每次页面加载连接数据库时，都顺手清理一下过期活动 
// 这样用户就不需要重新登录，只要刷新页面，过期活动就会自动关闭 
// 注意：调用存储过程后需要处理所有结果集，否则会导致Commands out of sync错误
if ($conn->multi_query("CALL sp_close_expired_activities()")) {
    do {
        // 消费所有结果集
        if ($result = $conn->store_result()) {
            $result->free();
        }
    } while ($conn->more_results() && $conn->next_result());
}
// =========== 新增代码结束 =========== 

// 记录操作日志的通用函数 
function write_audit_log($conn, $user_id, $action_type, $target_type, $target_id) { 
    // 改为调用存储过程 
    $sql = "CALL sp_log_action($user_id, '$action_type', '$target_type', $target_id)"; 
    $conn->query($sql); 
}
?>