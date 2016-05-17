#ifndef __LV2_GTK2_SUPPORT_H__
#define __LV2_GTK2_SUPPORT_H__

namespace MusEGui
{
  
  typedef void(*sz_cb_fn)(int, int, void *);
  bool lv2Gtk2Helper_init();
  void *lv2Gtk2Helper_gtk_plug_new(unsigned long winId, void *arg);
  void lv2Gtk2Helper_gtk_widget_destroy(void *plug);
  void lv2Gtk2Helper_gtk_container_add(void *plug, void *w);
  void lv2Gtk2Helper_gtk_widget_show_all(void *plug);
  void lv2Gtk2Helper_gtk_widget_get_allocation(void *plug, int *width, int *height);
  void lv2Gtk2Helper_register_allocate_cb(void *plug, sz_cb_fn fn);
  void lv2Gtk2Helper_register_resize_cb(void *plug, sz_cb_fn fn);
  unsigned long lv2Gtk2Helper_gdk_x11_drawable_get_xid(void *plug);
  void lv2Gtk2Helper_deinit();
  
} // namespace MusEGui

#endif
