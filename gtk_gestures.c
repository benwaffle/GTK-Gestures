static GtkGesture *drag, *rotate, *zoom;

typedef struct { double x, y; } Touch;
static GHashTable *touches = NULL;

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

static void handle_gestures(GtkWidget *widget,
                            GtkWidget *draw_area)
{
    touches = g_hash_table_new_full(NULL, NULL, NULL, &free);
    g_signal_connect(widget, "touch-event", G_CALLBACK(on_touch), draw_area);

    drag = gtk_gesture_drag_new(widget);
    rotate = gtk_gesture_rotate_new(widget);
    zoom = gtk_gesture_zoom_new(widget);

    g_signal_connect_swapped(drag, "drag-update", G_CALLBACK(gtk_widget_queue_draw), draw_area);
    g_signal_connect_swapped(rotate, "angle-changed", G_CALLBACK(gtk_widget_queue_draw), draw_area);
    g_signal_connect_swapped(zoom, "scale-changed", G_CALLBACK(gtk_widget_queue_draw), draw_area);
}

static gboolean draw(GtkWidget *widget,
                     cairo_t   *cr,
                     gpointer   user_data)
{
    if (gtk_gesture_is_recognized(drag))
    {
        gdouble x, y;
        gtk_gesture_drag_get_start_point(GTK_GESTURE_DRAG(drag), &x, &y);
        cairo_move_to(cr, x, y);

        gdouble dx, dy;
        gtk_gesture_drag_get_offset(GTK_GESTURE_DRAG(drag), &dx, &dy);
        cairo_line_to(cr, x + dx, y + dy);

        cairo_set_source_rgb(cr, 1, 0.5, 0);
        cairo_set_line_width(cr, 4);
        cairo_stroke(cr);
    }

    gdouble scale = 1;
    if (gtk_gesture_is_recognized(zoom))
        scale = gtk_gesture_zoom_get_scale_delta(GTK_GESTURE_ZOOM(zoom));
    if (gtk_gesture_is_recognized(rotate))
    {
        cairo_matrix_t matrix;
        gdouble x, y;
        gtk_gesture_get_bounding_box_center(rotate, &x, &y);
        cairo_matrix_init_translate(&matrix, x, y);
        cairo_matrix_rotate(&matrix, gtk_gesture_rotate_get_angle_delta(GTK_GESTURE_ROTATE(rotate))*2);
        cairo_matrix_scale(&matrix, scale, scale);

        cairo_save(cr); // save matrix
        {
            cairo_set_matrix(cr, &matrix);
            cairo_set_source_rgb(cr, 1, 0, 0.5);
            cairo_rectangle(cr, -100, -100, 200, 200);
            cairo_fill(cr);
        }
        cairo_restore(cr);
    }

    double min_x = gtk_widget_get_allocated_width(widget);
    double max_x = 0;
    double min_y = gtk_widget_get_allocated_height(widget);
    double max_y = 0;

    GList *touch_list = g_hash_table_get_values(touches);
    for (GList *touch_elem = touch_list; touch_elem  != NULL; touch_elem = touch_elem->next)
    {
        Touch *t = touch_elem->data;
        cairo_arc(cr, t->x, t->y, 25, 0, 2*M_PI);
        cairo_fill(cr);

        min_x = MIN(min_x, t->x);
        min_y = MIN(min_y, t->y);
        max_x = MAX(max_x, t->x);
        max_y = MAX(max_y, t->y);
    }
    g_list_free(touch_list);

    cairo_set_source_rgb(cr, 0, 1, 0);
    cairo_set_line_width(cr, 2);
    cairo_rectangle(cr, min_x, min_y, max_x-min_x, max_y-min_y);
    cairo_stroke(cr);

    return GDK_EVENT_PROPAGATE;
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GTK gestures demo");
    gtk_window_maximize(GTK_WINDOW(window));
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *drawing_area = gtk_drawing_area_new();
    g_signal_connect(drawing_area, "draw", G_CALLBACK(draw), NULL);
    gtk_container_add(GTK_CONTAINER(window), drawing_area);

    handle_gestures(window, drawing_area);
    gtk_widget_show_all(window);

    gtk_main();
}
