<?php
/**
 * @file
 * tm_notifications_messaging.strongarm.inc
 */

/**
 * Implements hook_strongarm().
 */
function tm_notifications_messaging_strongarm() {
  $export = array();

  $strongarm = new stdClass();
  $strongarm->disabled = FALSE; /* Edit this to true to make a default strongarm disabled initially */
  $strongarm->api_version = 1;
  $strongarm->name = 'tm_notifications__member_message';
  $strongarm->value = array(
    'user_label' => 'Allow members who I follow to contact me',
    'default_state' => '1',
    'change_allowed' => 1,
    'email' => array(
      'subject' => '__USER_FIRST_NAME__ __USER_LAST_NAME__ just sent you a message via __SITE_NAME__',
      'message' => 'Hi [user:field_user_first_name],
<br>
<br>__USER_FIRST_NAME__ __USER_LAST_NAME__ just sent you a message on __SITE_NAME__.
__MEMBER_WIDGET__
<strong>Message:</strong>
<br>
<br> __MESSAGE__
<br>
<br><strong>You can respond to __USER_FIRST_NAME__ by replying directly to this email.</strong>
<br>
<br>__EMAIL_SIGNOFF__
',
    ),
  );
  $export['tm_notifications__member_message'] = $strongarm;

  return $export;
}
