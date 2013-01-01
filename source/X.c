#include <X11/X.h>
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <zvt/zvtterm.h>

void size_allocate (GtkWidget *widget, GtkWindow *window)
{
  ZvtTerm *term;
  XSizeHints sizehints;

  g_assert (widget != NULL);
  term = ZVT_TERM (widget);
  
  /* Not sure why it is x3 and x2.... klass shows as 2x2 and the variation is 6x4 */
  sizehints.base_width = 
    (GTK_WIDGET (window)->allocation.width) +
    (GTK_WIDGET (term)->style->klass->xthickness * 3) -
    (GTK_WIDGET (term)->allocation.width);
  
  sizehints.base_height =
    (GTK_WIDGET (window)->allocation.height) +
    (GTK_WIDGET (term)->style->klass->ythickness * 2) -
    (GTK_WIDGET (term)->allocation.height);
  
  sizehints.width_inc = term->charwidth;
  sizehints.height_inc = term->charheight;
  sizehints.min_width = sizehints.base_width + (sizehints.width_inc * 20);
  sizehints.min_height = sizehints.base_height + (sizehints.height_inc * 5);
  
  sizehints.flags = (PBaseSize|PMinSize|PResizeInc);
  
  XSetWMNormalHints (GDK_DISPLAY(),
		     GDK_WINDOW_XWINDOW (GTK_WIDGET (window)->window),
		     &sizehints);
  gdk_flush ();
}
