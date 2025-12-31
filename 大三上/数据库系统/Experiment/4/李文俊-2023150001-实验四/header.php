<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>校园活动征招平台</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js"></script>
    <style>
        body { background-color: #f8f9fa; }
        .card { box-shadow: 0 4px 6px rgba(0,0,0,0.1); border: none; }
    </style>
</head>
<body>

<?php 
// 确保 session 开启
if(!session_id()) session_start();

if(isset($_SESSION['user'])): 
    // 获取清洗后的角色
    $current_role_header = isset($_SESSION['role']) ? strtoupper(trim($_SESSION['role'])) : '';
?>
<nav class="navbar navbar-expand-lg navbar-dark bg-primary shadow-sm mb-4">
  <div class="container">
    <a class="navbar-brand fw-bold" href="index.php">🎉 校园活动平台</a>
    <button class="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#navbarContent">
      <span class="navbar-toggler-icon"></span>
    </button>
    
    <div class="collapse navbar-collapse" id="navbarContent">
      <ul class="navbar-nav me-auto mb-2 mb-lg-0">
        <li class="nav-item"><a class="nav-link active" href="index.php">活动大厅</a></li>
        
        <?php if($current_role_header == 'USER'): ?>
            <li class="nav-item"><a class="nav-link" href="my_applications.php">📄 我的申请</a></li>
        <?php endif; ?>

        <?php if($current_role_header == 'ADMIN'): ?>
            <li class="nav-item"><a class="nav-link" href="admin_audit.php">⚖️ 待办审核</a></li>
            <li class="nav-item"><a class="nav-link" href="#" data-bs-toggle="modal" data-bs-target="#addActivityModal">➕ 发布活动</a></li>
        <?php endif; ?>
      </ul>
      <span class="navbar-text text-white me-3">
        欢迎, <b><?php echo $_SESSION['user']; ?></b> (<?php echo $current_role_header; ?>)
      </span>
      <a href="profile.php" class="btn btn-sm btn-info text-white me-2">👤 个人中心</a>
      <a href="login.php?logout=1" class="btn btn-sm btn-light text-primary fw-bold">退出</a>
    </div>
  </div>
</nav>

<?php if($current_role_header == 'ADMIN'): ?>
<div class="modal fade" id="addActivityModal" tabindex="-1">
  <div class="modal-dialog">
    <div class="modal-content">
      <form action="action.php" method="post">
          <div class="modal-header">
            <h5 class="modal-title">发布新活动</h5>
            <button type="button" class="btn-close" data-bs-dismiss="modal"></button>
          </div>
          <div class="modal-body">
            <input type="hidden" name="act" value="add_activity">
            <div class="mb-3"><label>活动标题</label><input type="text" name="title" class="form-control" required></div>
            <div class="mb-3"><label>描述</label><textarea name="desc" class="form-control"></textarea></div>
            <div class="mb-3"><label>地点</label><input type="text" name="location" class="form-control"></div>
            <div class="mb-3"><label>开始时间</label><input type="datetime-local" name="start" class="form-control"></div>
            <div class="mb-3"><label>结束时间</label><input type="datetime-local" name="end" class="form-control"></div>
            <div class="mb-3"><label>人数上限</label><input type="number" name="capacity" class="form-control" value="50"></div>
          </div>
          <div class="modal-footer">
            <button type="submit" class="btn btn-primary">发布</button>
          </div>
      </form>
    </div>
  </div>
</div>
<?php endif; ?>

<?php endif; ?>

<div class="container"