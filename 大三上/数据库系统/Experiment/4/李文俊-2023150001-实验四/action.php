<?php
include("conn.php");
if(!isset($_SESSION['user'])) die("未登录");

$act = isset($_REQUEST['act']) ? $_REQUEST['act'] : '';
$current_role = isset($_SESSION['role']) ? strtoupper(trim($_SESSION['role'])) : '';

// 1. 用户报名
if($act == 'apply') {
    $aid = $_GET['aid'];
    $uid = $_SESSION['user_id'];
    
    // 查重
    $check = $conn->query("SELECT * FROM Application WHERE user_id=$uid AND activity_id=$aid");
    if($check->num_rows > 0) {
        header("Location: index.php?error=您已报名过此活动");
        exit;
    }

    $sql = "INSERT INTO Application (user_id, activity_id, application_status_id, apply_time) VALUES ($uid, $aid, 1, NOW())";
    try {
        if($conn->query($sql)) {
            header("Location: index.php?msg=报名成功");
        } else {
            // 如果未抛出异常但返回 false
            $error_msg = $conn->error;
            header("Location: index.php?error=" . urlencode($error_msg));
        }
    } catch (mysqli_sql_exception $e) {
        // 捕获触发器抛出的 "报名失败：该活动名额已满！" 
        $error_msg = $e->getMessage();
        header("Location: index.php?error=" . urlencode($error_msg));
    }
}

// 2. 添加活动
if($act == 'add_activity' && $current_role == 'ADMIN') {
    $title = $_POST['title'];
    $desc = $_POST['desc'];
    $loc = $_POST['location'];
    $start = $_POST['start'];
    $end = $_POST['end'];
    $cap = $_POST['capacity'];
    
    $sql = "INSERT INTO Activity (activity_status_id, title, description, location, start_time, end_time, capacity, created_at) VALUES (1, '$title', '$desc', '$loc', '$start', '$end', $cap, NOW())";
    try {
        if($conn->query($sql)) {
            $new_aid = $conn->insert_id; // 获取刚插入的ID 
            $conn->query("CALL sp_log_action(" . $_SESSION['user_id'] . ", 'CREATE', 'ACTIVITY', $new_aid)");
            header("Location: index.php?msg=发布成功");
        } else {
            header("Location: index.php?error=发布失败: " . urlencode($conn->error));
        }
    } catch (mysqli_sql_exception $e) {
        header("Location: index.php?error=发布异常: " . urlencode($e->getMessage()));
    }
}

// 3. 修改活动
if($act == 'update_activity' && $current_role == 'ADMIN') {
    $aid = $_POST['aid'];
    $title = $_POST['title'];
    $desc = $_POST['desc'];
    $loc = $_POST['location'];
    $start = $_POST['start'];
    $end = $_POST['end'];
    $cap = $_POST['capacity'];
    $status_id = $_POST['status_id'];

    $sql = "UPDATE Activity SET title='$title', description='$desc', location='$loc', start_time='$start', end_time='$end', capacity='$cap', activity_status_id='$status_id' WHERE activity_id=$aid";
    try {
        if($conn->query($sql)) {
            header("Location: index.php?msg=修改成功");
        } else {
            header("Location: index.php?error=修改失败: " . urlencode($conn->error));
        }
    } catch (mysqli_sql_exception $e) {
        header("Location: index.php?error=修改异常: " . urlencode($e->getMessage()));
    }
}

// 4. 审核 (核心修改)
if($act == 'audit' && $current_role == 'ADMIN') {
    $appid = $_GET['appid'];
    $decision = $_GET['decision'];
    $admin_id = $_SESSION['user_id'];
    
    // 检查是否有 from 参数
    $from = isset($_GET['from']) ? $_GET['from'] : '';
    $aid = isset($_GET['aid']) ? $_GET['aid'] : '';

    $conn->begin_transaction();
    try {
        // 调用存储过程执行审核
        $sql = "CALL sp_audit_application($appid, $decision, $admin_id)";
        $res = $conn->query($sql);
        
        if(!$res) {
            throw new Exception($conn->error);
        }
        
        $conn->commit();
        
        // 如果是从名单页来的，跳回名单页
        if($from == 'list' && $aid) {
            header("Location: view_applicants.php?aid=$aid&msg=审核操作成功");
        } else {
            // 否则跳回待办审核页
            header("Location: admin_audit.php?msg=审核完成");
        }
    } catch (Exception $e) {
        $conn->rollback();
        // 捕获错误并弹窗提示
        $err = $e->getMessage();
        // 跳回来的页面，带着错误信息
        if($from == 'list' && $aid) {
             header("Location: view_applicants.php?aid=$aid&error=" . urlencode($err));
        } else {
             header("Location: admin_audit.php?error=" . urlencode($err));
        }
    }
}

// 5. 删除活动
if($act == 'delete_activity' && $current_role == 'ADMIN') {
    $aid = $_GET['aid'];
    
    try {
        // 尝试删除 
        // 如果触发器拦截，可能会抛出 mysqli_sql_exception
        if($conn->query("DELETE FROM Activity WHERE activity_id = $aid")) { 
            header("Location: index.php?msg=活动已删除"); 
        } else {
             // 如果未抛出异常但返回 false (取决于配置)
            $error_msg = $conn->error; 
            header("Location: index.php?error=" . urlencode($error_msg)); 
        }
    } catch (mysqli_sql_exception $e) {
        // 捕获触发器抛出的 "删除失败：该活动已有成员通过..." 
        $error_msg = $e->getMessage();
        header("Location: index.php?error=" . urlencode($error_msg));
    }
}

// 6. 用户撤销报名 (调用存储过程) 
if($act == 'withdraw') { 
    $aid = $_GET['aid']; 
    $uid = $_SESSION['user_id']; 
    
    // 调用存储过程 
    // 注意：我们需要获取 OUT 参数 p_result 
    // 使用 multi_query 来处理 OUT 参数
    $sql = "CALL sp_withdraw_application($uid, $aid, @res); SELECT @res as msg;"; 
    
    try {
        if($conn->multi_query($sql)) { 
            $msg = '';
            do {
                if ($res = $conn->store_result()) {
                    while ($row = $res->fetch_assoc()) {
                        if(isset($row['msg'])) $msg = $row['msg'];
                    }
                    $res->free();
                }
            } while ($conn->more_results() && $conn->next_result());
            
            if($msg == 'SUCCESS') { 
                header("Location: my_applications.php?msg=撤销成功！"); 
            } elseif($msg == 'NOT_FOUND') { 
                header("Location: my_applications.php?error=未找到申请记录"); 
            } elseif($msg == 'CANNOT_WITHDRAW') { 
                header("Location: my_applications.php?error=当前状态不允许撤销"); 
            } else { 
                header("Location: my_applications.php?error=操作失败：$msg"); 
            } 
        } else { 
            header("Location: my_applications.php?error=数据库错误：" . $conn->error); 
        } 
    } catch (mysqli_sql_exception $e) {
        header("Location: my_applications.php?error=系统错误：" . urlencode($e->getMessage()));
    }
}
?>