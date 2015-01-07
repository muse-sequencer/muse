#include <dlfcn.h>
#include <gtk/gtk.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gdk/gdkx.h>
#include <stdio.h>

static void *gtk2LibHandle = NULL;
static void *gtkmm2LibHandle = NULL;

#define LIBGTK2_LIBRARY_NAME "libgtk-x11-2.0.so"
#define LIBGTKMM2_LIBRARY_NAME "libgtkmm-2.4.so"
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
   fn(width, height, arg);
}

extern "C" bool lv2Gtk2Helper_init()
{
   const char *gtk2LibraryNames []= {"libgtk-x11-2.0.so", "libgtk-x11-2.0.so.0", NULL};
   const char *gtk2mmLibraryNames [] = {"libgtkmm-2.4.so", "libgtkmm-2.4.so.1", NULL};
   if(gtk2LibHandle == NULL)
   {
      int i = 0;
      while(gtk2LibraryNames [i] != NULL)
      {
         const char *libName = gtk2LibraryNames [i];
         gtk2LibHandle = dlopen(libName, RTLD_GLOBAL | RTLD_LAZY);
         char *err = dlerror();
         fprintf(stderr, "Lv2Gtk2Helper: dlerror (%s) = %s\n", libName, err);
         if(gtk2LibHandle != NULL)
         {
            break;
         }
         ++i;
      }

      i = 0;
      while(gtk2mmLibraryNames [i] != NULL)
      {
         const char *libName = gtk2mmLibraryNames [i];
         gtkmm2LibHandle = dlopen(libName, RTLD_GLOBAL | RTLD_LAZY);
         char *err = dlerror();
         fprintf(stderr, "Lv2Gtk2Helper: dlerror (%s) = %s\n", libName, err);
         if(gtkmm2LibHandle != NULL)
         {
            break;
         }
         ++i;
      }

      if(gtk2LibHandle == NULL || gtkmm2LibHandle == NULL)
      {
         if(gtk2LibHandle != NULL)
         {
            dlclose(gtk2LibHandle);
            gtk2LibHandle = NULL;
         }

         if(gtkmm2LibHandle != NULL)
         {
            dlclose(gtkmm2LibHandle);
            gtkmm2LibHandle = NULL;
         }
         return false;
      }

      gtk_init(NULL, NULL);
      //create gtkmm2 main class
      gtkmm2Main = new Gtk::Main(NULL, NULL);
   }
   return true;
}

extern "C" void *lv2Gtk2Helper_gtk_plug_new(unsigned long winId, void *arg)
{
   GtkWidget *gtkPlug = gtk_plug_new(winId);
   g_object_set_data(G_OBJECT(gtkPlug), "lv2Gtk2Helper_arg", arg);
   return static_cast<void *>(gtkPlug);
}

extern "C" void lv2Gtk2Helper_gtk_widget_destroy(void *plug)
{
   gtk_widget_destroy(static_cast<GtkWidget *>(plug));
}

extern "C" void lv2Gtk2Helper_gtk_container_add(void *plug, void *w)
{
   gtk_container_add(GTK_CONTAINER(plug), static_cast<GtkWidget *>(w));
}

extern "C" void lv2Gtk2Helper_gtk_widget_show_all(void *plug)
{
   gtk_widget_realize(static_cast<GtkWidget *>(plug));
   gtk_widget_show_all(static_cast<GtkWidget *>(plug));
}

extern "C" void lv2Gtk2Helper_gtk_widget_get_allocation(void *plug, int *width, int *height)
{
   GtkAllocation allocSize;
   gtk_widget_get_allocation(static_cast<GtkWidget *>(plug), &allocSize);
   *width = allocSize.width;
   *height = allocSize.height;
}


extern "C" void lv2Gtk2Helper_register_allocate_cb(void *plug, sz_cb_fn fn)
{
   g_signal_connect(G_OBJECT(plug), "size-allocate", G_CALLBACK(plug_on_size_allocate), reinterpret_cast<gpointer>(fn));
}

extern "C" void lv2Gtk2Helper_register_resize_cb(void *plug, sz_cb_fn fn)
{
   g_signal_connect(G_OBJECT(plug), "size-request", G_CALLBACK(plug_on_size_request), reinterpret_cast<gpointer>(fn));
}

extern "C" unsigned long lv2Gtk2Helper_gdk_x11_drawable_get_xid(void *widget)
{
   GdkWindow *w =gtk_widget_get_window(static_cast<GtkWidget *>(widget));
   return gdk_x11_drawable_get_xid(w);
}

extern "C" void lv2Gtk2Helper_deinit()
{
   if(gtkmm2Main != NULL)
   {
      delete gtkmm2Main;
      gtkmm2Main = NULL;
   }
   if(gtk2LibHandle != NULL)
   {
      dlclose(gtk2LibHandle);
      gtk2LibHandle = NULL;
   }

   if(gtkmm2LibHandle != NULL)
   {
      dlclose(gtkmm2LibHandle);
      gtkmm2LibHandle = NULL;
   }

}
