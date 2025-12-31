<?php
include("conn.php");
include("header.php");

if(!isset($_SESSION['user_id'])) {
    header("Location: login.php");
    exit;
}

$user_id = $_SESSION['user_id'];
$msg = "";
$msg_type = "success";

// 处理表单提交
if($_SERVER["REQUEST_METHOD"] == "POST") {
    $phone = isset($_POST['phone']) ? trim($_POST['phone']) : '';
    $email = isset($_POST['email']) ? trim($_POST['email']) : '';
    $new_password = isset($_POST['password']) ? $_POST['password'] : '';
    
    // 构建更新 SQL
    $sql_update = "UPDATE users SET phone=?, email=? WHERE user_id=?";
    $stmt = $conn->prepare($sql_update);
    $stmt->bind_param("ssi", $phone, $email, $user_id);
    
    if($stmt->execute()) {
        $msg = "个人信息更新成功！";
        
        // 如果有修改密码
        if(!empty($new_password)) {
            $sql_pwd = "UPDATE users SET password=? WHERE user_id=?";
            $stmt_pwd = $conn->prepare($sql_pwd);
            $stmt_pwd->bind_param("si", $new_password, $user_id);
            if($stmt_pwd->execute()) {
                $msg .= " 密码已修改。";
            } else {
                $msg .= " 但密码修改失败。";
                $msg_type = "warning";
            }
        }
    } else {
        $msg = "更新失败：" . $conn->error;
        $msg_type = "danger";
    }
}

// 获取当前用户信息
$sql = "SELECT username, phone, email FROM users WHERE user_id = ?";
$stmt = $conn->prepare($sql);
$stmt->bind_param("i", $user_id);
$stmt->execute();
$result = $stmt->get_result();
$user = $result->fetch_assoc();

?>

<div class="row justify-content-center">
    <div class="col-md-8">
        <div class="card shadow-sm">
            <div class="card-header bg-primary text-white">
                <h4 class="mb-0">👤 个人中心</h4>
            </div>
            <div class="card-body p-4">
                
                <?php if($msg): ?>
                    <div class="alert alert-<?php echo $msg_type; ?> alert-dismissible fade show">
                        <?php echo $msg; ?>
                        <button type="button" class="btn-close" data-bs-dismiss="alert"></button>
                    </div>
                <?php endif; ?>

                <form method="post">
                    <div class="mb-3 row">
                        <label class="col-sm-3 col-form-label text-muted">用户名</label>
                        <div class="col-sm-9">
                            <input type="text" readonly class="form-control-plaintext fw-bold" value="<?php echo htmlspecialchars($user['username']); ?>">
                        </div>
                    </div>
                    
                    <div class="mb-3 row">
                        <label class="col-sm-3 col-form-label">联系电话</label>
                        <div class="col-sm-9">
                            <input type="text" name="phone" class="form-control" value="<?php echo htmlspecialchars($user['phone'] ?? ''); ?>">
                        </div>
                    </div>
                    
                    <div class="mb-3 row">
                        <label class="col-sm-3 col-form-label">电子邮箱</label>
                        <div class="col-sm-9">
                            <input type="email" name="email" class="form-control" value="<?php echo htmlspecialchars($user['email'] ?? ''); ?>">
                        </div>
                    </div>
                    
                    <hr class="my-4">
                    
                    <div class="mb-3 row">
                        <label class="col-sm-3 col-form-label text-danger">修改密码</label>
                        <div class="col-sm-9">
                            <input type="password" name="password" class="form-control" placeholder="如果不修改密码，请留空">
                            <div class="form-text">密码修改后下次登录生效</div>
                        </div>
                    </div>
                    
                    <div class="d-grid gap-2 d-md-flex justify-content-md-end mt-4">
                        <a href="index.php" class="btn btn-secondary me-md-2">返回首页</a>
                        <button type="submit" class="btn btn-primary px-4">💾 保存修改</button>
                    </div>
                </form>
            </div>
        </div>
    </div>
</div>

</body>
</html>