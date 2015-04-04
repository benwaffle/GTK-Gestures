enum drawing { drag, rotate } state;

union {
    struct { double start_x, start_y, end_x, end_y; }; // drag
    struct { double angle, angle_delta; }; // rotate
} data;

typedef struct { double x, y; } Touch;
GHashTable *touches = NULL;

static void on_touch(GtkWidget      *w,
                     GdkEventTouch  *ev,
                     GtkDrawingArea *draw_area)
{
    printf("event = %d\n", ev->type);
    switch (ev->type)
    {
        case GDK_TOUCH_BEGIN:
        {
            if (!g_hash_table_contains(touches, ev->sequence))
            {
                Touch *t = malloc(sizeof(Touch));
                t->x = ev->x;
                t->y = ev->y;
                g_hash_table_insert(touches, ev->sequence, t);
                printf("\tbegin %p: %g,%g\n", ev->sequence, t->x, t->y);
            }
            break;
        }

        case GDK_TOUCH_UPDATE:
        {
            Touch *t = g_hash_table_lookup(touches, ev->sequence);
            t->x = ev->x;
            t->y = ev->y;
            printf("\tupdate %p: %g,%g\n", ev->sequence, t->x, t->y);
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

static void on_drag(GtkGestureDrag *gesture,
                    double         offset_x,
                    double         offset_y,
                    GtkDrawingArea *draw_area)
{
    gtk_gesture_drag_get_start_point(gesture, &data.start_x, &data.start_y);
    data.end_x = data.start_x + offset_x;
    data.end_y = data.start_y + offset_y;
    state = drag;
    gtk_widget_queue_draw(GTK_WIDGET(draw_area));
}

static void on_rotate(GtkGesture     *gesture,
                      double          angle,
                      double          angle_delta,
                      GtkDrawingArea *draw_area)
{
    data.angle = angle;
    data.angle_delta = angle_delta;
    state = rotate;
    gtk_widget_queue_draw(GTK_WIDGET(draw_area));
}

static void handle_gestures(GtkWidget *widget,
                            GtkWidget *draw_area)
{
    touches = g_hash_table_new_full(NULL, NULL, NULL, &free);
    g_signal_connect(widget, "touch-event", G_CALLBACK(on_touch), draw_area);

    GtkGesture *drag = gtk_gesture_drag_new(widget);
    g_signal_connect(drag, "drag-update", G_CALLBACK(on_drag), draw_area);
    
    GtkGesture *rotate = gtk_gesture_rotate_new(widget);
    g_signal_connect(rotate, "angle-changed", G_CALLBACK(on_rotate), draw_area);
}

static bool draw(GtkWidget *widget,
                     cairo_t   *cr,
                     gpointer   user_data)
{
    cairo_set_source_rgb(cr, 0, 0, 0);
 
    if (state == drag) {
        cairo_move_to(cr, data.start_x, data.start_y);
        cairo_line_to(cr, data.end_x, data.end_y);
        cairo_set_line_width(cr, 4.0);
        cairo_stroke(cr);
    } else if (state == rotate) {
        cairo_arc(cr, 500, 500, 200, 0, data.angle_delta);
        cairo_fill(cr);
    }

    GList *touch_list = g_hash_table_get_values(touches);
    for (GList *touch_elem = touch_list; touch_elem  != NULL; touch_elem = touch_elem->next)
    {
        Touch *t = touch_elem->data;
        cairo_arc(cr, t->x, t->y, 25, 0, 2*M_PI);
        cairo_fill(cr);
    }
    g_list_free(touch_list);
    return GDK_EVENT_PROPAGATE;
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GTK gestures demo");
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *drawing_area = gtk_drawing_area_new();
    g_signal_connect(drawing_area, "draw", G_CALLBACK(draw), NULL);
    gtk_container_add(GTK_CONTAINER(window), drawing_area);

    handle_gestures(window, drawing_area);
    gtk_widget_show_all(window);

    gtk_main();
}
