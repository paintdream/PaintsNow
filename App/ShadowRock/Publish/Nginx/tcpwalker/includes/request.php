<?php
	$event = $_REQUEST['Action'];
	/* validate */
	if ($event == 'Enumerate') {
		$users = get_users();
		$data = array();
		foreach ($users as $u) {
			array_push($data, array($u->user_login, get_user_meta($u->ID, 'tcpwalker_ip', true), get_user_meta($u->ID, 'tcpwalker_port', true), get_user_meta($u->ID, 'tcpwalker_target', true)));
		}

		$result = array('Status' => 'OK', 'data' => $data);
	} else {
		$user = wp_get_current_user();
		if ($event == 'Login') {
			$ip = $_SERVER['REMOTE_ADDR'];
			$port = $_SERVER['REMOTE_PORT'];
			$serverPort = $_REQUEST['ServerPort']
			if (!update_user_meta($user->ID, 'tcpwalker_ip', $ip))
				add_user_meta($user->ID, 'tcpwalker_ip', $ip, true);
			if (!update_user_meta($user->ID, 'tcpwalker_port', $port)) {
				add_user_meta($user->ID, 'tcpwalker_passive_port', $port, true);
				add_user_meta($user->ID, 'tcpwalker_port', $serverPort, true);
			}
			$result = array('Status' => 'OK', 'Address' => $ip, 'PassivePort' => $port, 'Port' => $serverPort);
		} elseif ($event == 'Logoff') {
			delete_user_meta($user->ID, 'tcpwalker_ip');
			delete_user_meta($user->ID, 'tcpwalker_port');
			delete_user_meta($user->ID, 'tcpwalker_passive_port');
		} elseif ($event == 'Connect') {
			$id = $_REQUEST['Target'];
			add_user_meta($user->ID, 'tcpwalker_target', $id, true);
		} elseif ($event == 'Disconnect') {
			delete_user_meta($user->ID, 'tcpwalker_target');
		} else {
			$result = array('Status' => 'ERROR', 'Info' => 'Not implemented');
		}
	}

	echo json_encode($result);
	die();
?>