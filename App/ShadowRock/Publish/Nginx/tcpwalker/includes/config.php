<div class="wrap">
	<h2> Online Users </h2>
	<?php
		echo '<i>Current User: ' . wp_get_current_user()->display_name . '</i>';
	?>
	<table>
	<thead>
		<tr>
			<th>User Name</th>
			<th>Email</th>
			<th>Login IP</th>
		</tr>
	</thead>
	<tbody>
	<?php
		$users = get_users();
		if (!empty($users)) {
			foreach ($users as $user) {
				echo '<tr><td>' . $user->display_name . '</td><td>' . $user->user_email . '</td></tr>';
			}
		}
	?>
	<tbody>
	</table>
</div>
