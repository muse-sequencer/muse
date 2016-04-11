//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: strip.h,v 1.3.2.2 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 - 2016 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __STRIP_H__
#define __STRIP_H__

#include <list>

#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>

#include "type_defs.h"
#include "globaldefs.h"
#include "drange.h"

class QMouseEvent;
class QResizeEvent;
class QToolButton;
class QGridLayout;
class QLayout;
class QSize;

namespace MusECore {
class Track;
}

namespace MusEGui {
class CompactComboBox;
class Meter;
class CompactSlider;
class ElidedLabel;

//---------------------------------------------
// ComponentWidget
//  Defines an item in a rack layout.
//---------------------------------------------

class ComponentWidget
{
  public:
    // Type of components.
    typedef int ComponentType;
    // Type of component widget.
    typedef int ComponentWidgetType;
    
    // The component widget.
    QWidget* _widget;
    
  public:
    // Type of widget.
    ComponentWidgetType _widgetType;
    // Type of component: Controller, gain property, aux property etc.
    ComponentType _componentType;
    // User index value (controller index, aux property index etc.) provided to slots.
    int _index;
    // Cached current value, for widgets that use it.
    //double _currentValue;
    // Cached flag, for widgets that use it.
    bool _pressed;
    
  public:        
    ComponentWidget() : _widget(0),
                        _widgetType(0),
                        _componentType(0), 
                          _index(0),
                          _pressed(false)
                        { } 
                            
    ComponentWidget(QWidget* widget,
                    ComponentWidgetType widgetType,
                    ComponentType componentType,
                    int index
                    ) : 
                        _widget(widget),
                        _widgetType(widgetType),
                        _componentType(componentType), 
                        _index(index), 
                        _pressed(false)
                        { }
                        
    bool isValid() const { return _widget != 0; }
};

//---------------------------------------------
// ComponentWidgetList
//  List of components.
//---------------------------------------------

class ComponentWidgetList : public std::list<ComponentWidget>
{
  public:
    iterator find(ComponentWidget::ComponentType componentType, 
      int componentWidgetType = -1,
      int index = -1, QWidget* widget = 0) 
    {
      for(iterator i = begin(); i != end(); ++i)
      {
        ComponentWidget& cw = *i;
        if(cw._componentType == componentType)
        {
          if((componentWidgetType == -1 || cw._widgetType == componentWidgetType) &&
              (index == -1 || cw._index == index) && 
              (!widget || cw._widget == widget))
            return i;
        }
      }
      return end();
    }

    const_iterator find(ComponentWidget::ComponentType componentType, 
      int componentWidgetType = -1,
      int index = -1, const QWidget* widget = 0) const
    {
      for(const_iterator i = begin(); i != end(); ++i)
      {
        const ComponentWidget& cw = *i;
        if(cw._componentType == componentType)
        {
          if((componentWidgetType == -1 || cw._widgetType == componentWidgetType) && 
              (index == -1 || cw._index == index) && 
              (!widget || cw._widget == widget))
            return i;
        }
      }
    }
    
    iterator find(const ComponentWidget& cw)
    {
      return find(cw._componentType, cw._widgetType, cw._index, cw._widget);
    }
    
    // FIXME: I don't think this is calling the const version.
    const_iterator find(const ComponentWidget& cw) const
    {
      return find(cw._componentType, cw._widgetType, cw._index, cw._widget);
    }
};
typedef ComponentWidgetList::iterator iComponentWidget;
typedef ComponentWidgetList::const_iterator ciComponentWidget;


//---------------------------------------------
// ComponentDescriptor
//  Base class defining an item to be added to a rack layout.
//---------------------------------------------

class ComponentDescriptor
{
  public:
    // Type of widget to create.
    ComponentWidget::ComponentWidgetType _widgetType;
    
    // Type of component: Controller, gain property, aux property etc.
    ComponentWidget::ComponentType _componentType;
    // Object name to assign to the created widget.
    const char* _objName;
    
    // User index value (controller index, aux property index etc.) provided to given slots.
    int _index;

    QString _toolTipText; 
    QString _label; 
    QColor _color;
    
    bool _enabled;
    
    // Slots:
    const char* _changedSlot; 
    const char* _movedSlot; 
    const char* _pressedSlot; 
    const char* _releasedSlot; 
    const char* _rightClickedSlot;

  public:        
    ComponentDescriptor() :
                            _widgetType(0),
                            _componentType(0), 
                            _objName(0), 
                            _index(0), 
                            _enabled(true),
                            _changedSlot(0), _movedSlot(0), _pressedSlot(0), _releasedSlot(0), _rightClickedSlot(0)
                            { } 
                            
    ComponentDescriptor(ComponentWidget::ComponentWidgetType widgetType,
                        ComponentWidget::ComponentType type,
                        const char* objName = 0,
                        int index = 0,
                        const QString& toolTipText = QString(), 
                        const QString& label = QString(), 
                        const QColor& color = QColor(),
                        bool enabled = true,
                        const char* changedSlot = 0, 
                        const char* movedSlot = 0, 
                        const char* pressedSlot = 0, 
                        const char* releasedSlot = 0, 
                        const char* rightClickedSlot = 0
                        ) : 
                            _widgetType(widgetType),
                            _componentType(type),
                            _objName(objName), _index(index),
                            _toolTipText(toolTipText), 
                            _label(label),
                            _color(color),
                            _enabled(enabled),
                            _changedSlot(changedSlot), 
                            _movedSlot(movedSlot), 
                            _pressedSlot(pressedSlot),
                            _releasedSlot(releasedSlot), 
                            _rightClickedSlot(rightClickedSlot)
                            { }
};

class ComponentRackLayout : public QVBoxLayout
{
  Q_OBJECT
  
  public:
    ComponentRackLayout() : QVBoxLayout() { }
    ComponentRackLayout(QWidget* parent) : QVBoxLayout(parent) { }
};

//---------------------------------------------
// ComponentRack
//  Base class rack containing components.
//---------------------------------------------

class ComponentRack : public QFrame
{
  Q_OBJECT
  
  public:
    // Type of component.
    enum ComponentTypes { controllerComponent = 0, propertyComponent = 1, userComponent = 1000 };
    // Possible component properties.
    enum ComponentProperties { userComponentProperty = 1000 };
    // Possible widget types.
    enum ComponentWidgetTypes { ExternalComponentWidget = 0, CompactSliderComponentWidget = 1, ElidedLabelComponentWidget = 2, userComponentWidget = 1000 };
    
  protected:
    int _id;
    ComponentWidgetList _components;
    ComponentRackLayout* _layout;
    
    // Creates a new component widget from the given desc. Called by newComponent().
    // Connects known widget types' signals to slots.
    virtual void newComponentWidget( ComponentDescriptor* desc, const ComponentWidget& before = ComponentWidget() );

    // Adds a component widget created by newComponentWidget.
    void addComponentWidget( const ComponentWidget& cw, const ComponentWidget& before = ComponentWidget() );
    
  protected slots:
    virtual void controllerChanged(double /*val*/, bool /*isOff*/, int /*id*/, int /*scrollMode*/) { }
    virtual void controllerMoved(double /*val*/, int /*id*/, bool /*shift_pressed*/) { }
    virtual void controllerPressed(int /*id*/) { }
    virtual void controllerReleased(int /*id*/) { }
    virtual void controllerRightClicked(QPoint /*p*/, int /*id*/) { }
    virtual void propertyChanged(double /*val*/, bool /*isOff*/, int /*id*/, int /*scrollMode*/) { }
    virtual void propertyMoved(double /*val*/, int /*id*/, bool /*shift_pressed*/) { }
    virtual void propertyPressed(int /*id*/) { }
    virtual void propertyReleased(int /*id*/) { }
    virtual void propertyRightClicked(QPoint /*p*/, int /*id*/) { }
    virtual void labelPropertyPressed(QPoint /*p*/, int /*id*/, Qt::MouseButtons /*buttons*/, Qt::KeyboardModifiers /*keys*/) { }
    virtual void labelPropertyReleased(QPoint /*p*/, int /*id*/, Qt::MouseButtons /*buttons*/, Qt::KeyboardModifiers /*keys*/) { }
    
   public slots:
      virtual void songChanged(MusECore::SongChangedFlags_t) { }
      virtual void configChanged();
      
  public:
    ComponentRack(int id = -1, QWidget* parent = 0, Qt::WindowFlags f = 0);
    
    int id() const { return _id; }
    ComponentWidgetList* components() { return &_components; }
    
    // Adds a component to the layout and the list. Creates a new component using 
    //  the given desc values if the desc widget is not given.
    virtual void newComponent( ComponentDescriptor*, const ComponentWidget& /*before*/ = ComponentWidget() ) = 0;
    // Add a stretch to the layout.
    virtual void addStretch() { _layout->addStretch(); }
    // Add spacing to the layout.
    virtual void addSpacing(int spc) { _layout->addSpacing(spc); }
    virtual void setComponentRange(const ComponentWidget&, double min, double max, double step = 0.0, int pageSize = 1, 
                                   DoubleRange::ConversionMode mode = DoubleRange::ConvertDefault);
    virtual void setComponentMinValue(const ComponentWidget&, double min);
    virtual void setComponentMaxValue(const ComponentWidget&, double max);
    virtual double componentValue(const ComponentWidget&) const;
    virtual void setComponentValue(const ComponentWidget&, double val);
    virtual void setComponentText(const ComponentWidget&, const QString& text);
    virtual void setComponentEnabled(const ComponentWidget&, bool enable);
};

//---------------------------------------------
// WidgetComponentDescriptor
//  Class defining a general external QWidget item 
//   to be added to a rack layout.
//---------------------------------------------

class WidgetComponentDescriptor : public ComponentDescriptor
{
  public:
    // External widget pointer set by caller, corresponding to WidgetComponent type.
    QWidget* _widget;

  public:        
    WidgetComponentDescriptor() :
                            ComponentDescriptor(ComponentRack::ExternalComponentWidget,
                                                ComponentRack::propertyComponent),
                            _widget(0)
                            { }
                            
    WidgetComponentDescriptor(
                        QWidget* widget,
                        ComponentWidget::ComponentType componentType,
                        const char* objName = 0,
                        int index = 0,
                        const QString& toolTipText = QString(),
                        const QString& label = QString(),
                        const QColor& color = QColor(),
                        bool enabled = true,
                        const char* changedSlot = 0,
                        const char* movedSlot = 0,
                        const char* pressedSlot = 0,
                        const char* releasedSlot = 0,
                        const char* rightClickedSlot = 0
                        ) : 
                            ComponentDescriptor(ComponentRack::ExternalComponentWidget,
                                                componentType,
                                                objName,
                                                index,
                                                toolTipText,
                                                label,
                                                color,
                                                enabled,
                                                changedSlot,
                                                movedSlot,
                                                pressedSlot,
                                                releasedSlot,
                                                rightClickedSlot
                                               ),
                            _widget(widget)
                            { }
};


//---------------------------------------------
// CompactSliderComponentDescriptor
//  Class defining a CompactSlider to be added to a rack layout.
//---------------------------------------------

class CompactSliderComponentDescriptor : public ComponentDescriptor
{
  public:
    // Return value pointer created by the function, corresponding to a ComponentWidgetType:
    CompactSlider* _compactSlider;
    
    double _min;
    double _max; 
    int _precision;
    double _step; 
    double _initVal;
    bool _hasOffMode;
    bool _isOff;

    QColor _barColor;
    QColor _slotColor;
    QColor _thumbColor;
    QString _prefix; 
    QString _suffix; 
    QString _specialValueText; 
    
  public:        
    CompactSliderComponentDescriptor() :
      ComponentDescriptor(ComponentRack::CompactSliderComponentWidget,
                          ComponentRack::propertyComponent),
      _compactSlider(0),
      _min(0.0), _max(0.0), _precision(0), _step(1.0), _initVal(0.0), _hasOffMode(false), _isOff(false) { }
                            
    CompactSliderComponentDescriptor(
      ComponentWidget::ComponentType componentType,
      const char* objName = 0,
      int index = 0,
      const QString& toolTipText = QString(),
      const QString& label = QString(),
      const QColor& borderColour = QColor(),
      const QColor& barColour = QColor(),
      const QColor& slotColour = QColor(),
      const QColor& thumbColour = QColor(),
      const QString& prefix = QString(),
      const QString& suffix = QString(),
      const QString& specialValueText = QString(),
      bool enabled = true,
      double min = 0.0,
      double max = 100.0,
      int precision = 0,
      double step = 1.0,
      double initVal = 0.0,
      bool hasOffMode = false,
      bool isOff = false,
      const char* changedSlot = 0,
      const char* movedSlot = 0,
      const char* pressedSlot = 0,
      const char* releasedSlot = 0,
      const char* rightClickedSlot = 0
    )
    : ComponentDescriptor(ComponentRack::CompactSliderComponentWidget,
                          componentType,
                          objName,
                          index,
                          toolTipText,
                          label,
                          borderColour,
                          enabled,
                          changedSlot,
                          movedSlot,
                          pressedSlot,
                          releasedSlot,
                          rightClickedSlot
                         ),
      _compactSlider(0),
      _min(min), 
      _max(max), 
      _precision(precision), 
      _step(step), 
      _initVal(initVal),
      _hasOffMode(hasOffMode),
      _isOff(isOff),
      _barColor(barColour),
      _slotColor(slotColour),
      _thumbColor(thumbColour),
      _prefix(prefix),
      _suffix(suffix),
      _specialValueText(specialValueText) { }
};

//---------------------------------------------
// ElidedLabelComponentDescriptor
//  Class defining a ElidedLabel to be added to a rack layout.
//---------------------------------------------

class ElidedLabelComponentDescriptor : public ComponentDescriptor
{
  public:
    // Return value pointer created by the function, corresponding to a ComponentWidgetType:
    ElidedLabel* _elidedLabel;
    
    Qt::TextElideMode _elideMode; 
    int _minFontPoint;
    bool _ignoreHeight;
    bool _ignoreWidth;

    // Slots:
    const char* _labelPressedSlot; 
    const char* _labelReleasedSlot; 
    
  public:        
    ElidedLabelComponentDescriptor() : 
      ComponentDescriptor(ComponentRack::ElidedLabelComponentWidget,
                          ComponentRack::propertyComponent), 
      _elidedLabel(0),
      _elideMode(Qt::ElideNone), _minFontPoint(5), _ignoreHeight(true), _ignoreWidth(false),
      _labelPressedSlot(0), _labelReleasedSlot(0) { }
                            
    ElidedLabelComponentDescriptor(
      ComponentWidget::ComponentType componentType,
      const char* objName = 0,
      int index = 0,
      Qt::TextElideMode elideMode = Qt::ElideNone,
      const QString& toolTipText = QString(),
      const QString& text = QString(), 
      const QColor& borderColour = QColor(),
      int minFontPoint = 5,
      bool ignoreHeight = true, 
      bool ignoreWidth = false,
      bool enabled = true,
      const char* labelPressedSlot = 0,
      const char* labelReleasedSlot = 0
    )
    : ComponentDescriptor(ComponentRack::ElidedLabelComponentWidget,
                          componentType,
                          objName,
                          index,
                          toolTipText,
                          text,
                          borderColour,
                          enabled
                         ),
      _elidedLabel(0),
      _elideMode(elideMode), 
      _minFontPoint(minFontPoint), 
      _ignoreHeight(ignoreHeight), 
      _ignoreWidth(ignoreWidth),
      _labelPressedSlot(labelPressedSlot),
      _labelReleasedSlot(labelReleasedSlot)
       { }
};

struct GridPosStruct
{
  int _row;
  int _col;
  int _rowSpan;
  int _colSpan;
  
  GridPosStruct() : _row(0), _col(0), _rowSpan(0), _colSpan(0) { }
  GridPosStruct(int row, int col, int rowSpan, int colSpan)
              : _row(row), _col(col), _rowSpan(rowSpan), _colSpan(colSpan) { }
};

//---------------------------------------------------------
//   ExpanderHandle
//---------------------------------------------------------

class ExpanderHandle : public QFrame 
{
  Q_OBJECT

  protected:
    enum ResizeMode { ResizeModeNone, ResizeModeHovering, ResizeModeDragging };
    
  private:
    int _handleWidth;
    ResizeMode _resizeMode;
    QPoint _dragLastGlobPos;
      
  protected:
    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    //virtual void leaveEvent(QEvent*);
    virtual QSize sizeHint() const;

  signals:
    void moved(int xDelta);
    
  public:
    ExpanderHandle(QWidget * parent = 0, int handleWidth = 4, Qt::WindowFlags f = 0);
};

//---------------------------------------------------------
//   Strip
//---------------------------------------------------------

class Strip : public QFrame {
      Q_OBJECT

   private:
      QPoint mouseWidgetOffset;
      bool dragOn;
      bool _visible;
   
   protected:
      
      MusECore::Track* track;
      QLabel* label;
      QGridLayout* grid;
      int _curGridRow;
      Meter* meter[MAX_CHANNELS];
      // Extra width applied to the sizeHint, from user expanding the strip.
      int _userWidth;
      ExpanderHandle* _handle;
      
      QToolButton* record;
      QToolButton* solo;
      QToolButton* mute;
      QToolButton* iR; // Input routing button
      QToolButton* oR; // Output routing button
      QGridLayout* sliderGrid;
      CompactComboBox* autoType;
      void setLabelText();
      virtual void resizeEvent(QResizeEvent*);
      virtual void mousePressEvent(QMouseEvent *);
      virtual void mouseReleaseEvent(QMouseEvent *);
      virtual void mouseMoveEvent(QMouseEvent *);

      virtual void updateRouteButtons();

   private slots:
      void recordToggled(bool);
      void soloToggled(bool);
      void muteToggled(bool);

   protected slots:
      virtual void heartBeat();
      void setAutomationType(int t);

   public slots:
      void resetPeaks();
      virtual void songChanged(MusECore::SongChangedFlags_t) = 0;
      virtual void configChanged() = 0;
      virtual void changeUserWidth(int delta);

   public:
      Strip(QWidget* parent, MusECore::Track* t, bool hasHandle = false);
      ~Strip();

      bool getStripVisible() { return _visible; }
      void setStripVisible(bool v) { _visible = v; }

      static const int FIXED_METER_WIDTH;
      
      void setRecordFlag(bool flag);
      MusECore::Track* getTrack() const { return track; }
      void setLabelFont();
      QString getLabelText() { return label->text(); }
      
      void addGridWidget(QWidget* w, const GridPosStruct& pos, Qt::Alignment alignment = 0);
      void addGridLayout(QLayout* l, const GridPosStruct& pos, Qt::Alignment alignment = 0);
      
      int userWidth() const { return _userWidth; }
      void setUserWidth(int w);
      
      virtual QSize sizeHint() const;
      };

} // namespace MusEGui

#endif

