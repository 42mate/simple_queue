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
  
  $sort_primary = array();
  $sort_secondary = array();
  $sort_direction_regular = array('asc' => SORT_ASC, 'desc' => SORT_DESC);
  $sort_direction_reverse = array('asc' => SORT_DESC, 'desc' => SORT_ASC);

  foreach ($queues as $queue) {
    // If a queue has only one queue, store the qid so we can display
    // the number of nodes in the queue.

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
        $sort_primary[] = empty($queue->nodes) ? 0 : count($queue->nodes);
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
      // Re-indexes array keys; key no longer equals qid.
      array_multisort($sort_primary, $sort_direction['desc'], $sort_secondary, $queues);
    }
    else {
      // Re-indexes array keys; key no longer equals qid.
      array_multisort($sort_primary, $sort_direction['asc'], $sort_secondary, $queues);
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
 * @param 
 *   array $form Allow access to the form.
 * 
 * @param 
 *   array $form_state Allow access to the form.
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
 * @param 
 *   array $form_id The unique string identifying the desired form.
 * 
 * @param 
 *   array $form_state Allow access to the form.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_edit_queue_form_submit($form_id, &$form_state) {
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
 * Delete-validate function for the simple_queue_queue form.
 * 
 * @param 
 *   array $form Allow access to the form.
 * 
 * @param 
 *   array $form_state Allow access to the form.
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
 * @param 
 *   array $form_id The unique string identifying the desired form.
 * 
 * @param 
 *   array $form_state Allow access to the form.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_edit_queue_form_delete_submit($form_id, &$form_state) {
  $form_state['redirect'] = 'admin/structure/simple_queue/' . $form_state['values']['qid'] . '/delete';
}

/**
 * Confirm form to delete a queue.
 * 
 * @param 
 *   array $form_id The unique string identifying the desired form.
 * 
 * @param 
 *   array $form_state Allow access to the form.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_admin_delete($form, &$form_state, $queue) {
  $form['qid'] = array('#type'  => 'value', '#value' => $queue->qid);
  $destination = filter_input(INPUT_GET, 'destination');
  return confirm_form($form,
    t('Are you sure you want to delete "%title"?', array('%title' => $queue->title)), 
    isset($destination) ? $destination : 'admin/structure/simple_queue',
    t('This action cannot be undone.'),
    t('Delete'), t('Cancel')
  );
}

/**
 * Submit function for simple_queue delete.
 * 
 * @param 
 *   array $form_id The unique string identifying the desired form.
 * 
 * @param 
 *   array $form_state Allow access to the form.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_admin_delete_submit($form_id, &$form_state) {
  if ($form_state['values']['confirm']) {
    simple_queue_delete($form_state['values']['qid']);
    drupal_set_message(t('The queue has been deleted.'));
  }
  $form_state['redirect'] = 'admin/structure/simple_queue';
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
 * @return array
 *   View modes formatted.
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

/**
 * Page callback to view a queue.
 * 
 * @param 
 *   array $queue queue.
 * 
 * @return 
 *   array returns a renderable form array for a given form ID.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_nodes_list($queue) {
  drupal_set_title(t("Simple queue '@title'", array('@title' => $queue->title)), PASS_THROUGH);

  $breadcrumb = drupal_get_breadcrumb();
  $breadcrumb[] = l($queue->title, "admin/structure/simple_queue/$queue->qid");
  drupal_set_breadcrumb($breadcrumb);
  
  return simple_queue_arrange_queue($queue);
}

/**
 * View the contents of a queue, with links to re-order the queue.
 * 
 * @param 
 *   object $queue queue.
 * 
 * @return 
 *   array returns a renderable form array for a given form ID.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_arrange_queue($queue) {
  // get nodes from the queue
  $nodes = _simple_queue_dragdrop_get_nodes($queue);

  return drupal_get_form('simple_queue_arrange_queue_form_' . $queue->qid, $queue, $nodes);
}

/**
 * Return a list of nodes in a specific queue.
 * 
 * @param 
 *   object $queue queue.
 * 
 * @return 
 *   array a list of nodes in a specific queue.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function _simple_queue_dragdrop_get_nodes($queue) {
  //$order = $queue->reverse ? 'DESC' : 'ASC';

  $visible = simple_queue_nids_visible($queue->qid);

  // Get a list of all nodes in the queue, regardless of access restrictions.
  $query = db_select('node', 'n')
    ->distinct();

  $query->leftJoin('simple_queue_nodes', 'nq', 'nq.nid = n.nid');
  $query->fields('n', array('nid'))
    ->fields('nq')
    ->condition('nq.qid', $queue->qid);
    //->orderBy('nq.position', $order);
  $result = $query->execute();
 
  $nids = array();
  $sq_nodes = array();
  foreach ($result as $sq_node) {
    $nids[] = $sq_node->nid;
    // Save node position in a separate array so we can reference it later.
    $sq_nodes[$sq_node->nid]['position'] = $sq_node->position;
    $sq_nodes[$sq_node->nid]['lock'] = $sq_node->lock;
  }
  $nodes = node_load_multiple($nids);

  foreach ($nodes as $node) {
    $node->visible = isset($visible[$node->nid]) ? TRUE : FALSE;
    $node->position = $sq_nodes[$node->nid]['position'];
    $node->lock = $sq_nodes[$node->nid]['lock'];
  }

  return $nodes;
}

/**
 * Form definition for simple_queue drag'n'drop form.
 * 
 * @param 
 *   array $form Allow access to the form.
 * 
 * @param 
 *   array $form_state Allow access to the form.
 * 
 * @param 
 *   array $queue queue.
 * 
 * @return 
 *   array $form Contained form values.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_arrange_queue_form($form, $form_state, $queue, $nodes) {
  $form = array('#tree' => TRUE);
  
  // Prepare the main part of the form which will be themed as a table.
  $count = count($nodes);
  $form['nodes'] = array();
  $form['nodes']['#theme'] = 'simple_queue_arrange_queue_form_table';

  // Theme function needs these.
  $form['nodes']['#queue']    = (array) $queue;

  foreach ($nodes as $node) {
    $form['nodes'][$node->nid]['#node'] = (array) $node;
    if ($node->visible) {     
      $form['nodes'][$node->nid]['#node'] = (array) $node;
      $form['nodes'][$node->nid]['title'] = array('#markup' => l($node->title, 'node/' . $node->nid));
      $form['nodes'][$node->nid]['status'] = array('#markup' => intval($node->status) === NODE_PUBLISHED ? t('Published') : t('Not published'));
      $form['nodes'][$node->nid]['type'] = array('#markup' => $node->type);
      $form['nodes'][$node->nid]['lock'] = array('#markup' => $node->lock);
      $form['nodes'][$node->nid]['date'] = array('#markup' => format_date($node->created, 'short'));
    }
    else {
      $form['nodes'][$node->nid]['title'] = array('#value' => t('Restricted node, NID: @nid', array('@nid' => $node->nid)));
      $form['nodes'][$node->nid]['status'] = array('#value' => '');
      $form['nodes'][$node->nid]['type'] = array('#value' => '');
      $form['nodes'][$node->nid]['lock'] = array('#value' => '');
      $form['nodes'][$node->nid]['date'] = array('#value' => '');
    }

    if (node_access('update', $node)) {
      $form['nodes'][$node->nid]['edit'] = array('#markup' => l(t('edit'), 'node/' . $node->nid . '/edit', array('attributes' => array('title' => t('Edit this node')))));
    }
    $form['nodes'][$node->nid]['position'] = array(
      '#type' => 'position',
      '#delta' => $count,
      '#default_value' => $node->position,
      '#attributes' => array(
        'class' => array('node-position'),
      ),
    );

    $attr = array(
      '#attributes' => array(
        'title' => t('Remove from queue'),
        'style' => 'display: none;',
        'class' => array('simple_queue-remove'),
        'id' => 'simple_queue-remove-' . $node->nid,
      ),
      'query' => simple_queue_get_query_string($node->nid, TRUE),
    );

    $form['nodes'][$node->nid]['remove'] = array('#markup' => l(t('remove'), 'admin/structure/simple_queue/' . $queue->qid . '/remove-node/' . $node->nid, $attr));
  }

  // Validate permission to add node to the queue.
  if ((simple_queue_validate_permission(SIMPLE_QUEUE_PERMISSION_ADD_NODES_QUEUES)
        || (simple_queue_validate_permission(SIMPLE_QUEUE_PERMISSION_ADD_NODES_SAME_ROLES_QUEUES)
        && simple_queue_validate_queue_same_roles($queue->roles)))) {    
    
    // Textfield for adding nodes to the queue.
    $form['add'] = array(
      '#type' => 'container',
      '#attributes' => array('class' => array('container-inline')),
    );
    $form['add']['nid'] = array(
      '#type' => 'textfield',
      '#required' => TRUE,
      '#autocomplete_path' => 'simple_queue/autocomplete/' . $queue->qid,
      '#maxlength' => 1024,
      '#default_value' => t('Enter the title of a node to add it to the queue'),
      '#attributes' => array('class' => array('queue-add-nid')),
    );
    $form['add']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Add content'),
      '#validate' => array('simple_queue_arrange_queue_form_add_validate'),
      '#submit' => array('simple_queue_arrange_queue_form_add_submit'),
    );
  }  

  // Submit, and clear actions.
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#submit' => array('simple_queue_arrange_queue_form_submit'),
  );
  $form['actions']['clear'] = array(
    '#type' => 'submit',
    '#value' => t('Clear'),
    '#submit' => array('simple_queue_arrange_queue_form_clear_submit'),
  );

  // Disable all buttons if the queue is empty.
  if ($count == 0) {
    $form['actions']['submit']['#disabled']  = TRUE;
    $form['actions']['clear']['#disabled']   = TRUE;
  }

  return $form;
}

/**
 * Validate handler for simple_queue_arrange_queue_form.
 * 
 * @param 
 *   array $form Allow access to the form.
 * 
 * @param 
 *   array $form_state Allow access to the form.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_arrange_queue_form_validate($form, &$form_state) {
  $positions = array();
  foreach ($form_state['values'] as $nid => $element) {
    if (is_numeric($nid) && is_numeric($element['position'])) {
      $positions[$nid] = $element['position'];
    }
  }
  if (count(array_unique($positions)) < count($positions)) {
    $seen = array();
    foreach ($positions as $nid => $position) {
      if (isset($seen[$position])) {
        form_set_error($nid . '][position', t('Duplicate position value.'));
      }
      $seen[$position] = TRUE;
    }
  }
}

/**
 * Submit handler for simple_queue drag'n'drop form. Updates node positions in {simple_queue_nodes}.
 * 
 * @param 
 *   array $form Allow access to the form.
 * 
 * @param 
 *   array $form_state Allow access to the form.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_arrange_queue_form_submit($form, &$form_state) {
  $nodes = array();
  foreach ($form_state['values']['nodes'] as $nid => $element) {
    if (is_numeric($nid)) {
      $nodes[$form_state['values']['nodes'][$nid]['position']] = array(
        'nid' => $nid,
        'data' => $form_state['values']['nodes'][$nid]
      );
    }
  }

  $message = 'The queue has been updated.';

  $qid  = $form['nodes']['#queue']['qid'];
  simple_queue_save_queue_order($nodes, $qid);

  drupal_set_message(t('@message', array('@message' => $message)));
}

/**
 * Page callback to remove a node from a queue.
 * 
 * @param 
 *   array $queue a queue.
 * 
 * @param 
 *   array $node a node.
 * 
 * @return 
 *   type Description.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_admin_remove_node($queue, $node) {
  if (!simple_queue_check_token($node->nid)) {
    return drupal_goto();
  }

  simple_queue_queue_remove_node($queue->qid, $node->nid);

  // There should always be a destination set for this, so just goto wherever.
  drupal_goto();
}

/**
 * Validates new queue order information and if it passes validation,
 * deletes the old queue data from the database and saves the new data.
 *
 * @param 
 *   array $nodes an array of nodes, keyed on the queue position.
 * 
 * @param 
 *   string $qid the queue id.
 * 
 * @return An array where the first element is a numeric status code
 *   (0 means successfully saved) and the second element is a status message.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_save_queue_order($nodes, $qid) {
  $now = REQUEST_TIME;

  $queue = simple_queue_load($qid);

  // cleanup the node array
  $clean = array();
  $count = 1;
  ksort($nodes);
  drupal_alter('simple_queue_sort', $nodes);
  foreach ($nodes as $pos => $node) {
    if (!is_numeric($node['nid']) || $node['nid'] < 1) {
      return array(SIMPLE_QUEUE_INVALID_NID, 'Invalid nid value. New queue order not saved.');
    }
    if (is_numeric($pos)) {
      $clean[$count] = $node;
      $count++;
    }
    elseif ($pos == 'r') {
      // Remove the node from this queue.
      simple_queue_queue_remove_node($qid, $node['nid']);
    }
    else {
      return array(SIMPLE_QUEUE_INVALID_POSITION, 'Invalid position value. New queue order not saved.');
    }
  }
  $nodes = $clean;

  array_walk($clean, create_function('&$node', '$node = serialize($node);'));

  if (count(array_unique($clean)) < count($nodes)) {
    return array(SIMPLE_QUEUE_DUPLICATE_POSITION, 'Duplicate position values are not allowed. New queue order not saved.');
  }
  // Allow other modules to alter the order of nodes.
  foreach (module_implements('simple_queue_save_queue_order_alter') as $module) {
    $function = $module . '_simple_queue_save_queue_order_alter';
    $function($qid, $nodes);
  }

  // clear the queue and save the new positions
  db_delete('simple_queue_nodes')
    ->condition('qid', $qid)
    ->execute();
  foreach ($nodes as $pos => $node) {

    if ($pos != 'r') {
      db_insert('simple_queue_nodes')
        ->fields(array(
          'qid' => $qid,
          'nid' => $node['nid'],
          'position' => $pos,
          'timestamp' => $now,
          'display' => '',
        ))
        ->execute();
    }
  }

  if ($queue->size) {
    // only necessary if the queue is of finite length
    simple_queue_check_queue_size($queue);
  }

  return array(SIMPLE_QUEUE_OK, 'The queue has been updated.');
}

/**
 * Validate function for the simple_queue_arrange_queue form.
 * 
 * @param 
 *   array $form Allow access to the form.
 * 
 * @param
 *   array $form_state Allow access to the form.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_arrange_queue_form_add_validate($form, &$form_state) {
  // Validate textfield add.
  if (empty($form_state['values']['add']['nid']) || ($form_state['values']['add']['nid'] === t('Enter the title of a node to add it to the queue'))) {
    form_set_error('Node title', t('Please enter a title for the node.'));
  }  
}

/**
 * Submit function for the simple_queue_arrange_queue form.
 * 
 * @param 
 *   array $form Allow access to the form.
 * 
 * @param
 *   array $form_state Allow access to the form.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_arrange_queue_form_add_submit($form, &$form_state) { 
  $queue = simple_queue_load($form['nodes']['#queue']['qid']);
  
  if (!empty($form_state['values']['add']['nid'])) {
    $matches = array();
    preg_match('/\[nid: (\d+)\]$/', $form_state['values']['add']['nid'], $matches);

    $nid = empty($matches) ? $matches : $matches[1];
    if (empty($nid)) {
      form_set_error('', t('Please enter a valid node title.'));
    }
  }

  simple_queue_queue_add($queue, $nid);
}

/**
 * Page callback for autocomplete.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_autocomplete() {
  $args = func_get_args();
  $qid = array_shift($args);
  $string = implode('/', $args);
  $matches = _simple_queue_autocomplete($qid, $string);
  drupal_json_output(drupal_map_assoc($matches));
}

/**
 * Return to the nodes that matches with the key word.
 * 
 * @param 
 *   int $qid id of the queue.
 * 
 * @param 
 *   string $string word to search
 * 
 * @return
 *   array 
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function _simple_queue_autocomplete($qid, $string) {
  $output = array();

  if (!is_numeric($qid) || !$string) {
    return $output;
  }

  $queue = simple_queue_load($qid);
  if (!$queue) {
    return $output;
  }

  $nodes = simple_queue_api_autocomplete($queue, $string);
  return $nodes;
}
