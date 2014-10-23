#include <dlfcn.h>
#include <gtk/gtk.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>

static void *gtk2LibHandle = NULL;
static void *gtkmm2LibHandle = NULL;

#define LIBGTK2_LIBRARY_NAME "libgtk-x11-2.0.so"
#define LIBGTKMM2_LIBRARY_NAME "libgtkmm-2.4.so"
Gtk::Main *gtkmm2Main = NULL;

extern "C" bool lv2GtkHelper_init()
{
   if(gtk2LibHandle == NULL)
   {
      gtk2LibHandle = dlopen(LIBGTK2_LIBRARY_NAME, RTLD_GLOBAL | RTLD_LAZY);
      gtkmm2LibHandle = dlopen(LIBGTKMM2_LIBRARY_NAME, RTLD_GLOBAL | RTLD_LAZY);

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

extern "C" void *lv2GtkHelper_gtk_plug_new(unsigned long winId)
{
   return static_cast<void *>(gtk_plug_new(winId));
}

extern "C" void lv2GtkHelper_gtk_widget_destroy(void *w)
{
   gtk_widget_destroy(static_cast<GtkWidget *>(w));
}

extern "C" void lv2GtkHelper_gtk_container_add(void *plug, void *w)
{
   gtk_container_add(GTK_CONTAINER(plug), static_cast<GtkWidget *>(w));
}

extern "C" void lv2GtkHelper_gtk_widget_show_all(void *w)
{
   gtk_widget_show_all(static_cast<GtkWidget *>(w));
}

extern "C" void lv2GtkHelper_gtk_widget_get_allocation(void *w, int *width, int *height)
{
   GtkAllocation allocSize;
   gtk_widget_get_allocation(static_cast<GtkWidget *>(w), &allocSize);
   *width = allocSize.width;
   *height = allocSize.height;
}

extern "C" void lb2GtkHelper_deinit()
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
