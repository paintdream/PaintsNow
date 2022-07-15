<?php
/*
Plugin Name: Tcp Walker
Plugin URI: http://paintsnow.net
Descriptor: Establish p2p connection via Tcp Walker.
Version: 0.0.0.1
Author: PaintDream
Author URI: http://paintdream.com
License: MIT License
*/

if (is_admin()) {
	add_action('admin_menu', 'tcpwalker_menu');
	add_action('wp_ajax_tcpwalker_request', 'tcpwalker_request');
}

add_action('plugins_loaded', 'tcpwalker_init');

function tcpwalker_init() {
	add_action('wp_footer', 'tcpwalker_footer');
}

function tcpwalker_menu() {
	add_options_page('TcpWalker Settings', 'TcpWalker Settings', 'administrator', 'tcpwalker', 'tcpwalker_index_page');
}

function tcpwalker_index_page() {
	include('includes/config.php');
}

function tcpwalker_footer() {
	if (is_user_logged_in()) {
		echo '<h4>TCP Walker online.</h4>';
	} else {
		echo "<h4>TCP Walker offline.</h4>";
	}
}

function tcpwalker_request() {
	include('includes/request.php');
}

?>
