<?php
/**
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 * 
 * @package 
 *   42mate.
 * 
 * @file    
 *   Admin page callbacks for the simple_queue module.
 */

/**
 * Display a list of queues and their status for the administrator.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_queues_list() {
  $output = '';

  if (module_exists('advanced_help')) {
    $output .= theme('advanced_help_topic', array(
      'module' => 'simple_queue',
      'topic' => 'about',
    ));
    $output .= '&nbsp;' . theme('advanced_help_topic', array(
      'module' => 'simple_queue',
      'topic' => 'about',
      'type' => t('Click here for information about this module'),
    ));
    $output = '<p>' . $output . '</p>';
  }

  // Fetch all of the queues.
  $queues = simple_queue_load_queues(simple_queue_get_all_qids(25));
  
  if (empty($queues)) {
    return $output . t('No simple_queues exist.');
  }

  $header = array(
    array('data' => t('Title'), 'field' => 'title', 'sort' => 'asc'),
    array('data' => t('Max nodes'), 'field' => 'size'),
    array('data' => t('Nodes quantity'), 'field' => 'nodes'),
    array('data' => t('Default display'), 'field' => 'default_display', 'sort' => 'asc'),
    array('data' => t('Direction'), 'field' => 'direction'),
    array('data' => t('Operation')),
  );
  $table_sort = tablesort_init($header);

  // $qids = array();
  $sort_primary = array();
  $sort_secondary = array();
  $sort_direction_regular = array('asc' => SORT_ASC, 'desc' => SORT_DESC);
  $sort_direction_reverse = array('asc' => SORT_DESC, 'desc' => SORT_ASC);
  foreach ($queues as $queue) {
    // If a queue has only one subqueue, store the qid so we can display
    // the number of nodes in the subqueue.

    $sort_secondary[] = drupal_strtolower($queue->title);
    switch ($table_sort['sql']) {
      case 'title':
      default:
        $sort_primary[] = drupal_strtolower($queue->title);
        $sort_direction = $sort_direction_regular;
        break;
      case 'size':
        $sort_primary[] = $queue->size;
        $sort_direction = $sort_direction_reverse;
        break;
      case 'nodes':
        $sort_primary[] = count($queue->nodes);
        $sort_direction = $sort_direction_reverse;
        break;
      case 'default_display':
        $sort_primary[] = $queue->default_display;
        $sort_direction = $sort_direction_regular;
        break;
      case 'direction':
        $sort_primary[] = $queue->direction;
        $sort_direction = $sort_direction_reverse;
        break;
    }
  }

  if (!empty($table_sort)) {
    if (strtolower($table_sort['sort']) == 'desc') {
      array_multisort($sort_primary, $sort_direction['desc'], $sort_secondary, $queues); // Re-indexes array keys; key no longer equals qid.
    }
    else {
      array_multisort($sort_primary, $sort_direction['asc'], $sort_secondary, $queues); // Re-indexes array keys; key no longer equals qid.
    }
  }

  $rows = array();
  foreach ($queues as $queue) {
    $operations = array();
    
    $operations[] = l(t('View'), 'admin/structure/simple_queue/' . $queue->qid . '/view');
    
    $simple_queue_title = check_plain($queue->title);
    
    if (simple_queue_validate_permission(SIMPLE_QUEUE_PERMISSION_EDIT_QUEUES)
        || (simple_queue_validate_permission(SIMPLE_QUEUE_PERMISSION_EDIT_SAME_ROLES_QUEUES) 
        && simple_queue_validate_queue_same_roles($queue->roles))) {
      $operations[] = l(t('Edit'), 'admin/structure/simple_queue/' . $queue->qid . '/edit');
      
      $simple_queue_title = l(check_plain($queue->title), 'admin/structure/simple_queue/' . $queue->qid . '/edit');
    }

    $rows[] = array(
      array('class' => array('simple_queue-title'), 'data' => $simple_queue_title),
      array('class' => array('simple_queue-max-nodes'), 'data' => $queue->size == 0 ? t('Infinite') : $queue->size),
      array('class' => array('simple_queue-nodes'), 'data' => empty($queue->nodes) ? 0 : count($queue->nodes)),
      array('class' => array('simple_queue-default-display'), 'data' => check_plain(empty($queue->default_display) ? 'default' : $queue->default_display)),
      array('class' => array('simple_queue-direction'), 'data' => $queue->direction),
      array('class' => array('simple_queue-operation'), 'data' => implode(' | ', $operations)),
    );
  }

  $output .= theme('table', array('header' => $header, 'rows' => $rows));
  $output .= theme('pager', array('tags' => NULL));

  return $output;
}

/**
 * Add or edit a queue.
 * 
 * @param 
 *   array $form Allow access to the form.
 * 
 * @param 
 *   array $form_state Allow access to the form.
 * 
 * @param 
 *   array $queue Queue to add or edit.
 * 
 * @return 
 *   array $form Contained form values.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_edit_queue_form($form, &$form_state, $queue) {  
  // If the $queue is null - means to add a queue.  
  if (is_string($queue)) {
    drupal_set_title(t('Add queue'));
    $queue = new simple_queue_queue();
  }
  else {
    drupal_set_title(t("Simple queue '@title'", array('@title' => $queue->title)), PASS_THROUGH);
  }
  
  $form['description'] = array(
    '#type' => 'fieldset',
    '#title' => t('Simple queue'),
    '#description' => t('Nodes put into a queue are added to the back of the queue; when a node is added to a full queue, the node in the front of the queue will be popped out to make room.'),
  );

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => $queue->title,
    '#size' => 50,
    '#required' => TRUE,
    '#maxlength' => 64,
    '#description' => t('Enter the name of the queue'),
  );

  $form['name'] = array(
    '#type' => 'machine_name',
    '#maxlength' => 32,
    '#machine_name' => array(
      'exists' => 'simple_queue_machine_name_exists',
      'source' => array('title'),
    ),
    '#description' => t('A unique machine-readable name for this queue. It must only contain lowercase letters, numbers, and underscores.'),
  );

  if (!empty($queue->name)) {
    $form['name']['#default_value'] = $queue->name;
    $form['name']['#disabled'] = TRUE;
    $form['name']['#value'] = $queue->name;
  }

  $form['size'] = array(
    '#type' => 'textfield',
    '#title' => t('Queue size'),
    '#default_value' => $queue->size,
    '#size' => 2,
    '#maxlength' => 2,
    '#description' => t('The maximum number of nodes will appear in the queue. Enter 0 for no limit'),
  );
  
  $form['direction'] = array(
    '#type' => 'checkbox',
    '#title' => t('Direction'),
    '#default_value' => $queue->direction,
    '#description' => t('How the new nodes are going to be added to the queue. 0 (no marked) means from top, 1 (marked) means from the bottom. .'),
  );  

  $form['roles'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Roles'),
    '#required' => TRUE,
    '#default_value' => empty($queue->roles) ? array() : $queue->roles,
    '#options' => user_roles(),
    '#description' => t('Check each role that can add nodes to the queue. Be sure that roles you want to appear here have "manipulate queues" access in the main access control panel.'),
  );
  
  $form['display'] = array(
    '#title' => t('Default display'),
    '#type' => 'select',
    '#description' => t('Select the view mode.'),
    '#options' => simple_queue_get_view_modes_formatted(),
    '#default_value' => $queue->default_display,
  );
  
  $form['types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Types'),
    '#required' => TRUE, 
    '#default_value' => $queue->types,
    '#options' => node_type_get_names(), // Returns a list of available node type names.
    '#description' => t('Check each node type that can be added to this queue.'),
  );
  
  // In case of edit queue.  
  if (isset($queue->qid)) {
    if ((simple_queue_validate_permission(SIMPLE_QUEUE_PERMISSION_DELETE_QUEUES)
        || (simple_queue_validate_permission(SIMPLE_QUEUE_PERMISSION_DELETE_SAME_ROLES_QUEUES)
        && simple_queue_validate_queue_same_roles($queue->roles)))) {    
      
      $form[] = array(
        '#type' => 'submit',
        '#value' => t('Delete'),
        '#validate' => array('simple_queue_edit_queue_form_delete_validate'),
        '#submit' => array('simple_queue_edit_queue_form_delete_submit'),
      );
    }
    
    $form['qid'] = array(
      '#type' => 'value',
      '#value' => $queue->qid,
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  return $form;
}

/**
 * Validate the form.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_edit_queue_form_validate($form, &$form_state) { 
  
  
  
  // Validate title.
   if (empty($form_state['values']['title'])) {
    form_set_error('title', t('Please enter a title for this queue.'));
  }
  
  // Validate size.
  if (!is_numeric($form_state['values']['size'])) {
    form_set_error('size', t('Please enter a numeric value to the size.'));
  }  
}

/**
 * Submit function for the simple_queue_queue form.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_edit_queue_form_submit($formid, &$form_state) {
  $queue = (object) $form_state['values'];
  // Fix checkboxes.
  $queue->roles = array_keys(array_filter($queue->roles));
  $queue->types = array_keys(array_filter($queue->types)); 

  if (!isset($queue->qid)) {
    $queue->new = TRUE;
  }

  $qid = simple_queue_save($queue); // sets $queue->qid if needed.

  if (!empty($queue->new)) {
    $form_state['values']['qid'] = $qid;
    drupal_set_message(t('The queue has been created.'));
  }
  else {
    drupal_set_message(t('The queue has been updated.'));
  }
 
  $form_state['redirect'] = 'admin/structure/simple_queue';
}

/**
 * Delete-validate function for the nodequeue_queue form.
 * 
 * @author  
 * Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_edit_queue_form_delete_validate($form, &$form_state) {
  // TODO No validation for delete step!  But we need to have this so the default validation isn't called.
}

/**
 * Delete-submit function for the simple_queue_queue form.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_edit_queue_form_delete_submit($formid, &$form_state) {
  $form_state['redirect'] = 'admin/structure/simple_queue/' . $form_state['values']['qid'] . '/delete';
}

/**
 * Confirm form to delete a queue.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_admin_delete($form, &$form_state, $queue) {
  $form['qid'] = array('#type'  => 'value', '#value' => $queue->qid);
  return confirm_form($form,
    t('Are you sure you want to delete "%title"?', array('%title' => $queue->title)),
    isset($_GET['destination']) ? $_GET['destination'] : 'admin/structure/simple_queue',
    t('This action cannot be undone.'),
    t('Delete'), t('Cancel')
  );
}

/**
 * Submit function for simple_queue delete.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_admin_delete_submit($formid, &$form_state) {
  if ($form_state['values']['confirm']) {
    simple_queue_delete($form_state['values']['qid']);
    drupal_set_message(t('The queue has been deleted.'));
  }
  $form_state['redirect'] = 'admin/structure/simple_queue';
}

/**
 * Load an array of $qids.
 *
 * This exists to provide a way of loading a bunch of queues with
 * the fewest queries. Loading 5 queues results in only 4 queries,
 * not 20. This also caches queues so that they don't get loaded
 * repeatedly.
 *
 * @param $qids
 *   An array of queue IDs to load.
 *
 * @param $bypass_cache
 *   Boolean value indicating whether to bypass the cache or not.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_load_queues($qids = array(), $bypass_cache = FALSE) {
  static $cache = array();
  $to_load = $loaded = array();

  foreach ($qids as $qid) {
    if ($bypass_cache || !isset($cache[$qid])) {
      $to_load[] = $qid;
    }
  }

  if (!empty($to_load)) {
    $result = db_query("SELECT q.* FROM {simple_queue_queue} q WHERE q.qid IN (:to_load) GROUP BY q.qid", array(':to_load' => $to_load));

    foreach ($result as $queue) {
      $loaded[$queue->qid] = $queue;
      // ensure valid defaults:
      $loaded[$queue->qid]->types = array();
      $loaded[$queue->qid]->roles = array();
      $loaded[$queue->qid]->count = 0;
    }

    $result = db_query("SELECT qid, rid FROM {simple_queue_roles} WHERE qid IN (:to_load)", array(':to_load' => $to_load));
    foreach ($result as $obj) {
      $loaded[$obj->qid]->roles[] = $obj->rid;
    }

    $result = db_query("SELECT qid, type FROM {simple_queue_types} WHERE qid IN (:to_load)", array(':to_load' => $to_load));
    foreach ($result as $obj) {
      $loaded[$obj->qid]->types[] = $obj->type;
    }
    
    $result = db_query("SELECT n.qid, n.nid, n.position, n.timestamp, n.display, n.lock FROM {simple_queue_nodes} n WHERE n.qid IN (:to_load)", array(':to_load' => $to_load));
    
    foreach ($result as $obj) {      
      $loaded[$obj->qid]->nodes[] = $obj;
    }

    $context = 'load_queues';
    drupal_alter('simple_queue', $loaded, $context);
  }

  if ($bypass_cache) {
    return $loaded;
  }
  else {
    if (!empty($loaded)) {
      $cache += $loaded;
    }
    $queues = array();
    foreach ($qids as $qid) {
      if (isset($cache[$qid])) {
        $queues[$qid] = $cache[$qid];
      }
    }
    return $queues;
  }
}

/**
 * Get an array of qids using the pager query. This administrative list
 * does no permission checking, so should only be available to users who
 * have passed the 'administer queues' check.
 *
 * @param $page_size
 *   The page size to use. If this is 0 or NULL, all queues will be returned.
 *   Defaults to 0.
 * 
 * @param $pager_element
 *   In the rare event this should use another pager element, set this.
 * 
 * @param $bypass_cache
 *   Boolean value indicating whether to bypass the cache or not.
 *
 * @return $qids
 *   An array in the format: @code { array($qid => $qid) }
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_get_all_qids($page_size = 0, $pager_element = 0, $bypass_cache = FALSE) {
  $cache = &drupal_static(__FUNCTION__, array());
  if ($bypass_cache || empty($cache[$page_size])) {    

    // TODO First must validate if the user has the queue list permission. 
    // If the user has this permission then the view queues same permission is canceled.
    if (!simple_queue_validate_permission(SIMPLE_QUEUE_PERMISSION_LIST_QUEUES) 
        && simple_queue_validate_permission(SIMPLE_QUEUE_PERMISSION_VIEW_SAME_ROLES_QUEUES)) {
      global $user;
      
      //SELECT r.qid FROM simple_queue_roles r WHERE IN (:rids)
      $query = db_select('simple_queue_roles', 'r')
        ->fields('r', array('qid'))
        ->condition('r.rid', array_keys($user->roles), 'IN');
    } 
    else {
      $query = db_select('simple_queue_queue', 'q')
        ->fields('q', array('qid'));
    }

    if (!empty($page_size)) {
      $query->extend('PagerDefault')
        ->extend('TableSort')
        ->limit($page_size)
        ->element($pager_element);
    }

    $qids = $query->execute()->fetchAllKeyed(0, 0);

    $cache[$page_size] = $qids;
  }
  return $cache[$page_size];
}

/**
 * Get view modes formatted.
 * 
 * @return 
 *   array view modes formatted.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_get_view_modes_formatted() {
  $view_mode_formatted = array("" => 'Default');

  foreach (entity_get_info('node')['view modes'] as $key => $value) {
    if ($value['custom settings']) {
      $view_mode_formatted[$key] = $value['label'];
    }    
  }

  return $view_mode_formatted;
}