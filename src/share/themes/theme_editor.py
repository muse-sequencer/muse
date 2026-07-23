#!/usr/bin/env python

import colorsys
import json
import os
import shutil
import tkinter as tk
import tkinter.font as tkFont
from tkinter import ttk, colorchooser, messagebox, filedialog, simpledialog


class ColorPickerDialog(tk.Toplevel):
    """Modal color picker with both RGB and HSV sliders, kept in sync."""

    def __init__(self, parent, initial_color="#000000"):
        super().__init__(parent)
        self.title("Color Picker")
        self.resizable(False, False)
        self.transient(parent)
        self.result = None

        r, g, b = self._hex_to_rgb(initial_color)
        h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
        self.r, self.g, self.b = float(r), float(g), float(b)
        self.h, self.s, self.v = h * 360.0, s * 100.0, v * 100.0

        self._r_var = tk.DoubleVar(value=self.r)
        self._g_var = tk.DoubleVar(value=self.g)
        self._b_var = tk.DoubleVar(value=self.b)
        self._h_var = tk.DoubleVar(value=self.h)
        self._s_var = tk.DoubleVar(value=self.s)
        self._v_var = tk.DoubleVar(value=self.v)
        self._hex_var = tk.StringVar(value=self._rgb_to_hex(self.r, self.g, self.b))

        self._build()
        self._sync_preview()
        self._center_on(parent)
        self.grab_set()
        self.focus_set()

    # ---------- helpers ----------

    @staticmethod
    def _hex_to_rgb(h):
        h = h.lstrip("#")
        if len(h) == 3:
            h = "".join(c * 2 for c in h)
        return int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16)

    @staticmethod
    def _rgb_to_hex(r, g, b):
        return f"#{int(round(r)):02x}{int(round(g)):02x}{int(round(b)):02x}"

    def _build(self):
        # Preview
        wrap = tk.Frame(self, bd=1, relief="sunken")
        wrap.pack(padx=10, pady=(10, 6))
        self._preview = tk.Frame(wrap, width=280, height=60,
                                 bg=self._rgb_to_hex(self.r, self.g, self.b))
        self._preview.pack()
        self._preview.pack_propagate(False)

        # Sliders
        self._slider("R", self._r_var, 0, 255)
        self._slider("G", self._g_var, 0, 255)
        self._slider("B", self._b_var, 0, 255)
        # Separator
        tk.Frame(self, height=1, bg="#cccccc").pack(fill="x", padx=10, pady=4)
        self._slider("H", self._h_var, 0, 360)
        self._slider("S", self._s_var, 0, 100)
        self._slider("V", self._v_var, 0, 100)

        # Hex input
        row = tk.Frame(self)
        row.pack(fill="x", padx=10, pady=(6, 0))
        tk.Label(row, text="Hex").pack(side="left")
        ent = tk.Entry(row, textvariable=self._hex_var, width=10)
        ent.pack(side="left", padx=6)
        ent.bind("<Return>", self._from_hex)
        ent.bind("<FocusOut>", self._from_hex)

        # Buttons
        btns = tk.Frame(self)
        btns.pack(fill="x", padx=10, pady=10)
        tk.Button(btns, text="Cancel", width=8, command=self._cancel).pack(side="right")
        tk.Button(btns, text="OK", width=8, command=self._ok).pack(side="right", padx=6)

    def _slider(self, label, var, lo, hi):
        row = tk.Frame(self)
        row.pack(fill="x", padx=10, pady=1)
        tk.Label(row, text=label, width=2, anchor="e").pack(side="left")
        tk.Scale(row, from_=lo, to=hi, orient="horizontal",
                 variable=var, showvalue=False,
                 command=self._on_slider_moved).pack(side="left", fill="x", expand=True, padx=4)
        val_lbl = tk.Label(row, text="0", width=5, anchor="e")
        val_lbl.pack(side="left")

        def refresh(*_):
            val_lbl.config(text=f"{var.get():.0f}" if hi > 100 else f"{var.get():.0f}")
        var.trace_add("write", refresh)
        refresh()

    def _on_slider_moved(self, _evt=None):
        # Read whichever group just changed. We use the closest-to-slider value
        # so the *other* group is updated, not the one being dragged.
        r, g, b = self._r_var.get(), self._g_var.get(), self._b_var.get()
        h, s, v = self._h_var.get(), self._s_var.get(), self._v_var.get()
        # If the RGB triple matches what we already have, treat as HSV-driven
        if (round(r), round(g), round(b)) == (round(self.r), round(self.g), round(self.b)):
            self.h, self.s, self.v = h, s, v
            rr, gg, bb = colorsys.hsv_to_rgb(h / 360.0, s / 100.0, v / 100.0)
            self.r, self.g, self.b = rr * 255.0, gg * 255.0, bb * 255.0
            self._r_var.set(self.r)
            self._g_var.set(self.g)
            self._b_var.set(self.b)
        else:
            self.r, self.g, self.b = r, g, b
            hh, ss, vv = colorsys.rgb_to_hsv(r / 255.0, g / 255.0, b / 255.0)
            self.h, self.s, self.v = hh * 360.0, ss * 100.0, vv * 100.0
            self._h_var.set(self.h)
            self._s_var.set(self.s)
            self._v_var.set(self.v)
        self._sync_preview()

    def _from_hex(self, _evt=None):
        try:
            r, g, b = self._hex_to_rgb(self._hex_var.get())
        except (ValueError, IndexError):
            self._hex_var.set(self._rgb_to_hex(self.r, self.g, self.b))
            return
        self._r_var.set(r)
        self._g_var.set(g)
        self._b_var.set(b)
        self._on_slider_moved()

    def _sync_preview(self):
        self._preview.config(bg=self._rgb_to_hex(self.r, self.g, self.b))
        self._hex_var.set(self._rgb_to_hex(self.r, self.g, self.b))

    def _center_on(self, parent):
        self.update_idletasks()
        x = parent.winfo_rootx() + (parent.winfo_width() - self.winfo_width()) // 2
        y = parent.winfo_rooty() + (parent.winfo_height() - self.winfo_height()) // 2
        self.geometry(f"+{max(x, 0)}+{max(y, 0)}")

    def _ok(self):
        self.result = self._rgb_to_hex(self.r, self.g, self.b)
        self.grab_release()
        self.destroy()

    def _cancel(self):
        self.result = None
        self.grab_release()
        self.destroy()

    def show(self):
        self.wait_window()
        return self.result


class ThemeEditor(tk.Tk):
    def __init__(self, json_path=None):
        super().__init__()
        self.title("Qlementine Theme Editor")

        self.json_path = json_path
        self.original_data = {}
        self.data = {}
        self._configured_tags = set()
        self.checked_items = set()
        self.active_item = None
        self._bold_font = None  # set in create_widgets

        if json_path:
            self.load_json(json_path)

        self.create_widgets()
        self.populate_table()

    # ---------------- JSON LOAD / SAVE ----------------

    def load_json(self, path):
        with open(path, "r", encoding="utf-8") as f:
            self.original_data = json.load(f)
        self.data = dict(self.original_data)

    def save_json(self):
        if not self.json_path:
            self.json_path = filedialog.asksaveasfilename(
                defaultextension=".json",
                filetypes=[("JSON files", "*.json")]
            )
            if not self.json_path:
                return

        # Create a backup of the on-disk file before overwriting it,
        # so "Restore backup" can find a previous version.
        if os.path.exists(self.json_path):
            backup_path = self.json_path + ".bak"
            try:
                shutil.copy2(self.json_path, backup_path)
            except OSError as e:
                messagebox.showwarning("Backup failed", f"Could not create backup:\n{e}")

        with open(self.json_path, "w", encoding="utf-8") as f:
            json.dump(self.data, f, indent=2)

        messagebox.showinfo("Saved", "Theme saved successfully.")

    def restore_backup(self):
        if not self.json_path:
            messagebox.showwarning("No file", "No theme file is loaded.")
            return
        backup_path = self.json_path + ".bak"
        if not os.path.exists(backup_path):
            messagebox.showwarning(
                "No backup",
                f"No backup file found at:\n{backup_path}",
            )
            return
        try:
            with open(backup_path, "r", encoding="utf-8") as f:
                backup_data = json.load(f)
        except (OSError, json.JSONDecodeError) as e:
            messagebox.showerror("Restore failed", f"Could not read backup:\n{e}")
            return
        self.data = backup_data
        self.original_data = dict(backup_data)
        self.checked_items.clear()
        self.populate_table()
        messagebox.showinfo("Restored", "Theme restored from backup.")

    # ---------------- UI ----------------

    def create_widgets(self):
        columns = ("selected", "key", "value", "alpha")
        # selectmode="none" so clicking a row does NOT paint a blue selection
        # over the row's color swatch. The checkbox column is the only
        # "selected" indicator.
        self.tree = ttk.Treeview(
            self, columns=columns, show="headings",
            height=25, selectmode="none",
        )
        self.tree.pack(fill="both", expand=True, padx=5, pady=5)

        self.tree.heading("selected", text="✓")
        self.tree.heading("key", text="Property")
        self.tree.heading("value", text="Value")
        self.tree.heading("alpha", text="α")

        self.tree.column("selected", width=40, anchor="center")
        self.tree.column("key", width=260)
        self.tree.column("value", width=200)
        self.tree.column("alpha", width=50, anchor="center")

        # Pre-configure static tag styles
        self.tree.tag_configure("object", foreground="#666666")
        self.tree.tag_configure("thickness", background="#eef2f7")

        # Bold font for the active row
        try:
            base_font = tkFont.nametofont("TkDefaultFont")
            self._bold_font = base_font.copy()
            self._bold_font.config(weight="bold")
            self.tree.tag_configure("active", font=self._bold_font)
        except tk.TclError:
            self.tree.tag_configure("active")  # tag still works, just no font override

        # Bindings
        self.tree.bind("<Button-1>", self.on_press)
        self.tree.bind("<ButtonRelease-1>", self.on_release)
        self.tree.bind("<Double-1>", self.on_double_click)

        # Buttons
        btn_frame = tk.Frame(self)
        btn_frame.pack(fill="x", pady=5)

        tk.Button(btn_frame, text="New key", command=self.new_key).pack(side="left", padx=4)
        tk.Button(btn_frame, text="Delete checked", command=self.delete_checked).pack(side="left", padx=4)
        tk.Button(btn_frame, text="Check all", command=self.check_all).pack(side="left", padx=4)
        tk.Button(btn_frame, text="Uncheck all", command=self.uncheck_all).pack(side="left", padx=4)
        tk.Button(btn_frame, text="Save changes", command=self.save_json).pack(side="left", padx=4)
        tk.Button(btn_frame, text="Restore backup", command=self.restore_backup).pack(side="left", padx=4)
        tk.Button(btn_frame, text="Close", command=self.destroy).pack(side="right", padx=4)

    # ---------------- TABLE ----------------

    def populate_table(self):
        self.tree.delete(*self.tree.get_children())
        self.checked_items.clear()
        self.active_item = None
        self.insert_item("", self.data, depth=0)

    def insert_item(self, parent, obj, depth=0):
        """Recursive insertion of nested dicts with indentation."""
        for key, value in obj.items():
            display_key = ("  " * depth) + key
            if isinstance(value, dict):
                item_id = self.tree.insert(
                    parent, "end",
                    values=("☐", display_key, "<object>", ""),
                    tags=("object",),
                )
                self.insert_item(item_id, value, depth + 1)
            else:
                display_value = self.display_value(value)
                display_alpha = self.display_alpha(value)
                tags = self.get_value_tags(value)
                self.tree.insert(
                    parent, "end",
                    values=("☐", display_key, display_value, display_alpha),
                    tags=tags,
                )

    def display_value(self, value):
        """Return the value text for the value column (RGB only for colors)."""
        if isinstance(value, str) and value.startswith("#") and len(value) == 9:
            return value[:7]  # strip the AA suffix
        return value

    def display_alpha(self, value):
        """Return the alpha text for the alpha column (2-char hex or empty)."""
        if isinstance(value, str) and value.startswith("#") and len(value) == 9:
            return value[7:9]
        return ""

    def get_value_tags(self, value):
        """Return a list of tags used to visually style a row by its value."""
        tags = []
        if isinstance(value, str) and value.startswith("#"):
            rgb = value[:7] if len(value) == 9 else value
            if len(rgb) == 4:  # #RGB → #RRGGBB
                rgb = "#" + rgb[1] * 2 + rgb[2] * 2 + rgb[3] * 2
            if len(rgb) == 7:
                tag = f"color_{rgb}"
                if tag not in self._configured_tags:
                    fg = self._contrasting_color(rgb)
                    try:
                        self.tree.tag_configure(tag, background=rgb, foreground=fg)
                    except tk.TclError:
                        return tags
                    self._configured_tags.add(tag)
                tags.append(tag)
            return tags

        if self._is_thickness(value):
            tags.append("thickness")
        return tags

    def _is_thickness(self, value):
        try:
            return float(value) < 20
        except (ValueError, TypeError):
            return False

    def _contrasting_color(self, hex_color):
        """Return 'white' or 'black' for best contrast against the given background."""
        if not (isinstance(hex_color, str) and len(hex_color) == 7 and hex_color.startswith("#")):
            return "black"
        try:
            r = int(hex_color[1:3], 16)
            g = int(hex_color[3:5], 16)
            b = int(hex_color[5:7], 16)
        except ValueError:
            return "black"
        luminance = (0.299 * r + 0.587 * g + 0.114 * b) / 255
        return "white" if luminance < 0.55 else "black"

    def new_key(self):
        # Use RGBA so the new key is alpha-editable out of the box
        self.data["newProperty"] = "#ffffffff"
        self.populate_table()

    # ---------------- CHECKBOX / CLICK ----------------

    # ---------------- CLICK / ACTIVE ROW ----------------

    def on_press(self, event):
        # Record the press position so we can ignore drag-end releases.
        self._press_x = event.x
        self._press_y = event.y
        self._press_col = self.tree.identify_column(event.x)
        self._press_row = self.tree.identify_row(event.y)

    def on_release(self, event):
        # Only act if the mouse didn't drift much (i.e. this is a click, not a drag)
        if not hasattr(self, "_press_x"):
            return
        if abs(event.x - self._press_x) > 4 or abs(event.y - self._press_y) > 4:
            return
        col = self.tree.identify_column(event.x)
        row_id = self.tree.identify_row(event.y)
        if not row_id:
            return
        # If the release landed on a different cell than the press, ignore
        if col != self._press_col or row_id != self._press_row:
            return
        if col == "#1":
            self.toggle_checkbox(row_id)
        else:
            self.set_active(row_id)

    def set_active(self, item_id):
        """Mark a row as the 'active' row, shown via bold font (no bg change)."""
        if self.active_item == item_id:
            return
        if self.active_item and self.tree.exists(self.active_item):
            old_tags = list(self.tree.item(self.active_item)["tags"])
            if "active" in old_tags:
                old_tags.remove("active")
                self.tree.item(self.active_item, tags=old_tags)
        self.active_item = item_id
        if item_id:
            new_tags = list(self.tree.item(item_id)["tags"])
            if "active" not in new_tags:
                new_tags.append("active")
                self.tree.item(item_id, tags=new_tags)

    def toggle_checkbox(self, item_id):
        """Toggle the checkbox state of the given item."""
        vals = self.tree.item(item_id)["values"]
        if not vals:
            return
        if vals[0] == "☐":
            new_vals = ("☑",) + tuple(vals[1:])
            self.tree.item(item_id, values=new_vals)
            self.checked_items.add(item_id)
        elif vals[0] == "☑":
            new_vals = ("☐",) + tuple(vals[1:])
            self.tree.item(item_id, values=new_vals)
            self.checked_items.discard(item_id)

    def check_all(self):
        for item_id in self._all_items():
            self._set_checkbox(item_id, True)

    def uncheck_all(self):
        for item_id in self._all_items():
            self._set_checkbox(item_id, False)
        self.checked_items.clear()

    def _set_checkbox(self, item_id, checked):
        vals = self.tree.item(item_id)["values"]
        if not vals:
            return
        target = "☑" if checked else "☐"
        if vals[0] == target:
            return
        new_vals = (target,) + tuple(vals[1:])
        self.tree.item(item_id, values=new_vals)
        if checked:
            self.checked_items.add(item_id)
        else:
            self.checked_items.discard(item_id)

    def _all_items(self):
        """Yield every item id in the tree (depth-first)."""
        def recurse(parent):
            for child in self.tree.get_children(parent):
                yield child
                yield from recurse(child)
        yield from recurse("")

    def delete_checked(self):
        if not self.checked_items:
            messagebox.showwarning("No selection", "Check at least one row to delete.")
            return
        # Resolve paths for all checked items
        paths = []
        for item_id in self.checked_items:
            path = self.get_json_path(item_id)
            if path:
                paths.append(path)
        # Delete deeper paths first so parents are removed after their children
        paths.sort(key=len, reverse=True)
        for path in paths:
            self.delete_json_key(path)
        self.checked_items.clear()
        self.populate_table()

    # ---------------- JSON PATH UTILITIES ----------------

    def get_json_path(self, item_id):
        """Return list of keys representing the path inside nested dicts."""
        path = []
        while item_id:
            vals = self.tree.item(item_id)["values"]
            if vals:
                key = vals[1].strip()  # strip leading indentation spaces
                path.insert(0, key)
            item_id = self.tree.parent(item_id)
        return path

    def delete_json_key(self, path):
        obj = self.data
        for p in path[:-1]:
            if not isinstance(obj, dict) or p not in obj:
                return
            obj = obj[p]
        if isinstance(obj, dict) and path[-1] in obj:
            del obj[path[-1]]

    # ---------------- COLOR / ALPHA UTILITIES ----------------

    def normalize_color(self, value):
        """Convert #RRGGBBAA → #RRGGBB. Expand #RGB → #RRGGBB."""
        if isinstance(value, str) and value.startswith("#"):
            if len(value) == 9:
                return value[:7]
            if len(value) == 4:
                return "#" + value[1] * 2 + value[2] * 2 + value[3] * 2
        return value

    def extract_alpha(self, value):
        """Return alpha as integer 0–255 or empty string."""
        if isinstance(value, str) and value.startswith("#") and len(value) == 9:
            return int(value[7:9], 16)
        return ""

    # ---------------- EDITORS ----------------

    def on_double_click(self, event):
        col = self.tree.identify_column(event.x)
        if col == "#1":  # checkbox column, ignore
            return
        row_id = self.tree.identify_row(event.y)
        if not row_id:
            return
        if col == "#3":  # value column
            self.edit_value(row_id)
        elif col == "#4":  # alpha column
            self.edit_alpha(row_id)

    def edit_value(self, row_id):
        """Dispatch to the right editor based on the value's Python type."""
        full_value = self.lookup_value(row_id)
        if full_value is None:
            return
        # bool MUST be checked before int (bool is a subclass of int)
        if isinstance(full_value, bool):
            self.edit_bool(row_id)
        elif isinstance(full_value, int):
            self.edit_int(row_id)
        elif isinstance(full_value, float):
            self.edit_float(row_id)
        elif isinstance(full_value, str):
            if full_value.startswith("#"):
                self.edit_color(row_id)
            else:
                self.edit_string(row_id)
        else:
            messagebox.showinfo(
                "Not editable",
                f"Values of type {type(full_value).__name__} are not editable here.",
            )

    def edit_string(self, row_id):
        vals = self.tree.item(row_id)["values"]
        key = vals[1].strip()
        current_value = self.lookup_value(row_id)
        new_value = simpledialog.askstring(
            "Edit String",
            f"Enter value for '{key}':",
            initialvalue=current_value,
            parent=self,
        )
        if new_value is None:
            return
        path = self.get_json_path(row_id)
        self.set_json_value(path, new_value)
        self.populate_table()

    def edit_int(self, row_id):
        vals = self.tree.item(row_id)["values"]
        key = vals[1].strip()
        current_value = self.lookup_value(row_id)
        new_value = simpledialog.askinteger(
            "Edit Integer",
            f"Enter integer for '{key}':",
            initialvalue=current_value,
            parent=self,
            minvalue=-2**31, maxvalue=2**31 - 1,
        )
        if new_value is None:
            return
        path = self.get_json_path(row_id)
        self.set_json_value(path, new_value)
        self.populate_table()

    def edit_float(self, row_id):
        vals = self.tree.item(row_id)["values"]
        key = vals[1].strip()
        current_value = self.lookup_value(row_id)
        new_value = simpledialog.askfloat(
            "Edit Number",
            f"Enter number for '{key}':",
            initialvalue=current_value,
            parent=self,
        )
        if new_value is None:
            return
        path = self.get_json_path(row_id)
        self.set_json_value(path, new_value)
        self.populate_table()

    def edit_bool(self, row_id):
        """Toggle a boolean value (double-click flips it)."""
        full_value = self.lookup_value(row_id)
        new_value = not full_value
        path = self.get_json_path(row_id)
        self.set_json_value(path, new_value)
        self.populate_table()

    def edit_color(self, row_id):
        full_value = self.lookup_value(row_id)
        if not (isinstance(full_value, str) and full_value.startswith("#")):
            return
        rgb = self.normalize_color(full_value)
        dialog = ColorPickerDialog(self, initial_color=rgb)
        new_color = dialog.show()
        if new_color:
            # Preserve alpha if present
            alpha = self.extract_alpha(full_value)
            if alpha != "":
                new_color = f"{new_color}{alpha:02x}"
            path = self.get_json_path(row_id)
            self.set_json_value(path, new_color)
            self.populate_table()

    def edit_alpha(self, row_id):
        vals = self.tree.item(row_id)["values"]
        key = vals[1].strip()
        full_value = self.lookup_value(row_id)
        if not (isinstance(full_value, str) and full_value.startswith("#") and len(full_value) == 9):
            messagebox.showwarning(
                "No alpha",
                f"'{key}' does not have an alpha channel (use #RRGGBBAA).",
            )
            return
        current_alpha_hex = full_value[7:9]
        new_alpha_hex = simpledialog.askstring(
            "Edit Alpha",
            f"Enter alpha (00–ff) for '{key}':",
            initialvalue=current_alpha_hex,
            parent=self,
        )
        if new_alpha_hex is None:
            return
        new_alpha_hex = new_alpha_hex.strip().lower()
        if len(new_alpha_hex) != 2:
            messagebox.showerror("Invalid", "Alpha must be exactly 2 hex digits (00–ff).")
            return
        try:
            int(new_alpha_hex, 16)
        except ValueError:
            messagebox.showerror("Invalid", "Alpha must be a valid hex value.")
            return
        rgb = full_value[:7]
        new_value = f"{rgb}{new_alpha_hex}"
        path = self.get_json_path(row_id)
        self.set_json_value(path, new_value)
        self.populate_table()

    def lookup_value(self, item_id):
        """Return the actual JSON value for a given tree item (with alpha intact)."""
        path = self.get_json_path(item_id)
        obj = self.data
        for p in path:
            if isinstance(obj, dict) and p in obj:
                obj = obj[p]
            else:
                return None
        return obj

    def set_json_value(self, path, new_value):
        obj = self.data
        for p in path[:-1]:
            obj = obj[p]
        obj[path[-1]] = new_value


# ---------------- MAIN ----------------

if __name__ == "__main__":
    import sys
    path = sys.argv[1] if len(sys.argv) > 1 else None
    app = ThemeEditor(path)
    app.mainloop()
