<?php

// tm_chapters.csv.inc - csv methods

/**
 * Check permission to download CSV
 */
function tm_chapters_download_csv_check_permission() {

  global $user;
  global $conf;
  $nid = _orig_nid();

  if($nid == 0) {
    return false;
  }

  // check if:
  // - moderator, or
  // - administrator
  if ((in_array("moderator", $user->roles)) or (in_array("administrator", $user->roles))) {
    return true;
  }

  // if chapter leader, or community manager
  // check if has role
  if (tm_users_is_chapter_leader($user->uid, $nid)) {
    $access_role = $conf["tm_chapters_download_csv_role_required"];
    if ((!in_array($access_role, $user->roles))) {
          drupal_goto("/" . drupal_get_path_alias("node/" . $nid) . "/export/message");
    } else {
      return true;
    }
  }

  // access denied
  return false;

}

/**
 * Add html meta tags
 * Called from tm/themes/template.php
 */
function tm_chapters_preprocess_html(&$variables, $hook) {

  global $conf;

  // add body tag
  if ((arg(0) == "chapters") and (arg(2) == "export")) {
    $variables['classes_array'][] = "page-chapter-download-csv";
  }
}

/**
 * Display message if user needs role to access
 * example to add role:
 * drush user-add-role "pro chapter leader" --mail=demo@example.massiveplatform.com
 */
function tm_chapters_download_csv_request_access_message() {

  global $conf;
  global $user;

  // Create form
  $form = array();

  // Only chapter leaders can view message
  if (!(in_array("chapter leader", $user->roles) or (in_array("moderator", $user->roles)) or (in_array("administrator", $user->roles)))) {
    drupal_access_denied();
  }

  // Swap in first name
  $account = user_load($user->uid);
  $terms_text = str_replace("__first_name__", $account->field_user_first_name[LANGUAGE_NONE][0]['value'], $conf["tm_chapters_download_csv_role_required_message"]); 

  // Display terms
  $html = "<p>" . $terms_text . "</p>";

  // Agree and cancel buttons
  $html .= '<input onClick="document.location=\'' . $conf["tm_chapters_download_csv_role_required_learn_more_url"] . '\'" type="submit" id="edit-submit" name="op" value="Learn more" class="form-submit">';
  $html .= ' <input onClick="window.history.back()" type="submit" value="Cancel" class="form-submit">';


  $form['confirm_agreement'] = array('#markup' => $html);

  return $form;

}

/**
 * Display terms of use before downloading member data
 */
function tm_chapters_download_csv() {

  global $conf;
  global $user;

  // Check we have a node
  $nid = _orig_nid();
  if($nid == 0) {
    print drupal_not_found();
    return;
  }

  // Chapter url
  $chapter_url = drupal_get_path_alias("node/" . $nid);

  // Create form
  $form = array();

  // Swap in first name
  $account = user_load($user->uid);
  $terms_text = str_replace("__first_name__", $account->field_user_first_name[LANGUAGE_NONE][0]['value'], $conf["tm_terms_chapter_data_terms_text"]); 

  // Display terms
  $html = "<p>" . $terms_text . "</p>";

  // Agree and cancel buttons
  $html .= '<input onClick="document.getElementById(\'tm_show_after_terms_agree\').style.display = \'block\'; document.location=\'/' . $chapter_url . '/export/confirm\'" type="submit" id="edit-submit" name="op" value="I agree" class="form-submit">';
  $html .= ' <input onClick="document.location=\'/' . $chapter_url . '\'" type="submit" value="Cancel" class="form-submit">';

  $html .= "<br><br><span id='tm_show_after_terms_agree' style='display: none;'>Your download is starting &mdash; you can return to your <a href='/" . $chapter_url . "'>chapter page</a>.</span>";
 
  $form['confirm_agreement'] = array('#markup' => $html);

  return $form;

}


/**
 * Download Chapter CSV function 
 */
function tm_chapters_download_csv_confirm(){

  global $user;
  global $conf;
  $nid = _orig_nid();

  if($nid == 0) {
    print drupal_not_found();
    return;
  }

  // Additional check if 
  // check if have permission
  if (in_array($user->uid, $conf["tm_chapters_users_who_can_access_csv"])) {
    return true;
  }

  // Get chapter
  $chapter = node_load($nid);

  // flag chapter as having downlaoded data
  if ($chapter != null) {
    $flag = flag_get_flag('download_member_csv');
    $flag->flag("unflag", $chapter->nid); // unflag so we can flag again
    $flag->flag("flag", $chapter->nid);
  }

  // create filename
  $oldzone = date_default_timezone_get();
  date_default_timezone_set(drupal_get_user_timezone());
  $csvname = $chapter->title.'_'.date('M_d_Y_Gi',time()) . ".csv";
  date_default_timezone_set($oldzone);

  // csv header
  drupal_add_http_header('Content-Type', 'text/csv; utf-8');
  drupal_add_http_header('Content-Disposition', 'attachment; filename = '.$csvname);
  
  // check cache
  $cache = cache_get('tm-chapters-members-csv-' . $chapter->nid, 'cache');
  if (!empty($cache)) {
    echo $cache->data;
  } else {
    echo "Please run drush tm-chapters-members-csv-cache to generate csv";
  }

}


/**
 * Generate Chapters CSV
 */
function tm_chapters_members_csv_generate($nid) {

  global $conf;

  // Get chapter
  $chapter = node_load($nid);

  // Create memory file handler
  $fh = fopen('php://memory', 'w');

  // put information use reminder
 
  $message = "# IMPORTANT: Please care about our members' privacy.";
  $message .= "\n# Under the " . $conf['tm_site_name'] . " Privacy Terms you cannot transfer personal data (name or email) to sponsors or 3rd parties.";
  $message .= "\n# Please use the SubscribeChapterNewsletter field to manage external email newsletter subscriptions.\n\n";

  fputs($fh,$message,strlen($message));

  // puts a header row across the top of the csv
  $item = array(t('First name'),t('Last name'),t('Email'),t('SubscribeChapterNewsletter'),t('SubscribeGlobalNewsletter'),t('SubscribeChapterAnnouncements'),t('Country'),t('City'),t('Organization'),t('Role'),t('Website'),t('Segment'),t('MemberSince'),t('Events'),t('Waitlists'), t('MemberType'));
  foreach ($conf["tm_users_link_fields"] as $field_name) {
    $item[] = ucwords($field_name);
  }
  fputcsv($fh, $item);

  // outside of loop for better performance
  $module_exists_tm_subscriptions_user = module_exists("tm_subscriptions_user");

  // preload flags
  if ($module_exists_tm_subscriptions_user) {
    $user_subscription_enable_flag = flag_get_flag('user_subscription_enable', NULL);
  }

  // list of users
  $users = db_select('flagging', 'f')
    ->fields('f')
    ->condition('entity_id',$nid,'=')
    ->condition('entity_type','node','=')
    ->execute();
  while ($load_user = $users->fetchAssoc()){
    $account = user_load($load_user['uid']);
    if (empty($account->language)) $language = LANGUAGE_NONE;
    else $language = $account->language;

    $item = array();
    // columns in same order as the header array above
    $item[] = $account->field_user_first_name[$language][0]['value'];
    $item[] = $account->field_user_last_name[$language][0]['value'];
    $item[] = $account->mail;
    $notif = new TMNotification("chapter_newsletters");
    $item[] = ($notif->isEnabled($account->uid) ? "yes" : "no");
    $notif = new TMNotification("global_newsletter");
    $item[] = ($notif->isEnabled($account->uid) ? "yes" : "no");
    $notif = new TMNotification("user_chapter_announcements");
    $item[] = ($notif->isEnabled($account->uid) ? "yes" : "no");
    if (isset($account->field_user_country[$language][0])){
      $item[] = $account->field_user_country[$language][0]['iso2'];
    }
    else $item[] = t('none');
    if (isset($account->field_location_city[$language][0]['value'])){
      $item[] = $account->field_location_city[$language][0]['value'];
    }
    else $item[] = t('none');
    if (isset($account->field_job_organization[$language][0]['value'])){
      $item[] = $account->field_job_organization[$language][0]['value'];
    }
    else $item[] = t('none');
    if (isset($account->field_job_role[$language][0]['value'])){
      $item[] = $account->field_job_role[$language][0]['value'];
    }
    else $item[] = t('none');
    if (isset($account->field_link_website[$language][0]['url'])){
      $item[] = $account->field_link_website[$language][0]['url'];
    }
    else $item[] = t('none');
    if (isset($account->field_segment[$language][0]['tid'])){
      $term = taxonomy_term_load($account->field_segment[$language][0]['tid']);
      // load parent term
      $parent_terms = taxonomy_get_parents($term->tid);
      if (sizeof($parent_terms) > 0) {
        $parent_term = current($parent_terms);
        $item[] = $parent_term->name. " > " . $term->name;
      } else {
        $item[] = $term->name;
      }
    }
    else $item[] = t('none');
    $item[] = date('j M Y',$load_user['timestamp']);

    // number of registered events
    $num_events = tm_chapters_get_num_events_for_member($account->uid, $chapter->nid, "event_register");
    $item[] = $num_events;

    // number of waitlists
    $num_waitlists = tm_chapters_get_num_events_for_member($account->uid, $chapter->nid, "event_waitlist");
    $item[] = $num_waitlists;

    // guest member or approved
    $member_type = "Guest Member";
    if (in_array("approved user", $account->roles)) {
      $member_type = "Approved Member";
    }
    if (in_array("chapter leader", $account->roles)) {
      $member_type = "Chapter Leader";
    }

    // member type - show user subscription, or override with business subscription

    // user subscription
    // use flag lookup directly for performance
    if ($module_exists_tm_subscriptions_user) {
      if ($user_subscription_enable_flag->is_flagged($account->uid)) {
        $member_type = $conf["tm_subscriptions_user_label_noun"];
      }
    }
    $item[] = $member_type;

    // add user fields
    foreach ($conf["tm_users_link_fields"] as $field_name) {
      $obj_field_name = "field_link_" . $field_name;
      if (isset($account->$obj_field_name[LANGUAGE_NONE][0]['url'])) {
        $item[] = strip_tags($account->$obj_field_name[LANGUAGE_NONE][0]['url']);
      } else {
        $item[] = "";
      }
    }

    // remove commas from any input data
    $new_item = array();
    foreach($item as $col => $val) {
      $new_item[$col] = str_replace(",", "", $val);
    }

    fputcsv($fh, $new_item);
  } 

  rewind($fh);

  // put it all in a variable
  $csv = stream_get_contents($fh);

  // close file handle
  fclose($fh);

  // return csv
  return $csv;
}
