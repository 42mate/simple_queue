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
      case 'direction':
        $sort_primary[] = $queue->direction;
        $sort_direction = $sort_direction_reverse;
        break;
    }
  }

  if (!empty($table_sort)) {
    if (strtolower($table_sort['sort']) == 'desc') {
      // Re-indexes array keys; key no longer equals qid.
      array_multisort($sort_primary, $sort_direction['desc'], $sort_secondary,
          $queues);
    }
    else {
      // Re-indexes array keys; key no longer equals qid.
      array_multisort($sort_primary, $sort_direction['asc'], $sort_secondary,
          $queues);
    }
  }

  $rows = array();
  foreach ($queues as $queue) {
    $operations = array();
    
    $operations[] = l(t('View'), 'admin/structure/simple_queue/' . $queue->qid
        . '/view');
    
    $simple_queue_title = check_plain($queue->title);

    if (simple_queue_validate_permission(SIMPLE_QUEUE_PERMISSION_EDIT_QUEUES)
        || (simple_queue_validate_permission(
            SIMPLE_QUEUE_PERMISSION_EDIT_SAME_ROLES_QUEUES) 
        && simple_queue_validate_queue_same_roles($queue->roles))) {
      $operations[] = l(t('Edit'), 'admin/structure/simple_queue/' . $queue->qid
          . '/edit');
      
      $simple_queue_title = l(check_plain($queue->title),
          'admin/structure/simple_queue/' . $queue->qid . '/edit');
    }

    $rows[] = array(
      array('class' => array('simple_queue-title'), 'data'
        => $simple_queue_title),
      array('class' => array('simple_queue-max-nodes'), 'data'
        => $queue->size == 0 ? t('Infinite') : $queue->size),
      array('class' => array('simple_queue-nodes'), 'data' 
        => empty($queue->nodes) ? 0 : count($queue->nodes)),
      array('class' => array('simple_queue-direction'), 'data' 
        => $queue->direction),
      array('class' => array('simple_queue-operation'), 'data'
        => implode(' | ', $operations)),
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
    drupal_set_title(t("Simple queue '@title'", array('@title'
      => $queue->title)), PASS_THROUGH);
  }
  
  $form['description'] = array(
    '#type' => 'fieldset',
    '#title' => t('Simple queue'),
    '#description' => t('Nodes put into a queue are added to the back of'
        . ' the queue; when a node is added to a full queue, the node in the'
        . ' front of the queue will be popped out to make room.'),
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
    '#description' => t('A unique machine-readable name for this queue.'
        . ' It must only contain lowercase letters, numbers, and underscores.'),
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
    '#description' => t('The maximum number of nodes will appear in the queue.'
        . ' Enter 0 for no limit'),
  );
  
  $form['direction'] = array(
    '#type' => 'checkbox',
    '#title' => t('Direction'),
    '#default_value' => $queue->direction,
    '#description' => t('How the new nodes are going to be added to the queue.'
        . ' 0 (no marked) means from top, 1 (marked) means from the bottom. .'),
  );  

  $form['roles'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Roles'),
    '#required' => TRUE,
    '#default_value' => empty($queue->roles) ? array() : $queue->roles,
    '#options' => user_roles(),
    '#description' => t('Check each role that can add nodes to the queue.'
        . ' Be sure that roles you want to appear here have "manipulate queues"'
        . ' access in the main access control panel.'),
  );
  
  $node_type_names = node_type_get_names();
  
  $form['types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Types'),
    '#required' => TRUE, 
    '#default_value' => empty($queue->types) ? array()
    : simple_queue_get_attribute_from_objects($queue->types, 'type'),
    '#options'
    => $node_type_names, // Returns a list of available node type names.
    '#description'
    => t('Check each node type that can be added to this queue.'),
  );
  
  foreach ($node_type_names as $key => $value) {
    $form[$key] = array(
      '#title' => check_plain(t('Select view mode to default for ' . $value)),
      '#type' => 'select',
      '#options' => simple_queue_get_view_modes_formatted($key),
      '#default_value' => empty($queue->types) ? array()
      : get_default_display_from_types($queue->types, $key),
      '#states' => array(      
        'visible' => array(
          ':input[name="types[' . $key . ']"]' => array('checked' => TRUE),
        ),
      ),
    );
  } 
  
  // In case of edit queue.  
  if (isset($queue->qid)) {
    if ((simple_queue_validate_permission(SIMPLE_QUEUE_PERMISSION_DELETE_QUEUES)
        || (simple_queue_validate_permission(SIMPLE_QUEUE_PERMISSION_DELETE_SAME_ROLES_QUEUES)
        && simple_queue_validate_queue_same_roles($queue->roles)))) {    
      
      $form[] = array(
        '#type' => 'submit',
        '#value' => t('Delete'),
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
  $types_names = array_keys(array_filter($queue->types));
  $queue->types = array();
  
  foreach ($types_names as $value) {
    $type = array();
    $type['type'] = $value;
    $type['default_display'] = $queue->$value;
    array_push($queue->types, $type);
  }

  if (!isset($queue->qid)) {
    $queue->new = TRUE;
  } 

  $qid = simple_queue_save($queue); // sets $queue->qid if needed. 
  
  if (!isset($queue->new)) {    
    if ($queue->size) {
      // only necessary if the queue is of finite length
      $queue->count = db_query("SELECT COUNT(nid) FROM {simple_queue_nodes}"
        . " WHERE qid = :qid", array(':qid' => $queue->qid))->fetchField();
      simple_queue_check_queue_size($queue);
    }    
    
    drupal_set_message(t('The queue has been updated.'));
  } 
  else {
    $form_state['values']['qid'] = $qid;
    drupal_set_message(t('The queue has been created.'));
  }  

  $form_state['redirect'] = 'admin/structure/simple_queue';
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
  $form_state['redirect'] = 'admin/structure/simple_queue/'
      . $form_state['values']['qid'] . '/delete';
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
    t('Are you sure you want to delete "%title"?',
        array('%title' => $queue->title)),
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
  drupal_set_title(t("Simple queue '@title'", array('@title' => $queue->title)),
      PASS_THROUGH);

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

  return drupal_get_form('simple_queue_arrange_queue_form_'
      . $queue->qid, $queue, $nodes);
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
  $visible = simple_queue_nids_visible($queue->qid);

  // Get a list of all nodes in the queue, regardless of access restrictions.
  $query = db_select('node', 'n')
    ->distinct();

  $query->leftJoin('simple_queue_nodes', 'nq', 'nq.nid = n.nid');
  $query->fields('n', array('nid'))
    ->fields('nq')
    ->condition('nq.qid', $queue->qid)
    ->orderBy('nq.position', 'ASC');
  $result = $query->execute();
  
  $nids = array();
  $sq_nodes = array();
  foreach ($result as $sq_node) {
    $nids[] = $sq_node->nid;
    // Save node position in a separate array so we can reference it later.
    $sq_nodes[$sq_node->nid]['position'] = $sq_node->position;
    $sq_nodes[$sq_node->nid]['locked'] = $sq_node->locked;
  }
  $nodes = node_load_multiple($nids);
  
  foreach ($nodes as $node) {
    $node->visible = isset($visible[$node->nid]) ? TRUE : FALSE;
    $node->position = $sq_nodes[$node->nid]['position'];
    $node->locked = $sq_nodes[$node->nid]['locked'];
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
      $form['nodes'][$node->nid]['title'] = array('#markup' => l($node->title,
          'node/' . $node->nid));
      $form['nodes'][$node->nid]['status'] = array(
        '#type' => 'select',
        '#options' => array(
          NODE_NOT_PUBLISHED => t('Not published'),
          NODE_PUBLISHED => t('Published'),         
        ),
        '#default_value' => intval($node->status),
      );
      $form['nodes'][$node->nid]['type'] = array('#markup' => $node->type);
      $form['nodes'][$node->nid]['locked'] = simple_queue_validate_permission(
          SIMPLE_QUEUE_PERMISSION_LOCK_UNLOCK_NODES) ? array(
        '#type' => 'select',
        '#options' => array(
          0,
          1,
        ),
        '#default_value' => intval($node->locked),
      ) : array('#markup' => $node->locked);
      $form['nodes'][$node->nid]['date'] = array('#markup' => 
        format_date($node->created, 'short'));
    }
    else {
      $form['nodes'][$node->nid]['title'] = array('#value' =>
        t('Restricted node, NID: @nid', array('@nid' => $node->nid)));
      $form['nodes'][$node->nid]['status'] = array('#value' => '');
      $form['nodes'][$node->nid]['type'] = array('#value' => '');
      $form['nodes'][$node->nid]['locked'] = array('#value' => '');
      $form['nodes'][$node->nid]['date'] = array('#value' => '');
    }

    if (node_access('update', $node)) {
      $form['nodes'][$node->nid]['edit'] = array('#markup' =>
        l(t('edit'), 'node/' . $node->nid . '/edit', array('attributes'
          => array('title' => t('Edit this node')))));
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

    $form['nodes'][$node->nid]['remove'] = array('#markup' => l(t('remove'),
        'admin/structure/simple_queue/' . $queue->qid . '/remove-node/'
        . $node->nid, $attr));
  }

  // Validate permission to add node to the queue.
  if ((simple_queue_validate_permission(SIMPLE_QUEUE_PERMISSION_ADD_NODES_QUEUES)
        || (simple_queue_validate_permission(
            SIMPLE_QUEUE_PERMISSION_ADD_NODES_SAME_ROLES_QUEUES)
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
  $form['actions']['fill'] = array(
    '#type' => 'submit',
    '#value' => t('Fill'),
    '#submit' => array('simple_queue_arrange_queue_form_fill_submit'),
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
 * Submit handler for simple_queue drag'n'drop form. Updates node positions
 *  in {simple_queue_nodes}.
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

  $qid  = $form['nodes']['#queue']['qid'];
  $message = simple_queue_save_queue_order($nodes, $qid);

  drupal_set_message(t('@message', array('@message' => $message)));
}

/**
 * Submit handler for simple_queue Add an action button to fill with the latest 
 *  published nodes of the allowed content types for the queue.
 * 
 * 
 * If the queue supports 10 items, and we have 5 items, this action will fill
 *  the queue with 5 more nodes, these last five will be firsts
 *  (or last depending on direction).
 * 
 * Avoid node repetitions.
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
function simple_queue_arrange_queue_form_fill_submit($form, &$form_state) {
  $queue  = json_decode(json_encode($form['nodes']['#queue']), FALSE);
  $message = 'The queue has been fill successfuly.';
  $type = 'status';

  if ($queue->size == 0 || $queue->size > count($queue->nodes)) {
    simple_queue_fill($queue);
  } 
  else {
    $message = 'The queue is full.';
    $type = 'warning';
  }
  drupal_set_message(t('@message', array('@message' => $message)), $type);
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
 * Page callback to add a node to a queue.
 * 
 *  @param 
 *   array $queue a queue.
 * 
 * @param 
 *   array $node a node.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_admin_add_node($queue, $node) {  
  if ((simple_queue_validate_permission(SIMPLE_QUEUE_PERMISSION_ADD_NODES_QUEUES)
        || (simple_queue_validate_permission(
            SIMPLE_QUEUE_PERMISSION_ADD_NODES_SAME_ROLES_QUEUES)
        && simple_queue_validate_queue_same_roles($queue->roles)))) {    
    
    if (!simple_queue_check_token($node->nid)) {
    return drupal_goto();
  }
  simple_queue_queue_add($queue, $node->nid);

  // There should always be a destination set for this, so just goto wherever.
  drupal_goto();
  }  
}

/**
 * Display the queue controls for a node.
 *
 * @param 
 *   $node The loaded $node; will be loaded by the hook_menu.
 * 
 * @return
 *  array.
 * 
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */
function simple_queue_node_tab($node) {
  $output = '';

  $queues = simple_queue_load_queues_by_type($node->type);

  if (!$queues) {
    return FALSE;
  }
  $header = array();
  $header[] = array('data' => t('Queue'), 'field' => 'title', 'sort'
    => 'asc', 'class' => array('simple_queue-title'));
  $header[] = array('data' => t('Max nodes'), 'field' => 'size', 'class'
    => array('simple_queue-max-nodes'));
  $header[] = array('data' => t('In queue'), 'field' => 'count', 'class'
    => array('simple_queue-in-queue'));
  $header[] = array('data' => t('Operation'), 'class'
    => array('simple_queue-operation'));
  $table_sort = tablesort_init($header);

  $sort_primary = array();
  $sort_secondary = array();
  $sort_direction_regular = array('asc' => SORT_ASC, 'desc' => SORT_DESC);
  $sort_direction_reverse = array('asc' => SORT_DESC, 'desc' => SORT_ASC);
  
  foreach ($queues as $queue) {
    $sort_secondary[] = $queue->title;
    switch ($table_sort['sql']) {
      case 'title':
      default:
        $sort_primary[] = $queue->title;
        $sort_direction = $sort_direction_regular;
        break;
      case 'size':
        $sort_primary[] = $queue->size;
        $sort_direction = $sort_direction_reverse;
        break;
      case 'count':
        $sort_primary[] = count($queue->nodes);
        $sort_direction = $sort_direction_reverse;
        break;
    }
  }

  if (!empty($table_sort)) {
    if (strtolower($table_sort['sort']) == 'desc') {
      array_multisort($sort_primary, $sort_direction['desc'], $sort_secondary,
          $queues); // Re-indexes array keys; key no longer equals qid.
    }
    else {
      array_multisort($sort_primary, $sort_direction['asc'], $sort_secondary, 
          $queues); // Re-indexes array keys; key no longer equals qid.
    }
  }

  $rows = array();
  foreach ($queues as $queue) {    
    if (verify_node_exist_queue($queue, $node)) {    
      $op = l(
        t('Remove from queue'),
        "admin/structure/simple_queue/$queue->qid/remove-node/$node->nid",
        array('attributes' => array('class'
          => array('simple_queue-ajax-toggle')),
          'query' => simple_queue_get_query_string($node->nid, TRUE,
              array('tab' => '1')),
          'purl' => array('disabled' => TRUE))
      );
    }
    else {      
      if ((simple_queue_validate_permission(SIMPLE_QUEUE_PERMISSION_ADD_NODES_QUEUES)
        || (simple_queue_validate_permission(
            SIMPLE_QUEUE_PERMISSION_ADD_NODES_SAME_ROLES_QUEUES)
        && simple_queue_validate_queue_same_roles($queue->roles)))) {   
        
        $op = l(
          t('Add to queue'),
          "admin/structure/simple_queue/$queue->qid/add-node/$node->nid",
          array('attributes' => array('class'
            => array('simple_queue-ajax-toggle')),
            'query' => simple_queue_get_query_string($node->nid, TRUE,
                array('tab' => '1')),
            'purl' => array('disabled' => TRUE))
        );
      } 
      else {
        $op = NULL;
      }     
    }
    $row = array();
    $row[] = array(
      'class' => array('simple_queue-title'),
      'data' => l(simple_queue_title_substitute($queue->title, $queue),
          "admin/structure/simple_queue/$queue->qid/view"),
    );
    $row[] = array(
      'class' => array('simple_queue-max-nodes'),
      'data' => $queue->size ? $queue->size : t('Infinite')
    );
    $count = property_exists($queue, 'nodes') ? count($queue->nodes) : 0;
    $row[] = array(
      'id' => 'simple_queue-count-' . $queue->qid,
      'class' => array('simple_queue-in-queue'),
      'data' => simple_queue_queue_size_text($queue->size, $count, FALSE)
    );
    $row[] = array('class' => array('simple_queue-operation'), 'data' => $op);
    $rows[] = $row;
  }

  $output .= theme('table', array('header' => $header, 'rows' => $rows,
    'attributes' => array('class' => array('simple_queue-table'))));
  drupal_add_js(drupal_get_path('module', 'simple_queue') . '/simple_queue.js');
  drupal_add_css(drupal_get_path('module',
      'simple_queue') . '/simple_queue.css');
  return $output;
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
  
  //drupal_alter('simple_queue_sort', $nodes);
  foreach ($nodes as $pos => $node) {
    if (!is_numeric($node['nid']) || $node['nid'] < 1) {
      return 'Invalid nid value. New queue order not saved.';
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
      return 'Invalid position value. New queue order not saved.';
    }
  }
  $nodes = $clean;

  array_walk($clean, create_function('&$node', '$node = serialize($node);'));

  if (count(array_unique($clean)) < count($nodes)) {
    return 'Duplicate position values are not allowed.'
    . ' New queue order not saved.';
  }

  // clear the queue and save the new positions
  db_delete('simple_queue_nodes')
    ->condition('qid', $qid)
    ->execute();
  foreach ($nodes as $pos => $node) {    
    if ($pos != 'r') {
      simple_queue_node_custom_update($node['nid'],
          'status', $node['data']['status']);
      
      db_insert('simple_queue_nodes')
        ->fields(array(
          'qid' => $qid,
          'nid' => $node['nid'],          
          'position' => $pos,
          'timestamp' => $now,
          'display' => '',
          'locked' => $node['data']['locked'],
        ))
        ->execute();
    }
  }  

  if ($queue->size) {
    // only necessary if the queue is of finite length    
    $queue->count = count($queue->nodes);
    simple_queue_check_queue_size($queue);
  }

  return 'The queue has been updated.';
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
  if (empty($form_state['values']['add']['nid']) ||
      ($form_state['values']['add']['nid'] === 
      t('Enter the title of a node to add it to the queue'))) {
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
    preg_match('/\[nid: (\d+)\]$/', $form_state['values']['add']['nid'],
        $matches);

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