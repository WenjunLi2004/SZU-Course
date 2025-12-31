<?php
include("conn.php");
include("header.php");

// 权限检查
if(!isset($_SESSION['user']) || strtoupper(trim($_SESSION['role'])) != 'ADMIN') {
    die("<div class='alert alert-danger m-4'>只有管理员可以访问此页面 <a href='index.php'>返回</a></div>");
}

$aid = isset($_GET['aid']) ? $_GET['aid'] : 0;

// 查询当前活动信息
$sql = "SELECT * FROM Activity WHERE activity_id = $aid";
$result = $conn->query($sql);
if(!$result || $result->num_rows == 0) {
    die("<div class='alert alert-danger m-4'>活动不存在 <a href='index.php'>返回</a></div>");
}
$row = $result->fetch_assoc();

// 处理 datetime-local 格式 (HTML5 input 需要 'YYYY-MM-DDTHH:MM' 格式)
$start_time_fmt = date('Y-m-d\TH:i', strtotime($row['start_time']));
$end_time_fmt = date('Y-m-d\TH:i', strtotime($row['end_time']));
?>

<div class="row justify-content-center">
    <div class="col-md-8">
        <div class="card shadow">
            <div class="card-header bg-primary text-white">
                <h4 class="mb-0">✏️ 修改活动信息</h4>
            </div>
            <div class="card-body">
                <form action="action.php" method="post">
                    <input type="hidden" name="act" value="update_activity">
                    <input type="hidden" name="aid" value="<?php echo $row['activity_id']; ?>">
                    
                    <div class="mb-3">
                        <label class="form-label">活动标题</label>
                        <input type="text" name="title" class="form-control" value="<?php echo $row['title']; ?>" required>
                    </div>
                    
                    <div class="mb-3">
                        <label class="form-label">活动描述</label>
                        <textarea name="desc" class="form-control" rows="3"><?php echo $row['description']; ?></textarea>
                    </div>
                    
                    <div class="row">
                        <div class="col-md-6 mb-3">
                            <label class="form-label">地点</label>
                            <input type="text" name="location" class="form-control" value="<?php echo $row['location']; ?>">
                        </div>
                        <div class="col-md-6 mb-3">
                            <label class="form-label">人数上限</label>
                            <input type="number" name="capacity" class="form-control" value="<?php echo $row['capacity']; ?>">
                        </div>
                    </div>
                    
                    <div class="row">
                        <div class="col-md-6 mb-3">
                            <label class="form-label">开始时间</label>
                            <input type="datetime-local" name="start" class="form-control" value="<?php echo $start_time_fmt; ?>">
                        </div>
                        <div class="col-md-6 mb-3">
                            <label class="form-label">结束时间 (报名截止)</label>
                            <input type="datetime-local" name="end" class="form-control" value="<?php echo $end_time_fmt; ?>">
                        </div>
                    </div>
                    
                    <div class="mb-3">
                        <label class="form-label">活动状态</label>
                        <select name="status_id" class="form-select">
                            <option value="1" <?php if($row['activity_status_id'] == 1) echo 'selected'; ?>>OPEN (开启)</option>
                            <option value="2" <?php if($row['activity_status_id'] == 2) echo 'selected'; ?>>CLOSED (关闭)</option>
                        </select>
                        <div class="form-text">如果手动改为关闭，用户将无法报名。</div>
                    </div>

                    <div class="d-grid gap-2 d-md-flex justify-content-md-end">
                        <a href="index.php" class="btn btn-secondary me-md-2">取消</a>
                        <button type="submit" class="btn btn-primary">💾 保存修改</button>
                    </div>
                </form>
            </div>
        </div>
    </div>
</div>

</body>
</html>