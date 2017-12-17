//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  compact_patch_edit.h
//  (C) Copyright 2015-2016 Tim E. Real (terminator356 on sourceforge)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __COMPACT_PATCH_EDIT_H__
#define __COMPACT_PATCH_EDIT_H__

#include <QFrame>

class QMouseEvent;

namespace MusEGui {

class ElidedLabel;
class LCDPatchEdit;

//---------------------------------------------------------
//   CompactPatchEdit
//---------------------------------------------------------

class CompactPatchEdit : public QFrame
{
  Q_OBJECT

  public:
    enum ReadoutOrientation { ReadoutHorizontal = 0, ReadoutVertical };

  private:
    ReadoutOrientation _orient;
    bool _showPatchLabel;
    int _maxAliasedPointSize;
    int _id;
    int _currentPatch;
    LCDPatchEdit* _patchEdit;
    ElidedLabel* _patchNameLabel;

  private slots:
    void patchEditValueChanged(int val, int id);
    void patchEditDoubleClicked(QPoint p, int id, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys);
    void patchEditRightClicked(QPoint p, int id);
    void patchNamePressed(QPoint p, int id, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys);
    void patchNameReturnPressed(QPoint p, int id, Qt::KeyboardModifiers keys);

//  protected:
//     virtual void keyPressEvent(QKeyEvent*);

  signals:
    void patchValueRightClicked(QPoint p, int id);
    void patchNameClicked(QPoint p, int id);
    void patchNameRightClicked(QPoint p, int id);
    void valueChanged(int value, int id);

  public:
    CompactPatchEdit(QWidget *parent, const char *name = 0,
                QColor fillColor = QColor());

    virtual ~CompactPatchEdit();

    static QSize getMinimumSizeHint(const QFontMetrics& fm,
                                    Qt::Orientation orient = Qt::Vertical,
                                    int xMargin = 0,
                                    int yMargin = 0);

    int id() const             { return _id; }
    void setId(int i)          { _id = i; }

    int value() const;
    void setValue(int v);
    void setLastValidValue(int v);
    void setLastValidBytes(int hbank, int lbank, int prog);

    // Sets up tabbing for the entire patch edit.
    // Accepts a previousWidget which can be null and returns the last widget in the control,
    //  which allows chaining other widgets.
    virtual QWidget* setupComponentTabbing(QWidget* previousWidget = 0);
      
    void setReadoutOrientation(ReadoutOrientation);
    void setShowPatchLabel(bool);

    void setReadoutColor(const QColor&);

    // At what point size to switch from aliased text to non-aliased text. Zero means always use anti-aliasing.
    // Here in CompactPatchEdit, this only affects the CompactSliders so far, not the patch label.
    // If -1, no value has been set and default is each widget's setting.
    int maxAliasedPointSize() const { return _maxAliasedPointSize; }
    // Sets at what point size to switch from aliased text (brighter, crisper but may look too jagged and unreadable with some fonts)
    //  to non-aliased text (dimmer, fuzzier but looks better). Zero means always use anti-aliasing. Default is each widget's setting.
    // Here in CompactPatchEdit, this only affects the CompactSliders so far, not the patch label.
    void setMaxAliasedPointSize(int sz);

    QString patchName() const;
    void setPatchName(const QString& patchName);
    // Sets the off state.
    void setPatchNameOff(bool v);
    
    //virtual QSize sizeHint() const;
};

} // namespace MusEGui

#endif
