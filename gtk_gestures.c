#include "all.h"

typedef struct {
  GtkGesture *drag, *rotate, *zoom;
  GtkWidget *show_touches_switch, *bounding_box_switch, *rotate_switch, *zoom_switch, *drag_switch;
  GHashTable *touches;
} Data;

typedef struct {
  double x, y;
} Touch;

void on_touch(GtkWidget     *w,
              GdkEventTouch *ev,
              Data          *data)
{
  switch (ev->type) {
    case GDK_TOUCH_BEGIN: {
      if (!g_hash_table_contains(data->touches, ev->sequence)) {
        Touch *t = g_new(Touch, 1);
        t->x = ev->x;
        t->y = ev->y;
        g_hash_table_insert(data->touches, ev->sequence, t);
      }
      break;
    }

    case GDK_TOUCH_UPDATE: {
      Touch *t = g_hash_table_lookup(data->touches, ev->sequence);
      t->x = ev->x;
      t->y = ev->y;
      break;
    }

    case GDK_TOUCH_END: {
      g_hash_table_remove(data->touches, ev->sequence);
      break;
    }

    default: {
      puts("received a touch event (not begin, update, or end)");
      break;
    }
  }
  gtk_widget_queue_draw(w); // drawing area
}

gboolean draw(GtkWidget *widget,
              cairo_t   *cr,
              Data      *data)
{
  // drag gesture
  if (gtk_switch_get_state(GTK_SWITCH(data->drag_switch)) &&
      gtk_gesture_is_recognized(data->drag)) {
    double x, y;
    gtk_gesture_drag_get_start_point(GTK_GESTURE_DRAG(data->drag), &x, &y);
    cairo_move_to(cr, x, y);

    double dx, dy;
    gtk_gesture_drag_get_offset(GTK_GESTURE_DRAG(data->drag), &dx, &dy);
    cairo_line_to(cr, x + dx, y + dy);

    cairo_set_source_rgb(cr, 1, 0.5, 0);
    cairo_set_line_width(cr, 4);
    cairo_stroke(cr);
  }
  // zoom and rotate gestures
  bool draw_zoom = gtk_switch_get_state(GTK_SWITCH(data->zoom_switch)) &&
                   gtk_gesture_is_recognized(data->zoom);
  bool draw_rotate = gtk_switch_get_state(GTK_SWITCH(data->rotate_switch)) &&
                     gtk_gesture_is_recognized(data->rotate);

  double scale = draw_zoom ? gtk_gesture_zoom_get_scale_delta(GTK_GESTURE_ZOOM(data->zoom)) : 1;
  double rotation = draw_rotate ? gtk_gesture_rotate_get_angle_delta(GTK_GESTURE_ROTATE(data->rotate)) : 1;

  if (draw_zoom || draw_rotate) {
    double x, y;
    gtk_gesture_get_bounding_box_center(data->rotate, &x, &y);
    cairo_matrix_t matrix;
    cairo_get_matrix(cr, &matrix);
    cairo_matrix_translate(&matrix, x, y);
    cairo_matrix_rotate(&matrix, rotation);
    cairo_matrix_scale(&matrix, scale, scale);

    cairo_save(cr); // only use zoom/rotate matrix for the square
    {
      cairo_set_matrix(cr, &matrix);
      cairo_set_source_rgb(cr, 1, 0, 0.5);
      const int side = 200;
      cairo_rectangle(cr, -side/2, -side/2, side, side);
      cairo_fill(cr);
    }
    cairo_restore(cr);
  }

  // touches and bounding box
  bool draw_touches      = gtk_switch_get_state(GTK_SWITCH(data->show_touches_switch));
  bool draw_bounding_box = gtk_switch_get_state(GTK_SWITCH(data->bounding_box_switch)) &&
                           g_hash_table_size(data->touches) > 1;

  if (draw_touches || draw_bounding_box) {
    double min_x = gtk_widget_get_allocated_width(widget) + 1, // top left
           min_y = gtk_widget_get_allocated_height(widget) + 1,
           max_x = -1, // bottom right
           max_y = -1;

    const int circle_radius = 25;
    g_autoptr(GList) touch_list = g_hash_table_get_values(data->touches);
    for (GList *elem = touch_list; elem != NULL; elem = elem->next) {
      Touch *t = elem->data;
      if (draw_touches) {
        cairo_arc(cr, t->x, t->y, circle_radius, 0, 2*M_PI);
        cairo_fill(cr);
      }

      if (draw_bounding_box) {
        min_x = MIN(min_x, t->x); // top left
        min_y = MIN(min_y, t->y);
        max_x = MAX(max_x, t->x); // bottom right
        max_y = MAX(max_y, t->y);
      }
    }

    if (draw_bounding_box) {
      cairo_set_source_rgb(cr, 0, 1, 0);
      cairo_rectangle(cr, min_x-circle_radius, min_y-circle_radius, max_x-min_x+circle_radius*2, max_y-min_y+circle_radius*2);
      cairo_set_line_width(cr, 4);
      cairo_stroke(cr);
    }
  }

  return GDK_EVENT_PROPAGATE;
}

int main(int argc, char *argv[])
{
  gtk_init(&argc, &argv);

  g_autoptr(GtkBuilder) builder = gtk_builder_new_from_resource("/ui.glade");
  gtk_builder_connect_signals(builder, NULL);

#define get(widget) GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(builder, #widget));
  get(window);
  get(drawing_area);
#undef get
#define get(widget) GtkWidget *widget ## _switch = GTK_WIDGET(gtk_builder_get_object(builder, #widget));
  get(show_touches);
  get(bounding_box);
  get(rotate);
  get(zoom);
  get(drag);
#undef get

  g_autoptr(GHashTable) touches = g_hash_table_new_full(NULL, NULL, NULL, &free);

  g_autoptr(GtkGesture) drag = gtk_gesture_drag_new(drawing_area);
  g_autoptr(GtkGesture) rotate = gtk_gesture_rotate_new(drawing_area);
  g_autoptr(GtkGesture) zoom = gtk_gesture_zoom_new(drawing_area);

#define get(x) .x = x
  Data user_data = {
    get(drag), get(rotate), get(zoom),
    get(show_touches_switch), get(bounding_box_switch), get(rotate_switch), get(zoom_switch), get(drag_switch),
    get(touches)
  };
#undef get

  g_signal_connect(drawing_area, "touch-event", G_CALLBACK(on_touch), &user_data);
  g_signal_connect(drawing_area, "draw",        G_CALLBACK(draw),     &user_data);

  g_signal_connect_swapped(drag,   "drag-update",   G_CALLBACK(gtk_widget_queue_draw), drawing_area);
  g_signal_connect_swapped(rotate, "angle-changed", G_CALLBACK(gtk_widget_queue_draw), drawing_area);
  g_signal_connect_swapped(zoom,   "scale-changed", G_CALLBACK(gtk_widget_queue_draw), drawing_area);

  gtk_window_maximize(GTK_WINDOW(window));
  gtk_widget_show_all(window);

  gtk_main();
}
