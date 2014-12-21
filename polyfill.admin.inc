<?php

/**
 * @file
 * Administration pages for image settings.
 */

/**
 * Menu callback; Listing of all current image styles.
 */
function polyfill_list() {
  $page = array();

  $sets = polyfill_sets();
  $page['polyfill_list'] = array(
    '#markup' => theme('polyfill_list', array('sets' => $sets)),
  );

  return $page;

}




/**
 * ******************************************************************
 */


/**
 * Form builder; Edit an image style name and effects order.
 *
 * @param $form_state
 *   An associative array containing the current state of the form.
 * @param $style
 *   An image style array.
 * @ingroup forms
 * @see image_style_form_submit()
 */
function polyfill_form($form, &$form_state, $set) {
  $title = t('Edit %name set', array('%name' => $set['label']));
  drupal_set_title($title, PASS_THROUGH);

  $form_state['polyfill_set'] = $set;
  $form['#tree'] = TRUE;

  // Show the Image Style label.
  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Polyfill set name'),
    '#default_value' => $set['label'],
    '#required' => TRUE,
  );

  // Build the list of existing image styles for this polyfill set.
  $form['styles'] = array(
    '#theme' => 'polyfill_styles',
  );
  foreach ($set['styles'] as $key => $style) {
    $form['styles'][$key]['label'] = array(
      '#markup' => $style['label'],
    );

    $form['styles'][$key]['remove'] = array(
      '#type' => 'link',
      '#title' => t('delete'),
      '#href' => 'admin/config/media/polyfill/edit/' . $set['pid'] . '/styles/' . $style['isid'] . '/delete',
    );
  }

  // Build the new image effect addition form and add it to the effect list.
  $new_style_options = array();
  foreach (image_styles() as $style => $definition) {
    $new_style_options[$style] = check_plain($definition['label']);
  }
  $form['styles']['new'] = array(
    '#tree' => FALSE,
  );
  $form['styles']['new']['new'] = array(
    '#type' => 'select',
    '#title' => t('Effect'),
    '#title_display' => 'invisible',
    '#options' => $new_effect_options,
    '#empty_option' => t('Select a new effect'),
  );
  $form['effects']['new']['weight'] = array(
    '#type' => 'weight',
    '#title' => t('Weight for new effect'),
    '#title_display' => 'invisible',
    '#default_value' => count($form['effects']) - 1,
  );
  $form['effects']['new']['add'] = array(
    '#type' => 'submit',
    '#value' => t('Add'),
    '#validate' => array('image_style_form_add_validate'),
    '#submit' => array('image_style_form_submit', 'image_style_form_add_submit'),
  );

  // Show the Override or Submit button for this style.
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['override'] = array(
    '#type' => 'submit',
    '#value' => t('Override defaults'),
    '#validate' => array(),
    '#submit' => array('image_style_form_override_submit'),
    '#access' => !$editable,
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update style'),
    '#access' => $editable,
  );

  return $form;
}

/**
 * Validate handler for adding a new image effect to an image style.
 */
function polyfill_set_form_add_validate($form, &$form_state) {
  if (!$form_state['values']['new']) {
    form_error($form['effects']['new']['new'], t('Select an effect to add.'));
  }
}

/**
 * Submit handler for adding a new image effect to an image style.
 */
function polyfill_set_form_add_submit($form, &$form_state) {
  $style = $form_state['image_style'];
  // Check if this field has any configuration options.
  $effect = image_effect_definition_load($form_state['values']['new']);

  // Load the configuration form for this option.
  if (isset($effect['form callback'])) {
    $path = 'admin/config/media/image-styles/edit/' . $form_state['image_style']['name'] . '/add/' . $form_state['values']['new'];
    $form_state['redirect'] = array($path, array('query' => array('weight' => $form_state['values']['weight'])));
  }
  // If there's no form, immediately add the image effect.
  else {
    $effect['isid'] = $style['isid'];
    $effect['weight'] = $form_state['values']['weight'];
    image_effect_save($effect);
    drupal_set_message(t('The image effect was successfully applied.'));
  }
}

/**
 * Submit handler for saving an image style.
 */
function polyfill_form_submit($form, &$form_state) {
  // Update the image style.
  $style = $form_state['image_style'];
  $style['name'] = $form_state['values']['name'];
  $style['label'] = $form_state['values']['label'];

  // Update image effect weights.
  if (!empty($form_state['values']['effects'])) {
    foreach ($form_state['values']['effects'] as $ieid => $effect_data) {
      if (isset($style['effects'][$ieid])) {
        $effect = $style['effects'][$ieid];
        $effect['weight'] = $effect_data['weight'];
        image_effect_save($effect);
      }
    }
  }

  image_style_save($style);
  if ($form_state['values']['op'] == t('Update style')) {
    drupal_set_message(t('Changes to the style have been saved.'));
  }
  $form_state['redirect'] = 'admin/config/media/image-styles/edit/' . $style['name'];
}

/**
 * Form builder; Form for adding a new image style.
 *
 * @ingroup forms
 * @see image_style_add_form_submit()
 */
function polyfill_add_form($form, &$form_state) {
  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Set name'),
    '#default_value' => '',
    '#required' => TRUE,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Create new style'),
  );

  return $form;
}

/**
 * Submit handler for adding a new image style.
 */
function polyfill_add_form_submit($form, &$form_state) {
  $set = array(
    'label' => $form_state['values']['label'],
    'data' => array(),
  );
  $set = polyfill_set_save($set);
  drupal_set_message(t('Set %name was created.', array('%name' => $set['label'])));
  $form_state['redirect'] = 'admin/config/media/polyfill/edit/' . $set['pid'];
}


/**
 * Form builder; Form for deleting an image style.
 *
 * @param $style
 *   An image style array.
 *
 * @ingroup forms
 * @see image_style_delete_form_submit()
 */
function polyfill_delete_form($form, &$form_state, $set) {
  $form_state['set'] = $set;

  $message = t('Are you sure you want to delete polyfill set %name?', array('%name' => $set['label']));
  $caption = '<p>' . t('This action cannot be undone.') . '</p>';

  return confirm_form($form, $message, 'admin/config/media/polyfill', $caption, t('Delete'));

}

/**
 * Submit handler to delete an image style.
 */
function polyfill_delete_form_submit($form, &$form_state) {
  $set = $form_state['set'];

  polyfill_set_delete($set);
  drupal_set_message(t('Set %name was deleted.', array('%name' => $set['label'])));
  $form_state['redirect'] = 'admin/config/media/polyfill';
}


/**
 * Form builder; Form for adding and editing image effects.
 *
 * This form is used universally for editing all image effects. Each effect adds
 * its own custom section to the form by calling the form function specified in
 * hook_image_effects().
 *
 * @param $form_state
 *   An associative array containing the current state of the form.
 * @param $style
 *   An image style array.
 * @param $effect
 *   An image effect array.
 *
 * @ingroup forms
 * @see hook_image_effects()
 * @see image_effects()
 * @see image_resize_form()
 * @see image_scale_form()
 * @see image_rotate_form()
 * @see image_crop_form()
 * @see image_effect_form_submit()
 */
function polyfill_style_form($form, &$form_state, $style, $effect) {
  if (!empty($effect['data'])) {
    $title = t('Edit %label effect', array('%label' => $effect['label']));
  }
  else {
    $title = t('Add %label effect', array('%label' => $effect['label']));
  }
  drupal_set_title($title, PASS_THROUGH);

  $form_state['image_style'] = $style;
  $form_state['image_effect'] = $effect;

  // If no configuration for this image effect, return to the image style page.
  if (!isset($effect['form callback'])) {
    drupal_goto('admin/config/media/image-styles/edit/' . $style['name']);
  }

  $form['#tree'] = TRUE;
  $form['#attached']['css'][drupal_get_path('module', 'image') . '/image.admin.css'] = array();
  if (function_exists($effect['form callback'])) {
    $form['data'] = call_user_func($effect['form callback'], $effect['data']);
  }

  // Check the URL for a weight, then the image effect, otherwise use default.
  $form['weight'] = array(
    '#type' => 'hidden',
    '#value' => isset($_GET['weight']) ? intval($_GET['weight']) : (isset($effect['weight']) ? $effect['weight'] : count($style['effects'])),
  );

  $form['actions'] = array('#tree' => FALSE, '#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => isset($effect['ieid']) ? t('Update effect') : t('Add effect'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => 'admin/config/media/image-styles/edit/' . $style['name'],
  );

  return $form;
}

/**
 * Submit handler for updating an image effect.
 */
function polyfill_style_form_submit($form, &$form_state) {
  $style = $form_state['image_style'];
  $effect = array_merge($form_state['image_effect'], $form_state['values']);
  $effect['isid'] = $style['isid'];
  image_effect_save($effect);
  drupal_set_message(t('The image effect was successfully applied.'));
  $form_state['redirect'] = 'admin/config/media/image-styles/edit/' . $style['name'];
}

/**
 * Form builder; Form for deleting an image effect.
 *
 * @param $style
 *   Name of the image style from which the image effect will be removed.
 * @param $effect
 *   Name of the image effect to remove.
 * @ingroup forms
 * @see image_effect_delete_form_submit()
 */
function polyfill_style_delete_form($form, &$form_state, $style, $effect) {
  $form_state['image_style'] = $style;
  $form_state['image_effect'] = $effect;

  $question = t('Are you sure you want to delete the @effect effect from the %style style?', array('%style' => $style['label'], '@effect' => $effect['label']));
  return confirm_form($form, $question, 'admin/config/media/image-styles/edit/' . $style['name'], '', t('Delete'));
}

/**
 * Submit handler to delete an image effect.
 */
function polyfill_style_delete_form_submit($form, &$form_state) {
  $style = $form_state['image_style'];
  $effect = $form_state['image_effect'];

  image_effect_delete($effect);
  drupal_set_message(t('The image effect %name has been deleted.', array('%name' => $effect['label'])));
  $form_state['redirect'] = 'admin/config/media/image-styles/edit/' . $style['name'];
}

/**
 * Element validate handler to ensure an integer pixel value.
 *
 * The property #allow_negative = TRUE may be set to allow negative integers.
 */
function polyfill_style_integer_validate($element, &$form_state) {
  $value = empty($element['#allow_negative']) ? $element['#value'] : preg_replace('/^-/', '', $element['#value']);
  if ($element['#value'] != '' && (!is_numeric($value) || intval($value) <= 0)) {
    if (empty($element['#allow_negative'])) {
      form_error($element, t('!name must be an integer.', array('!name' => $element['#title'])));
    }
    else {
      form_error($element, t('!name must be a positive integer.', array('!name' => $element['#title'])));
    }
  }
}


/**
 * Form structure for the image resize form.
 *
 * Note that this is not a complete form, it only contains the portion of the
 * form for configuring the resize options. Therefore it does not not need to
 * include metadata about the effect, nor a submit button.
 *
 * @param $data
 *   The current configuration for this resize effect.
 */
function polyfill_resize_form($data) {
  $form['width'] = array(
    '#type' => 'textfield',
    '#title' => t('Width'),
    '#default_value' => isset($data['width']) ? $data['width'] : '',
    '#field_suffix' => ' ' . t('pixels'),
    '#required' => TRUE,
    '#size' => 10,
    '#element_validate' => array('image_effect_integer_validate'),
    '#allow_negative' => FALSE,
  );
  $form['height'] = array(
    '#type' => 'textfield',
    '#title' => t('Height'),
    '#default_value' => isset($data['height']) ? $data['height'] : '',
    '#field_suffix' => ' ' . t('pixels'),
    '#required' => TRUE,
    '#size' => 10,
    '#element_validate' => array('image_effect_integer_validate'),
    '#allow_negative' => FALSE,
  );
  return $form;
}


/**
 * Returns HTML for the page containing the list of image styles.
 *
 * @param $variables
 *   An associative array containing:
 *   - styles: An array of all the image styles returned by image_get_styles().
 *
 * @see image_get_styles()
 * @ingroup themeable
 */
function theme_polyfill_list($variables) {
  $sets = $variables['sets'];

  $header = array(t('Set name'), array('data' => t('Operations'), 'colspan' => 2));
  $rows = array();
  foreach ($sets as $set) {
    $row = array();
    $row[] = l($set['label'], 'admin/config/media/polyfill/' . $set['pid'] . '/edit');
    $link_attributes = array(
      'attributes' => array(
        'class' => array('polyfill-set-link'),
      ),
    );
    $row[] = l(t('edit'), 'admin/config/media/polyfill/' . $set['pid'] . '/edit', $link_attributes);
    $row[] = l(t('delete'), 'admin/config/media/polyfill/' . $set['pid'] . '/delete', $link_attributes);
    $rows[] = $row;
  }

  if (empty($rows)) {
    $rows[] = array(array(
      'colspan' => 3,
      'data' => t('There are currently no sets. <a href="!url">Add a new one</a>.', array('!url' => url('admin/config/media/polyfill/add'))),
    ));
  }

  return theme('table', array('header' => $header, 'rows' => $rows));
}

/**
 * Returns HTML for a listing of the effects within a specific image style.
 *
 * @param $variables
 *   An associative array containing:
 *   - form: A render element representing the form.
 *
 * @ingroup themeable
 */
function theme_polyfill_styles($variables) {
  $form = $variables['form'];

  $rows = array();

  foreach (element_children($form) as $key) {
    $row = array();
    $form[$key]['weight']['#attributes']['class'] = array('image-effect-order-weight');
    if (is_numeric($key)) {
      $summary = drupal_render($form[$key]['summary']);
      $row[] = drupal_render($form[$key]['label']) . (empty($summary) ? '' : ' ' . $summary);
      $row[] = drupal_render($form[$key]['weight']);
      $row[] = drupal_render($form[$key]['configure']);
      $row[] = drupal_render($form[$key]['remove']);
    }
    else {
      // Add the row for adding a new image effect.
      $row[] = '<div class="image-style-new">' . drupal_render($form['new']['new']) . drupal_render($form['new']['add']) . '</div>';
      $row[] = drupal_render($form['new']['weight']);
      $row[] = array('data' => '', 'colspan' => 2);
    }

    if (!isset($form[$key]['#access']) || $form[$key]['#access']) {
      $rows[] = array(
        'data' => $row,
        'class' => !empty($form[$key]['weight']['#access']) || $key == 'new' ? array('draggable') : array(),
      );
    }
  }

  $header = array(
    t('Effect'),
    t('Weight'),
    array('data' => t('Operations'), 'colspan' => 2),
  );

  if (count($rows) == 1 && $form['new']['#access']) {
    array_unshift($rows, array(array(
      'data' => t('There are currently no effects in this style. Add one by selecting an option below.'),
      'colspan' => 4,
    )));
  }

  $output = theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'image-style-effects')));
  drupal_add_tabledrag('image-style-effects', 'order', 'sibling', 'image-effect-order-weight');
  return $output;
}

