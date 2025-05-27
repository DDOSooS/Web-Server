#!/usr/bin/php
<?php
header("Content-Type: text/html");
?>
<html>
<head><title>PHP CGI Test</title></head>
<body>
<h1>PHP CGI Script Working!</h1>
<h2>Environment Variables:</h2>
<ul>
<li>REQUEST_METHOD: <?php echo $_ENV['REQUEST_METHOD'] ?? 'Not Set'; ?></li>
<li>QUERY_STRING: <?php echo $_ENV['QUERY_STRING'] ?? 'Not Set'; ?></li>
<li>SCRIPT_NAME: <?php echo $_ENV['SCRIPT_NAME'] ?? 'Not Set'; ?></li>
<li>SERVER_NAME: <?php echo $_ENV['SERVER_NAME'] ?? 'Not Set'; ?></li>
<li>SERVER_PORT: <?php echo $_ENV['SERVER_PORT'] ?? 'Not Set'; ?></li>
</ul>

<?php if (!empty($_GET)): ?>
<h2>GET Parameters:</h2>
<ul>
<?php foreach($_GET as $key => $value): ?>
<li><?php echo htmlspecialchars($key); ?>: <?php echo htmlspecialchars($value); ?></li>
<?php endforeach; ?>
</ul>
<?php endif; ?>

<?php if (!empty($_POST)): ?>
<h2>POST Parameters:</h2>
<ul>
<?php foreach($_POST as $key => $value): ?>
<li><?php echo htmlspecialchars($key); ?>: <?php echo htmlspecialchars($value); ?></li>
<?php endforeach; ?>
</ul>
<?php endif; ?>

<h2>PHP Info:</h2>
<p>PHP Version: <?php echo phpversion(); ?></p>
</body>
</html>