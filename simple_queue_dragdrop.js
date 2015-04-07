/**
 * @file
 *   name: simple_queue_dragdrop.js.
 * 
 * target:
 *   functionality to drag and drop on node table.
 *   
 * @author  
 *   Daniel Anzawa daniel@42mate.com.
 */

var Drupal = Drupal || { 'settings': {}, 'behaviors': {}, 'locale': {} };

// Allow other JavaScript libraries to use $.
jQuery.noConflict();

(function ($) {

Drupal.behaviors.simple_queueDrag = {
  attach: function(context) {
    $('.simple_queue-dragdrop').each(function() {
      var table_id = $(this).attr('id');
      var tableDrag = Drupal.tableDrag[table_id];
      
      tableDrag.onDrop = function() {
        $('#' + table_id + ' td.position').each(function(i) {
          $(this).html(i + 1);
        });

        simple_queueUpdateNodePositions(table_id);
      };
    });
  }
};

Drupal.behaviors.simple_queueReverse = {
  attach: function(context) {
    $('#edit-actions-reverse').click(function() {
      var $table = $(this).parent().parent().find('.simple_queue-dragdrop:first');
      var table_id = $table.attr('id');

      // Reverse table rows.
      $table.find('tr.draggable').each(function(i) {
        $table.find('tbody').prepend(this);
      });

      simple_queueUpdateNodePositions(table_id);
      simple_queueInsertChangedWarning(table_id);
      simple_queueRestripeTable(table_id);

      return false;
    });
  }
};

Drupal.behaviors.simple_queueShuffle = {
  attach: function(context) {
    $('#edit-actions-shuffle').click(function() {
      var $table = $(this).parent().parent().find('.simple_queue-dragdrop:first');
      var table_id = $table.attr('id');

      // Randomize table rows.
      var rows = $('#' + table_id + ' tbody tr:not(:hidden)').get();
      rows.sort(function(){return (Math.round(Math.random())-0.5);});
      $.each(rows, function(i, row) {
        $('#' + table_id + ' tbody').prepend(this);
      });

      simple_queueUpdateNodePositions(table_id);
      simple_queueInsertChangedWarning(table_id);
      simple_queueRestripeTable(table_id);

      return false;
    });
  }
};

Drupal.behaviors.simple_queueClear = {
  attach: function(context) {
    $('#edit-actions-clear').click(function() {
      var $table = $(this).parent().parent().find('.simple_queue-dragdrop:first');
      var table_id = $table.attr('id');

      // Mark nodes for removal.
      $('#' + table_id + ' .node-position').each(function(i) {
        $(this).val('r');
      });

      // Remove table rows.
      rows = $('#' + table_id + ' tbody tr:not(:hidden)').hide();

      simple_queuePrependEmptyMessage(table_id);
      simple_queueInsertChangedWarning(table_id);

      return false;
    });
  }
};

Drupal.behaviors.simple_queueRemoveNode = {
  attach: function(context) {
    $('a.simple_queue-remove').css('display', 'block');
    $('a.simple_queue-remove').click(function() {
      a = $(this).attr('id');
      a = '#' + a.replace('simple_queue-remove-', 'edit-') + '-position';
      $(a).val('r');

      // Hide the current row.
      $(this).parent().parent().fadeOut('fast', function() {
        var $table = $(this).parent().parent();
        var table_id = $table.attr('id');

        if ($('#' + table_id + ' tbody tr:not(:hidden)').size() === 0) {
          simple_queuePrependEmptyMessage(table_id);
        }
        else {
          simple_queueRestripeTable(table_id);
          simple_queueInsertChangedWarning(table_id);
        }
      });

      return false;
    });
  }
};

Drupal.behaviors.simple_queueClearTitle = {
  attach: function(context) {
    $('.queue-add-nid').focus(function() {
      if (this.value === this.defaultValue) {
        this.value = '';
        $(this).css('color', '#000');
      }
    }).blur(function() {
      if (!this.value.length) {
        this.value = this.defaultValue;
        $(this).css('color', '#999');
      }
    });
  }
};

/**
 * Updates node positions after simple_queue has been rearranged.
 * It cares about the reverse order and populates nodes the other way round.
 * 
 * @param 
 *   table_id 
 */
function simple_queueUpdateNodePositions(table_id) {
  // Check if reverse option is set.
  var reverse = Drupal.settings.simple_queue.reverse[table_id.replace(/-/g, '_')];
  var size = reverse ? $('#' + table_id + ' .node-position').size() : 1;

  $('#' + table_id + ' tr').filter(":visible").find('.node-position').each(function(i) {
    $(this).val(size);
    reverse ? size-- : size++;
  });
};

/**
 * Restripe the simple_queue table after removing an element or changing the
 * order of the elements.
 * 
 * @param 
 *   table_id 
 */
function simple_queueRestripeTable(table_id) {
  $('#' + table_id + ' tbody tr:not(:hidden)')
  .filter(':odd')
    .removeClass('odd').addClass('even')
      .end()
  .filter(':even')
    .removeClass('even').addClass('odd')
      .end();

  $('#' + table_id + ' tr:visible td.position').each(function(i) {
    $(this).html(i + 1);
  });
};

/**
 * Add a row to the simple_queue table explaining that the queue is empty.
 * 
 * @param 
 *   table_id 
 */
function simple_queuePrependEmptyMessage(table_id) {
  $('#' + table_id + ' tbody').prepend('<tr class="odd"><td colspan="6">'+Drupal.t('No nodes in this queue.')+'</td></tr>');
};

/**
 * Display a warning reminding the user to save the simple_queue.
 * 
 * @param 
 *   table_id 
 */
function simple_queueInsertChangedWarning(table_id) {
  if (Drupal.tableDrag[table_id].changed === false) {
    $(Drupal.theme('tableDragChangedWarning')).insertAfter('#' + table_id).hide().fadeIn('slow');
    Drupal.tableDrag[table_id].changed = true;
  }
};

})(jQuery);