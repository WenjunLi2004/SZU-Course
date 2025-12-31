<?php
include("conn.php");
// 开启错误显示
error_reporting(E_ALL);
ini_set('display_errors', 1);

include("header.php");

// 权限校验
if(!isset($_SESSION['user']) || strtoupper(trim($_SESSION['role'])) != 'ADMIN') {
    die("<div class='container mt-5 alert alert-danger'>权限不足，请以管理员身份登录。</div>");
}

// ---------------------------------------------
// 数据查询 1：待审核列表 (状态 = 1 / PENDING)
// ---------------------------------------------
$sql_pending = "SELECT app.application_id, u.username, act.title, app.apply_time
                FROM Application app
                JOIN users u ON app.user_id = u.user_id
                JOIN Activity act ON app.activity_id = act.activity_id
                WHERE app.application_status_id = 1 
                ORDER BY app.apply_time ASC";
$res_pending = $conn->query($sql_pending);

// ---------------------------------------------
// 数据查询 2：审核历史/存档 (状态 != 1)
// 关联 Review 表获取审核意见
// ---------------------------------------------
$sql_history = "SELECT app.application_id, u.username, act.title, app.apply_time, 
                       s.status_name, s.status_desc,
                       r.decision, r.comment, r.review_time, admin_u.username as admin_name
                FROM Application app
                JOIN users u ON app.user_id = u.user_id
                JOIN Activity act ON app.activity_id = act.activity_id
                JOIN ApplicationStatus s ON app.application_status_id = s.application_status_id
                LEFT JOIN Review r ON app.application_id = r.application_id
                LEFT JOIN users admin_u ON r.admin_id = admin_u.user_id
                WHERE app.application_status_id != 1 
                ORDER BY r.review_time DESC";
$res_history = $conn->query($sql_history);
?>

<div class="row mb-3">
    <div class="col-12">
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
    </div>
</div>

<div class="card shadow-sm">
    <div class="card-header bg-white p-0">
        <ul class="nav nav-tabs card-header-tabs m-0" id="auditTabs" role="tablist">
            <li class="nav-item">
                <button class="nav-link active fw-bold py-3 px-4" id="pending-tab" data-bs-toggle="tab" data-bs-target="#pending" type="button">
                    ⏳ 待审核申请 
                    <?php if($res_pending && $res_pending->num_rows > 0): ?>
                        <span class="badge bg-danger rounded-pill ms-2"><?php echo $res_pending->num_rows; ?></span>
                    <?php endif; ?>
                </button>
            </li>
            <li class="nav-item">
                <button class="nav-link fw-bold py-3 px-4" id="history-tab" data-bs-toggle="tab" data-bs-target="#history" type="button">
                    🗂️ 审核记录
                </button>
            </li>
        </ul>
    </div>

    <div class="card-body">
        <div class="tab-content" id="auditTabsContent">
            
            <div class="tab-pane fade show active" id="pending">
                <?php if(!$res_pending || $res_pending->num_rows == 0): ?>
                    <div class="text-center py-5 text-muted">
                        <h4>🎉 太棒了！</h4>
                        <p>当前没有待处理的申请。</p>
                    </div>
                <?php else: ?>
                    <div class="table-responsive">
                        <table class="table table-hover align-middle">
                            <thead class="table-light">
                                <tr>
                                    <th>ID</th>
                                    <th>申请人</th>
                                    <th>申请活动</th>
                                    <th>申请时间</th>
                                    <th>操作</th>
                                </tr>
                            </thead>
                            <tbody>
                                <?php while($row = $res_pending->fetch_assoc()): ?>
                                <tr>
                                    <td>#<?php echo $row['application_id']; ?></td>
                                    <td>
                                        <div class="d-flex align-items-center">
                                            <div class="bg-primary text-white rounded-circle d-flex align-items-center justify-content-center me-2" style="width:30px;height:30px;">
                                                <?php echo mb_substr($row['username'], 0, 1); ?>
                                            </div>
                                            <strong><?php echo $row['username']; ?></strong>
                                        </div>
                                    </td>
                                    <td><?php echo $row['title']; ?></td>
                                    <td><?php echo $row['apply_time']; ?></td>
                                    <td>
                                        <a href="action.php?act=audit&decision=2&appid=<?php echo $row['application_id']; ?>" 
                                           class="btn btn-sm btn-success px-3 me-2">✅ 通过</a>
                                        <a href="action.php?act=audit&decision=3&appid=<?php echo $row['application_id']; ?>" 
                                           class="btn btn-sm btn-outline-danger px-3"
                                           onclick="return confirm('确定要拒绝吗？')">❌ 拒绝</a>
                                    </td>
                                </tr>
                                <?php endwhile; ?>
                            </tbody>
                        </table>
                    </div>
                <?php endif; ?>
            </div>

            <div class="tab-pane fade" id="history">
                <div class="table-responsive">
                    <table class="table table-striped align-middle">
                        <thead class="table-light">
                            <tr>
                                <th>活动名称</th>
                                <th>申请人</th>
                                <th>申请结果</th>
                                <th>审核人</th>
                                <th>处理时间</th>
                            </tr>
                        </thead>
                        <tbody>
                            <?php if(!$res_history || $res_history->num_rows == 0): ?>
                                <tr><td colspan="5" class="text-center py-4">暂无历史记录</td></tr>
                            <?php else: ?>
                                <?php while($row = $res_history->fetch_assoc()): ?>
                                <tr>
                                    <td><?php echo $row['title']; ?></td>
                                    <td><?php echo $row['username']; ?></td>
                                    <td>
                                        <?php 
                                        $decision = isset($row['decision']) ? $row['decision'] : '';
                                        if(strpos($decision, 'APPROVED') !== false): 
                                        ?>
                                            <span class="badge bg-success">已通过</span>
                                        <?php else: ?>
                                            <span class="badge bg-danger">已拒绝</span>
                                        <?php endif; ?>
                                    </td>
                                    <td class="small text-muted">
                                        <?php echo $row['admin_name'] ? $row['admin_name'] : 'System'; ?>
                                    </td>
                                    <td class="small text-muted">
                                        <?php echo $row['review_time']; ?>
                                    </td>
                                </tr>
                                <?php endwhile; ?>
                            <?php endif; ?>
                        </tbody>
                    </table>
                </div>
            </div>

        </div>
    </div>
</div>

<script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js"></script>

</body>
</html>