<?php

// tm_amp.organization.inc

/**
 * Render organization
 */
function tm_amp_render_organization($node_id) {

	global $conf;

	// Load node
	$node = node_load($node_id);
	if ($node == null) {
		return "Oops, an error occured (invalid id). <a href='/'>Return home</a>.";
	}

	// Note: We use https://www.ampstart.com/templates#news-blog template
	$template_file = drupal_get_path('module', 'tm_amp') . '/templates/tm_amp-template.tpl.php';
	$template = file_get_contents($template_file);

	// Replace global template vars
	$html = tm_amp_insert_template_vars($template);

	// Canonical URL
	$canonical_url = $conf["tm_email_server_http"] . "://" . $_SERVER["HTTP_HOST"] . "/" . drupal_get_path_alias("node/" . $node_id);
	$html = str_replace("__CANONICAL_URL__", $canonical_url, $html);

	// Title
	$title = strip_tags($node->title);

	// Replace variables
	$pre_heading = "<a href='/user/login?destination=" . strip_tags(drupal_get_path_alias("node/" . $node_id)) . "'>Log in</a> or <a href='/user/register'>Sign Up</a>"; // "&raquo; <a href='/companies'>Companies</a>"
	$html = str_replace("__PAGE_TITLE__", $title . " | " . $conf["tm_site_name"], $html);
	$html = str_replace("__PAGE_PRE_HEADING__", $pre_heading, $html);
	$html = str_replace("__PAGE_HEADING__", $title, $html);

	// location
	$location = "";
	if (isset($node->field_location_city[LANGUAGE_NONE][0]['value'])) {
		$location = strip_tags($node->field_location_city[LANGUAGE_NONE][0]['value']);
	}

	$html = str_replace("__PAGE_SUB_HEADING__", $location, $html);

	// Set avatar
	if (empty($node->field_avatar)) {
		$img_uri = $conf["tm_images_default_field_avatar"];
	}  else {
		$img_uri = $node->field_avatar[LANGUAGE_NONE][0]['uri'];
	}

	// If image is default, replace with random image from folder
	if (isset($conf["tm_images_default_path"])) {
		if ($img_uri == $conf["tm_images_default_field_avatar"]) {
			$image_id = $node->nid;
			$cover_files = $conf["tm_images_default_avatar"];
			$image_index = $image_id % sizeof($cover_files);
			$img_uri = $conf["tm_images_default_path"] . $cover_files[$image_index];
		}
	}

	$img_url = file_create_url(image_style_url("avatar", $img_uri));

	// image
	$html = str_replace("__PAGE_IMAGE_URL__", "/" . drupal_get_path_alias("node/" . $node_id), $html); 
	$html = str_replace("__PAGE_IMAGE__", $img_url, $html);
	$html = str_replace("__PAGE_IMAGE_WIDTH__", 128, $html);
	$html = str_replace("__PAGE_IMAGE_HEIGHT__", 128, $html);
	$html = str_replace("__PAGE_IMAGE_LAYOUT", '', $html);
	$html = str_replace("__PAGE_IMAGE_ALT__", $title, $html);

	// description
	$description = "";
	if (isset($node->body[LANGUAGE_NONE][0]['value'])) {
		$description = strip_tags($node->body[LANGUAGE_NONE][0]['value']);
		$description = str_replace("\n", "<br>", $description); // add line breaks
		$description = preg_replace("/[-]{4,10}/", "<br>", $description); // replace ---- with hr
		$description = tm_base_linkify($description, ["http", "https", "mail", "twitter"], ["rel" => "nofollow", "target" => "_blank"]); // add links
	}

	$html = str_replace("__PAGE_DESCRIPTION__", $description, $html);

	// extra
	$num_followers = tm_organizations_get_num_followers($node_id);

	// organization segment
	$organization_segment_l1 = "";
	$organization_segment_l2 = "";
	$field_segment_tid = null;
	if (isset($node->field_segment[LANGUAGE_NONE][0]['tid'])) {
		$field_segment_tid = $node->field_segment[LANGUAGE_NONE][0]['tid'];
	}
	if ($field_segment_tid != null){
		$term = taxonomy_term_load($field_segment_tid);
		// load parent term
		$parent_terms = taxonomy_get_parents($term->tid);
		if (sizeof($parent_terms) > 0) {
			$parent_term = current($parent_terms);
			$organization_segment_l1 = $parent_term->name;
			$organization_segment_l2 = $term->name;
		} else {
			$organization_segment_l1 = $term->name;
			$organization_segment_l2 = "";
		}
	}
	
	$industry_segment = $organization_segment_l1;
	if ($organization_segment_l2 != "") {
		$industry_segment .= " &gt; " . $organization_segment_l1;
	}

	$extra_info = '<section class="px3 mb4">
  <h2 class="mb2"></h2>
  <span class="ampstart-hint block mb3">Organization details</span>
  <ul class="mb4">';

	if ($industry_segment != "") {
		$extra_info .= '<li>' . $industry_segment . '</li>';
	}

	if ($num_followers > 0) {
		$extra_info .= '<li><a href="/node/' . $node_id . '/followers">' . $num_followers . ' follower' . tm_amp_pluralize($num_followers) . '</a></li>';
	}

	// profile managers
	// Show manager names
	$manager_uids = array();
	if (isset($node->field_owner[LANGUAGE_NONE])) {
		foreach ($node->field_owner[LANGUAGE_NONE] as $manager) {
			$manager_uids[] = $manager['target_id'];
		}
	}
	$manager_names = array();
	foreach ($manager_uids as $manager_uid) {
		$manager = user_load($manager_uid);
		$manager_url = $conf["tm_email_server_http"] . "://" . $_SERVER["HTTP_HOST"] . "/" . drupal_get_path_alias("user/" . $manager_uid) . "?amp";
		$manager_names[] = '<a href="' . $manager_url . '">' . strip_tags($manager->field_user_first_name[LANGUAGE_NONE][0]['value'] . " " . $manager->field_user_last_name[LANGUAGE_NONE][0]['value']) . '</a>'; 
	}
	if (sizeof($manager_names) > 0) {
		$last = array_pop($manager_names);
		$manager_links = (count($manager_names) ? implode(", ", $manager_names) . " and " . $last : $last);
		$extra_info .= "<li>Managed by " . $manager_links . "</li>";
	}

	$extra_info .= '</ul>';
	$html = str_replace("__PAGE_EXTRA__", $extra_info, $html);

	// Insert meta data
	$meta_data = tm_amp_render_organization_meta($node, $canonical_url, $description, $location, $img_url);
	$html = str_replace("__META_DATA__", $meta_data, $html);

	// Youtube
	$youtube_js = "";
	$youtube_html = "";
	if (isset($node->field_company_cover_video[LANGUAGE_NONE][0]['value'])) {

		$cover_video_value = $node->field_company_cover_video[LANGUAGE_NONE][0]['value'];
		$youtube_id = tm_amp_get_youtube_id($cover_video_value);
	
		if ($youtube_id != "") {

			$youtube_js = '<script async custom-element="amp-youtube" src="https://cdn.ampproject.org/v0/amp-youtube-0.1.js"></script>';
			$youtube_html = '<p class="mb4 px3"><amp-youtube data-videoid="' . $youtube_id . '" layout="responsive" width="480" height="270"></amp-youtube></p>';
		}
	}

	$html = str_replace("__CUSTOM_SCRIPTS__", $youtube_js, $html);
	$html = str_replace("__YOUTUBE_HTML__", $youtube_html, $html);

	// return html
	return $html;
}

/**
 * Render meta data
 */
function tm_amp_render_organization_meta($node, $canonical_url, $description, $location, $image_url) {

	$meta = '<script type="application/ld+json">
  {
    "@context": "http://schema.org",
    "@type": "Organization",
    "datePublished": "__DATE_PUBLISHED__",
    "dateModified": "__DATE_MODIFIED__",
    "mainEntityOfPage": "__CANONICAL_URL__",
    "name": "__NAME__",
    "description": "__DESCRIPTION__",
    "location": "__LOCATION__",
    "logo": {
      "@type": "ImageObject",
      "url": "__IMAGE_URL__",
      "height": __IMAGE_HEIGHT__,
      "width": __IMAGE_WIDTH__
    }
  }
</script>';

	$meta = str_replace("__CANONICAL_URL__", $canonical_url, $meta);
	$meta = str_replace("__NAME__", str_replace('"', "", strip_tags($node->title)), $meta);
	$description = str_replace("<br>", "\n", $description);
	$meta = str_replace("__DESCRIPTION__", str_replace('"', "", $description), $meta);

	// location, country
	$country_name = "";
	if (isset($node->field_country[LANGUAGE_NONE][0]['iso2'])) {
		$country = country_load($node->field_country[LANGUAGE_NONE][0]['iso2']);
		if ($location != "") {
			$location .= ", ";
		}
		$location .= $country->name;
	}
	$meta = str_replace("__LOCATION__", str_replace('"', "", $location), $meta);

	// image
	$meta = str_replace("__IMAGE_URL__", $image_url, $meta);
	$meta = str_replace("__IMAGE_HEIGHT__", 512, $meta);
	$meta = str_replace("__IMAGE_WIDTH__", 512, $meta);

	// published and modified times
	$time_format = "Y-m-d\TH:i:s-00:00";
	$published_dt = new Datetime(null, new DateTimeZone('UTC'));
	$published_dt->setTimestamp($node->created);
	$published_time = $published_dt->format($time_format); //"2016-01-19T10:38:35-08:00";
	$meta = str_replace("__DATE_PUBLISHED__", $published_time, $meta);

	// check last update time
	$last_update = null;
	if (module_exists("tm_status_updates")) {
    	$last_update = tm_status_updates_get_last_update_timestamp($node->type, $node->nid);
  	}

	// get most recent update between status update and last changed
	$last_changed = $node->changed;
	$most_recent = max(array($last_update, $last_changed));

	// updated
	$updated_dt = new Datetime(null, new DateTimeZone('UTC'));
	$updated_dt->setTimestamp($most_recent);
	$updated_time = $updated_dt->format($time_format); //"2016-01-19T10:38:35-08:00";
	$meta = str_replace("__DATE_MODIFIED__", $updated_time, $meta);

	return $meta;

}
