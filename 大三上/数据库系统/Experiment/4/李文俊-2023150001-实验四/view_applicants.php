<?php
include("conn.php");
include("header.php");

// 权限检查
if(!isset($_SESSION['user']) || strtoupper(trim($_SESSION['role'])) != 'ADMIN') {
    die("<div class='alert alert-danger m-4'>只有管理员可以访问此页面 <a href='index.php'>返回</a></div>");
}

$aid = isset($_GET['aid']) ? intval($_GET['aid']) : 0;

// 1. 获取活动信息
$sql_act = "SELECT * FROM Activity WHERE activity_id = $aid";
$res_act = $conn->query($sql_act);
if(!$res_act || $res_act->num_rows == 0) die("活动不存在");
$activity = $res_act->fetch_assoc();

// 2. 获取该活动的申请名单 (关联 Users 和 状态表)
// 按照时间倒序排列
$sql_apps = "SELECT app.application_id, app.apply_time, 
                    u.username, u.email, u.phone,
                    s.status_name, s.status_desc
             FROM Application app
             JOIN users u ON app.user_id = u.user_id
             JOIN ApplicationStatus s ON app.application_status_id = s.application_status_id
             WHERE app.activity_id = $aid
             ORDER BY app.apply_time DESC";
$res_apps = $conn->query($sql_apps);
?>

<div class="row mb-3">
    <div class="col-md-8">
        <h3 class="fw-bold">📋 申请名单管理</h3>
        <p class="text-muted">
            活动：<span class="text-primary fw-bold"><?php echo $activity['title']; ?></span> 
            <span class="mx-2">|</span>
            总申请数：<?php echo $res_apps->num_rows; ?>
        </p>
    </div>
    <div class="col-md-4 text-end">
        <a href="index.php" class="btn btn-secondary">⬅ 返回活动大厅</a>
    </div>
</div>

<?php if(isset($_GET['msg'])): ?>
    <div class="alert alert-success alert-dismissible fade show">
        <?php echo htmlspecialchars($_GET['msg']); ?>
        <button type="button" class="btn-close" data-bs-dismiss="alert"></button>
    </div>
<?php endif; ?>
<?php if(isset($_GET['error'])): ?>
    <div class="alert alert-danger alert-dismissible fade show">
        <?php echo htmlspecialchars($_GET['error']); ?>
        <button type="button" class="btn-close" data-bs-dismiss="alert"></button>
    </div>
<?php endif; ?>

<div class="card shadow">
    <div class="card-body">
        <?php if($res_apps->num_rows == 0): ?>
            <div class="text-center py-5 text-muted">
                <h4>暂无申请记录</h4>
                <p>还没有用户报名参加这个活动。</p>
            </div>
        <?php else: ?>
            <div class="table-responsive">
                <table class="table table-hover align-middle">
                    <thead class="table-light">
                        <tr>
                            <th>申请人</th>
                            <th>联系方式</th>
                            <th>申请时间</th>
                            <th>当前状态</th>
                            <th>审核操作</th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php while($row = $res_apps->fetch_assoc()): ?>
                        <?php 
                            // 状态样式处理
                            $status_color = 'bg-secondary';
                            $status_val = strtoupper($row['status_name']);
                            if(strpos($status_val, 'APPROVED') !== false) $status_color = 'bg-success';
                            if(strpos($status_val, 'REJECTED') !== false) $status_color = 'bg-danger';
                            if(strpos($status_val, 'PENDING') !== false) $status_color = 'bg-warning text-dark';
                        ?>
                        <tr>
                            <td>
                                <div class="d-flex align-items-center">
                                    <div class="bg-primary text-white rounded-circle d-flex align-items-center justify-content-center me-2" style="width:35px;height:35px;">
                                        <?php echo mb_substr($row['username'], 0, 1); ?>
                                    </div>
                                    <div>
                                        <div class="fw-bold"><?php echo $row['username']; ?></div>
                                    </div>
                                </div>
                            </td>
                            <td class="small">
                                <div>📞 <?php echo $row['phone'] ? $row['phone'] : '未填'; ?></div>
                                <div>📧 <?php echo $row['email'] ? $row['email'] : '未填'; ?></div>
                            </td>
                            <td><?php echo date('Y-m-d H:i', strtotime($row['apply_time'])); ?></td>
                            <td>
                                <span class="badge <?php echo $status_color; ?>"><?php echo $row['status_desc']; ?></span>
                            </td>
                            <td>
                                <div class="btn-group" role="group">
                                    <a href="action.php?act=audit&decision=2&appid=<?php echo $row['application_id']; ?>&from=list&aid=<?php echo $aid; ?>" 
                                       class="btn btn-sm btn-outline-success <?php echo ($status_val=='APPROVED')?'active':''; ?>">
                                       通过
                                    </a>
                                    <a href="action.php?act=audit&decision=3&appid=<?php echo $row['application_id']; ?>&from=list&aid=<?php echo $aid; ?>" 
                                       class="btn btn-sm btn-outline-danger <?php echo ($status_val=='REJECTED')?'active':''; ?>">
                                       拒绝
                                    </a>
                                </div>
                            </td>
                        </tr>
                        <?php endwhile; ?>
                    </tbody>
                </table>
            </div>
        <?php endif; ?>
    </div>
</div>

</body>
</html>