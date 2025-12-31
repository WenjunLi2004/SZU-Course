<?php
include("conn.php");

// 退出登录逻辑
if(isset($_GET['logout'])) {
    session_destroy();
    header("Location: login.php");
    exit;
}

$msg = "";
$msg_type = "warning";

// === 处理表单提交 ===
if($_SERVER["REQUEST_METHOD"] == "POST") {
    // 修复未定义变量警告
    $action = isset($_POST['action']) ? $_POST['action'] : '';

    // 1. 处理登录
    if($action == 'login') {
        $u = $_POST['username'];
        $p = $_POST['password'];

        $sql = "SELECT u.user_id, u.username, r.role_name 
                FROM users u 
                JOIN UserRole ur ON u.user_id = ur.user_id 
                JOIN Role r ON ur.role_id = r.role_id 
                WHERE u.username='$u' AND u.password='$p'";
        
        $res = $conn->query($sql);
        if($res && $res->num_rows > 0) {
            $row = $res->fetch_assoc();
            $_SESSION['user_id'] = $row['user_id'];
            $_SESSION['user'] = $row['username'];
            $_SESSION['role'] = strtoupper($row['role_name']); // 强转大写
            
            // [删除] 下面这行可以删掉了，因为 conn.php 已经帮我们做了 
            // $conn->query("CALL sp_close_expired_activities()");
            
            // 写入日志 
            write_audit_log($conn, $row['user_id'], 'LOGIN', 'SYSTEM', 0); // 0表示系统级对象 

            header("Location: index.php");
            exit;
        } else {
            $msg = "账号或密码错误！";
            $msg_type = "danger";
        }
    } 
    
    // 2. 处理注册 (包含管理员密钥逻辑)
    elseif($action == 'register') {
        $u = $_POST['reg_username'];
        $p = $_POST['reg_password'];
        $phone = isset($_POST['reg_phone']) ? trim($_POST['reg_phone']) : '';
        $email = isset($_POST['reg_email']) ? trim($_POST['reg_email']) : '';
        $secret = isset($_POST['admin_secret']) ? trim($_POST['admin_secret']) : '';
        
        // --- 核心修改：密钥判断 ---
        // 假设管理员密钥是 "admin666"
        $target_role_id = 2; // 默认为 USER
        
        if($secret === "admin666") {
            $target_role_id = 1; // 1 为 ADMIN
        } elseif (!empty($secret)) {
            $msg = "注册失败：管理员密钥错误！";
            $msg_type = "danger";
        }
        // ------------------------

        if (empty($msg)) {
            // 使用存储过程注册
            // 构造 SQL
            $sql_call = "SET @msg = ''; CALL sp_register_user('$u', '$p', $target_role_id, @msg); SELECT @msg;";
            
            if ($conn->multi_query($sql_call)) {
                $result_msg = '';
                do {
                    if ($result = $conn->store_result()) {
                        while ($row = $result->fetch_assoc()) {
                            if (isset($row['@msg'])) {
                                $result_msg = $row['@msg'];
                            }
                        }
                        $result->free();
                    }
                } while ($conn->more_results() && $conn->next_result());
                
                if ($result_msg == 'SUCCESS') {
                    $msg = "注册成功！"; 
                    $msg_type = "success";
                    
                    // 如果注册成功，且有 phone/email，尝试更新
                    if ($phone || $email) {
                        $conn->query("UPDATE users SET phone='$phone', email='$email' WHERE username='$u'");
                    }
                } elseif ($result_msg == 'FAIL_DUPLICATE') {
                    $msg = "注册失败：用户名已存在"; 
                    $msg_type = "danger";
                } else {
                    $msg = "注册失败：" . $result_msg;
                    $msg_type = "danger";
                }
            } else {
                $msg = "数据库错误：" . $conn->error;
                $msg_type = "danger";
            }
        }
    }
}
?>

<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <title>登录 - 校园活动平台</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
    <style>
        body { 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); 
            height: 100vh; 
            display: flex; align-items: center; justify-content: center; 
        }
        .login-card { width: 400px; border-radius: 15px; overflow: hidden; box-shadow: 0 15px 35px rgba(0,0,0,0.2); }
        .card-header { background: #4e73df; color: white; text-align: center; padding: 25px; }
        .btn-register-trigger { text-decoration: none; color: #4e73df; font-weight: bold; cursor: pointer; }
    </style>
</head>
<body>

<div class="card login-card">
    <div class="card-header">
        <h3 class="mb-0">🚀 校园活动平台</h3>
    </div>
    <div class="card-body p-4 bg-white">
        <?php if($msg): ?>
            <div class="alert alert-<?php echo $msg_type; ?> alert-dismissible fade show">
                <?php echo $msg; ?>
                <button type="button" class="btn-close" data-bs-dismiss="alert"></button>
            </div>
        <?php endif; ?>
        
        <form method="post">
            <input type="hidden" name="action" value="login">
            <div class="mb-3">
                <label class="form-label text-muted">账号</label>
                <input type="text" name="username" class="form-control form-control-lg" required>
            </div>
            <div class="mb-4">
                <label class="form-label text-muted">密码</label>
                <input type="password" name="password" class="form-control form-control-lg" required>
            </div>
            <div class="d-grid gap-2">
                <button type="submit" class="btn btn-primary btn-lg">立即登录</button>
            </div>
        </form>
        
        <div class="text-center mt-4">
            <span class="btn-register-trigger" data-bs-toggle="modal" data-bs-target="#registerModal">
                注册新用户
            </span>
        </div>
    </div>
</div>

<div class="modal fade" id="registerModal" tabindex="-1" aria-hidden="true">
  <div class="modal-dialog modal-dialog-centered">
    <div class="modal-content">
      <div class="modal-header bg-success text-white">
        <h5 class="modal-title">📝 注册新账号</h5>
        <button type="button" class="btn-close btn-close-white" data-bs-dismiss="modal"></button>
      </div>
      <form method="post">
          <div class="modal-body p-4">
            <input type="hidden" name="action" value="register">
            <div class="mb-3">
                <label class="form-label">设置用户名</label>
                <input type="text" name="reg_username" class="form-control" required>
            </div>
            <div class="mb-3">
                <label class="form-label">设置密码</label>
                <input type="password" name="reg_password" class="form-control" required>
            </div>
            <div class="mb-3">
                <label class="form-label">联系电话</label>
                <input type="text" name="reg_phone" class="form-control" placeholder="选填">
            </div>
            <div class="mb-3">
                <label class="form-label">电子邮箱</label>
                <input type="email" name="reg_email" class="form-control" placeholder="选填">
            </div>
            
            <hr>
            <div class="mb-3">
                <label class="form-label text-danger">管理员密钥 (选填)</label>
                <input type="password" name="admin_secret" class="form-control border-danger" placeholder="普通用户请留空">
            </div>
          </div>
          <div class="modal-footer">
            <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">取消</button>
            <button type="submit" class="btn btn-success">确认注册</button>
          </div>
      </form>
    </div>
  </div>
</div>

<script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js"></script>

<?php if(isset($action) && $action == 'register' && $msg_type == 'success'): ?>
<script>
    if ( window.history.replaceState ) {
        window.history.replaceState( null, null, window.location.href );
    }
</script>
<?php endif; ?>

</body>
</html>