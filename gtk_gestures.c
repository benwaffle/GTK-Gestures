static GtkGesture *drag, *rotate, *zoom;
static GtkWidget *show_touches_switch,
                 *bounding_box_switch,
                 *rotate_switch,
                 *zoom_switch,
                 *drag_switch;

static GHashTable *touches = NULL;
typedef struct { double x, y; } Touch;

static void on_touch(GtkWidget      *w,
                     GdkEventTouch  *ev,
                     GtkDrawingArea *draw_area)
{
    switch (ev->type)
    {
        case GDK_TOUCH_BEGIN:
        {
            if (!g_hash_table_contains(touches, ev->sequence))
            {
                Touch *t = g_new(Touch, 1);
                t->x = ev->x;
                t->y = ev->y;
                g_hash_table_insert(touches, ev->sequence, t);
            }
            break;
        }

        case GDK_TOUCH_UPDATE:
        {
            Touch *t = g_hash_table_lookup(touches, ev->sequence);
            t->x = ev->x;
            t->y = ev->y;
            break;
        }

        case GDK_TOUCH_END:
        {
            g_hash_table_remove(touches, ev->sequence);
            break;
        }

        default:
        {
            puts("\tgdk_touch_cancel");
            break;
        }
    }
    gtk_widget_queue_draw(GTK_WIDGET(draw_area));
}

static void handle_gestures(GtkWidget *drawingarea)
{
    touches = g_hash_table_new_full(NULL, NULL, NULL, &free);

    drag = gtk_gesture_drag_new(drawingarea);
    rotate = gtk_gesture_rotate_new(drawingarea);
    zoom = gtk_gesture_zoom_new(drawingarea);

    g_signal_connect(drawingarea,    "touch-event",   G_CALLBACK(on_touch),              drawingarea);
    g_signal_connect_swapped(drag,   "drag-update",   G_CALLBACK(gtk_widget_queue_draw), drawingarea);
    g_signal_connect_swapped(rotate, "angle-changed", G_CALLBACK(gtk_widget_queue_draw), drawingarea);
    g_signal_connect_swapped(zoom,   "scale-changed", G_CALLBACK(gtk_widget_queue_draw), drawingarea);
}

gboolean draw(GtkWidget *widget,
              cairo_t   *cr,
              gpointer   user_data)
{
    // drag gesture
    if (gtk_switch_get_state(GTK_SWITCH(drag_switch)) && gtk_gesture_is_recognized(drag))
    {
        double x, y;
        gtk_gesture_drag_get_start_point(GTK_GESTURE_DRAG(drag), &x, &y);
        cairo_move_to(cr, x, y);

        double dx, dy;
        gtk_gesture_drag_get_offset(GTK_GESTURE_DRAG(drag), &dx, &dy);
        cairo_line_to(cr, x + dx, y + dy);

        cairo_set_source_rgb(cr, 1, 0.5, 0);
        cairo_set_line_width(cr, 4);
        cairo_stroke(cr);
    }

    // zoom and rotate gestures
    bool draw_zoom = gtk_switch_get_state(GTK_SWITCH(zoom_switch)) && gtk_gesture_is_recognized(zoom);
    bool draw_rotate = gtk_switch_get_state(GTK_SWITCH(rotate_switch)) && gtk_gesture_is_recognized(rotate);

    double scale = draw_zoom ? gtk_gesture_zoom_get_scale_delta(GTK_GESTURE_ZOOM(zoom)) : 1; 
    double rotation = draw_rotate ? gtk_gesture_rotate_get_angle_delta(GTK_GESTURE_ROTATE(rotate)) * 2 : 1;

    if (draw_zoom || draw_rotate)
    {
        double x, y;
        gtk_gesture_get_bounding_box_center(rotate, &x, &y);
        cairo_matrix_t matrix;
        cairo_matrix_init_translate(&matrix, x, y);
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
    bool draw_touches = gtk_switch_get_state(GTK_SWITCH(show_touches_switch));
    bool draw_bounding_box = gtk_switch_get_state(GTK_SWITCH(bounding_box_switch)) &&
        g_hash_table_size(touches) > 1;

    if (draw_touches || draw_bounding_box)
    {
        double min_x = gtk_widget_get_allocated_width(widget) + 1, // top left
               min_y = gtk_widget_get_allocated_height(widget) + 1,
               max_x = -1, // bottom right
               max_y = -1;
        
        const int circle_radius = 40;
        GList *touch_list = g_hash_table_get_values(touches);
        for (GList *elem = touch_list; elem != NULL; elem = elem->next)
        {
            Touch *t = elem->data;
            if (draw_touches)
            {
                cairo_arc(cr, t->x, t->y, circle_radius, 0, 2*M_PI);
                cairo_fill(cr);
            }
 
            if (draw_bounding_box)
            {
                min_x = MIN(min_x, t->x); // top left
                min_y = MIN(min_y, t->y);
                max_x = MAX(max_x, t->x); // bottom right
                max_y = MAX(max_y, t->y);
            }
        }
        g_list_free(touch_list);

        if (draw_bounding_box)
        {
            cairo_set_source_rgb(cr, 0, 1, 0);
            cairo_set_line_width(cr, 2);
            cairo_rectangle(cr, min_x-circle_radius, min_y-circle_radius, max_x-min_x+circle_radius*2, max_y-min_y+circle_radius*2);
            cairo_stroke(cr);
        }
    }

    return GDK_EVENT_PROPAGATE;
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    GtkBuilder *builder = gtk_builder_new_from_resource("/ui.glade");
    g_assert(builder);
    gtk_builder_connect_signals(builder, NULL);

#define get(widget) GTK_WIDGET(gtk_builder_get_object(builder, widget));
    GtkWidget *window              = get("window");
    GtkWidget *drawing_area        = get("drawingarea");
               show_touches_switch = get("show_touches");
               bounding_box_switch = get("bounding_box");
               rotate_switch       = get("rotate");
               zoom_switch         = get("zoom");
               drag_switch         = get("drag");
#undef get

    handle_gestures(drawing_area);

    gtk_window_maximize(GTK_WINDOW(window));
    gtk_widget_show_all(window);

    gtk_main();
}
