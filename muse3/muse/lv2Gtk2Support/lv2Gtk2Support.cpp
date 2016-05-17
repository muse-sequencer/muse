
#include "lv2Gtk2Support.h"

#include <gtk/gtk.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gdk/gdkx.h>

namespace MusEGui
{
  
Gtk::Main *gtkmm2Main = NULL;

typedef void(*sz_cb_fn)(int, int, void *);

static void
plug_on_size_request(GtkWidget* widget, GtkRequisition* requisition, gpointer user_data)
{
   sz_cb_fn fn = reinterpret_cast<sz_cb_fn>(user_data);
   int width = requisition->width;
   int height = requisition->height;
   void *arg = static_cast<void *>(g_object_get_data(G_OBJECT(widget), "lv2Gtk2Helper_arg"));
   fn(width, height, arg);
}

static void
plug_on_size_allocate(GtkWidget* widget, GdkRectangle* allocation, gpointer user_data)
{
   sz_cb_fn fn = reinterpret_cast<sz_cb_fn>(user_data);
   int width = allocation->width;
   int height = allocation->height;   
   void *arg = static_cast<void *>(g_object_get_data(G_OBJECT(widget), "lv2Gtk2Helper_arg"));
   gtk_widget_set_size_request( widget, width, height );
   fn(width, height, arg);
}

bool lv2Gtk2Helper_init()
{
   gtk_init(NULL, NULL);
   //create gtkmm2 main class // Not required?
   gtkmm2Main = new Gtk::Main(NULL, NULL);
   return true;
}

void *lv2Gtk2Helper_gtk_plug_new(unsigned long winId, void *arg)
{
   GtkWidget *gtkPlug = gtk_plug_new(winId);
   g_object_set_data(G_OBJECT(gtkPlug), "lv2Gtk2Helper_arg", arg);
   return static_cast<void *>(gtkPlug);
}

void lv2Gtk2Helper_gtk_widget_destroy(void *plug)
{
   gtk_widget_destroy(static_cast<GtkWidget *>(plug));
}

void lv2Gtk2Helper_gtk_container_add(void *plug, void *w)
{
   gtk_container_add(GTK_CONTAINER(plug), static_cast<GtkWidget *>(w));
}

void lv2Gtk2Helper_gtk_widget_show_all(void *plug)
{
   //gtk_widget_realize(static_cast<GtkWidget *>(plug));
   gtk_widget_show_all(static_cast<GtkWidget *>(plug));
}

void lv2Gtk2Helper_gtk_widget_get_allocation(void *plug, int *width, int *height)
{
   GtkAllocation allocSize;
   gtk_widget_get_allocation(static_cast<GtkWidget *>(plug), &allocSize);
   *width = allocSize.width;
   *height = allocSize.height;
}

void lv2Gtk2Helper_register_allocate_cb(void *plug, sz_cb_fn fn)
{
   g_signal_connect(G_OBJECT(plug), "size-allocate", G_CALLBACK(plug_on_size_allocate), reinterpret_cast<gpointer>(fn));
}

void lv2Gtk2Helper_register_resize_cb(void *plug, sz_cb_fn fn)
{
   g_signal_connect(G_OBJECT(plug), "size-request", G_CALLBACK(plug_on_size_request), reinterpret_cast<gpointer>(fn));
}

unsigned long lv2Gtk2Helper_gdk_x11_drawable_get_xid(void *plug)
{
   //GdkWindow *w =gtk_widget_get_window(static_cast<GtkWidget *>(widget));
   //return gdk_x11_drawable_get_xid(w);
   return gtk_plug_get_id(static_cast<GtkPlug *>(plug));
}

void lv2Gtk2Helper_deinit()
{
   if(gtkmm2Main != NULL)
   {
      delete gtkmm2Main;
      gtkmm2Main = NULL;
   }
}

} // namespace MusEGui
