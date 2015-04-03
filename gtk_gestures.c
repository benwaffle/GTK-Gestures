gdouble startx = 0, starty = 0, endx = 0, endy = 0;
gdouble angle = 0, angle_delta = 0;

enum drawing { drag, rotate };
enum drawing state;

static void on_drag(GtkGestureDrag *gesture,
                    gdouble         offset_x,
                    gdouble         offset_y,
                    GtkDrawingArea *draw_area)
{
	gtk_gesture_drag_get_start_point(gesture, &startx, &starty);
	endx = offset_x;
	endy = offset_y;
	state = drag;
	gtk_widget_queue_draw(GTK_WIDGET(draw_area));
}

static void on_rotate(GtkGesture       *gesture,
                      GdkEventSequence *sequence,
                      GtkDrawingArea   *draw_area)
{
	//angle = _angle;
	//angle_delta = _angle_delta;
	state = rotate;
	gtk_widget_queue_draw(GTK_WIDGET(draw_area));
}

static void handle_gestures(GtkWidget *w,
                            GtkWidget *draw_area)
{
    GtkGesture *drag = gtk_gesture_drag_new(w);
    g_signal_connect(drag, "drag-update", G_CALLBACK(on_drag), draw_area);
    
    GtkGesture *rotate = gtk_gesture_rotate_new(w);
    g_signal_connect(rotate, "begin", G_CALLBACK(on_rotate), draw_area);
}

static gboolean draw(GtkWidget *widget,
                     cairo_t   *cr,
                     gpointer   user_data)
{
	cairo_set_source_rgb(cr, 0, 0, 0);
	if (state == drag) {
		cairo_move_to(cr, startx, starty);
		cairo_line_to(cr, startx+endx, starty+endy);
		cairo_set_line_width(cr, 4.0);
		cairo_stroke(cr);
	} else {
		puts("drawing rotate");
		cairo_arc(cr, 500, 500, 200, 0, 2*M_PI);
		cairo_fill(cr);
	}
	return FALSE;
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
