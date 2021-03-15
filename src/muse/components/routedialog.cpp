//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: routedialog.cpp,v 1.5.2.2 2007/01/04 00:35:17 terminator356 Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2015 Tim E. Real (terminator356 on sourceforge)
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

#include <QVector>
#include <QList>
#include <QPalette>
#include <QRect>
#include <QPoint>
#include <QModelIndex>
#include <QHeaderView>
#include <QVariant>
#include <QPainterPath>

#include "routedialog.h"
#include "globaldefs.h"
#include "globals.h"
#include "gconfig.h"
#include "midiport.h"
#include "track.h"
#include "song.h"
#include "audio.h"
#include "driver/jackaudio.h"
#include "globaldefs.h"
#include "app.h"
#include "operations.h"
#include "icons.h"

// Forwards from header:
#include <QCloseEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QContextMenuEvent>

// For debugging output: Uncomment the fprintf section.
#define DEBUG_PRST_ROUTES(dev, format, args...) // fprintf(dev, format, ##args);

// Undefine if and when multiple output routes are added to midi tracks.
#define _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_

// An arbitrarily large value for size hint calculations.
#define _VERY_LARGE_INTEGER_ 1000000

namespace MusEGui {

const QString RouteDialog::tracksCat(QObject::tr("Tracks:"));
const QString RouteDialog::midiPortsCat(QObject::tr("Midi ports:"));
const QString RouteDialog::midiDevicesCat(QObject::tr("Midi devices:"));
const QString RouteDialog::jackCat(QObject::tr("Jack:"));
const QString RouteDialog::jackMidiCat(QObject::tr("Jack midi:"));

const int RouteDialog::channelDotDiameter = 12;
const int RouteDialog::channelDotSpacing = 1;
const int RouteDialog::channelDotsPerGroup = 4;
const int RouteDialog::channelDotGroupSpacing = 3;
const int RouteDialog::channelDotsMargin = 1;
const int RouteDialog::channelBarHeight = RouteDialog::channelDotDiameter + 2 * RouteDialog::channelDotsMargin;
const int RouteDialog::channelLineWidth = 1;
const int RouteDialog::channelLinesSpacing = 1;
const int RouteDialog::channelLinesMargin = 1;

std::list<QString> tmpJackInPorts;
std::list<QString> tmpJackOutPorts;
std::list<QString> tmpJackMidiInPorts;
std::list<QString> tmpJackMidiOutPorts;

//---------------------------------------------------------
//   RouteChannelsList
//---------------------------------------------------------

int RouteChannelsList::connectedChannels() const
{
  int n = 0;
  const int sz = size();
  for(int i = 0; i < sz; ++i)
    if(at(i)._connected)
      ++n;
  return n;
}

// Static.
int RouteChannelsList::channelsPerWidth(int width)
{
//   if(width <= 0)
//     return size();
  if(width < 0)
    width = _VERY_LARGE_INTEGER_;
  
  int groups_per_col = (width - 2 * RouteDialog::channelDotsMargin) / 
                        (RouteDialog::channelDotGroupSpacing + 
                         RouteDialog::channelDotsPerGroup * (RouteDialog::channelDotDiameter + RouteDialog::channelDotSpacing));
  if(groups_per_col < 1)
    groups_per_col = 1;
  return RouteDialog::channelDotsPerGroup * groups_per_col;
}

// Static.
int RouteChannelsList::groupsPerChannels(int channels)
{
  
  int groups = channels / RouteDialog::channelDotsPerGroup;
  //if(groups < 1)
  //  groups = 1;
  if(channels % RouteDialog::channelDotsPerGroup)
    ++groups;
  return groups;
}

int RouteChannelsList::barsPerColChannels(int cc) const
{
  if(cc == 0)
    return 0;
  const int chans = size();
  int bars = chans / cc;
  if(chans % cc)
    ++bars;
  //if(chan_rows < 1)
  //  chan_rows = 1;
  return bars;
}

// Static.
int RouteChannelsList::minimumWidthHint()
{
  return RouteDialog::channelDotsPerGroup * (RouteDialog::channelDotDiameter + RouteDialog::channelDotSpacing) +
         RouteDialog::channelDotGroupSpacing +
         2 * RouteDialog::channelDotsMargin;
}

int RouteChannelsList::widthHint(int width) const
{
  const int chans = size();
  int chans_per_col = channelsPerWidth(width);
  // Limit to actual number of channels available.
  if(chans_per_col > chans)
    chans_per_col = chans;
  const int groups_per_col = groupsPerChannels(chans_per_col);
  return chans_per_col * (RouteDialog::channelDotDiameter + RouteDialog::channelDotSpacing) +
         groups_per_col * RouteDialog::channelDotGroupSpacing +
         2 * RouteDialog::channelDotsMargin;
}

int RouteChannelsList::heightHint(int width) const
{
  const int chans = size();
  int chans_per_col = channelsPerWidth(width);
  // Limit to actual number of channels available.
  if(chans_per_col > chans)
    chans_per_col = chans;
  const int bars = barsPerColChannels(chans_per_col);
  return bars * RouteDialog::channelBarHeight + 
         connectedChannels() * (RouteDialog::channelLinesSpacing + RouteDialog::channelLineWidth) + 
         4 * RouteDialog::channelLinesMargin;
}

//---------------------------------------------------------
//   RouteTreeWidgetItem
//---------------------------------------------------------

void RouteTreeWidgetItem::init()
{
  _curChannel = 0;
  setChannels();
  //computeChannelYValues();
  
  // A data role to pass the item type from item to delegate.
  //setData(RouteDialog::ROUTE_NAME_COL, TypeRole, QVariant::fromValue<int>(type()));
}

bool RouteTreeWidgetItem::setChannels()
{
  bool changed = false;
  
  switch(type())
  {
    case NormalItem:
    case CategoryItem:
    case RouteItem:
    break;
    
    case ChannelsItem:
      switch(_route.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          if(_route.track)
          {
            MusECore::RouteCapabilitiesStruct rcaps = _route.track->routeCapabilities();
            int chans = 0;
            switch(_route.track->type())
            {
              case MusECore::Track::AUDIO_INPUT:
                chans = _isInput ? rcaps._trackChannels._outChannels : rcaps._jackChannels._inChannels;
              break;
              case MusECore::Track::AUDIO_OUTPUT:
                chans = _isInput ? rcaps._jackChannels._outChannels : rcaps._trackChannels._inChannels;
              break;
              case MusECore::Track::MIDI:
              case MusECore::Track::DRUM:
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
                chans = _isInput ? MusECore::MUSE_MIDI_CHANNELS : rcaps._midiPortChannels._inChannels;
#else                  
                chans = _isInput ? rcaps._midiPortChannels._outChannels : rcaps._midiPortChannels._inChannels;
#endif              
              break;
                
              case MusECore::Track::WAVE:
              case MusECore::Track::AUDIO_AUX:
              case MusECore::Track::AUDIO_SOFTSYNTH:
              case MusECore::Track::AUDIO_GROUP:
                chans = _isInput ? rcaps._trackChannels._outChannels : rcaps._trackChannels._inChannels;
              break;
            }
            
            if(chans != _channels.size())
            {
              _channels.resize(chans);
              changed = true;
            }
          }
        break;  

        case MusECore::Route::JACK_ROUTE:  
        case MusECore::Route::MIDI_DEVICE_ROUTE:  
        case MusECore::Route::MIDI_PORT_ROUTE:
        break;  
      }
    break;  
  }
  
  
  if(changed)
  {
    _curChannel = 0;
    //computeChannelYValues();
  }

  return changed;
}

void RouteTreeWidgetItem::getSelectedRoutes(MusECore::RouteList& routes)
{
  switch(type())
  {
    case NormalItem:
    case CategoryItem:
    break;  
    case RouteItem:
      if(isSelected())
        routes.push_back(_route);
    break;  
    case ChannelsItem:
      switch(_route.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          if(_route.track)
          {
            MusECore::Route r(_route);
            const int sz = _channels.size();
            if(_route.track->isMidiTrack())
            {  
              for(int i = 0; i < sz && i < MusECore::MUSE_MIDI_CHANNELS; ++i)
              {
                //if(_channels.testBit(i))
                if(_channels.selected(i))
                {
                  //r.channel = (1 << i);
                  r.channel = i;
                  routes.push_back(r);
                }
              }
            }
            else
            {
              for(int i = 0; i < sz; ++i)
              {
                //if(_channels.testBit(i))
                if(_channels.selected(i))
                {
                  r.channel = i;
                  routes.push_back(r);
                }
              }
            }
          }
        break;
        case MusECore::Route::JACK_ROUTE:
        case MusECore::Route::MIDI_DEVICE_ROUTE:
        case MusECore::Route::MIDI_PORT_ROUTE:
          if(isSelected())
            routes.push_back(_route);
        break;
      }
      
    break;  
  }
}

int RouteTreeWidgetItem::channelAt(const QPoint& pt, const QRect& rect) const
{
//   if(!treeWidget()->viewport())
//     return false;
  
  RouteTreeWidget* rtw = qobject_cast<RouteTreeWidget*>(treeWidget());
  if(!rtw)
    return false;
  
  const int col = rtw->columnAt(pt.x());
  const int col_width = rtw->columnWidth(col); 
  //const int view_width = rtw->viewport()->width();
  const int chans = _channels.size();
  const int view_offset = rtw->header()->offset();
//   const int x_offset = (_isInput ? view_width - _channels.widthHint(view_width) - view_offset : -view_offset);
  const int x_offset = (_isInput ? 
                        //view_width - _channels.widthHint(rtw->wordWrap() ? view_width : -1) - view_offset : -view_offset);
                        col_width - _channels.widthHint(rtw->channelWrap() ? col_width : -1) - view_offset : -view_offset);

  QPoint p(pt.x() - x_offset, pt.y() - rect.y());
  
  DEBUG_PRST_ROUTES(stderr, "RouteTreeWidgetItem::channelAt() pt x:%d y:%d rect x:%d y:%d w:%d h:%d view_offset:%d x_offset:%d col w:%d header w:%d view w:%d p x:%d y:%d\n", 
          pt.x(), pt.y(), rect.x(), rect.y(), rect.width(), rect.height(), view_offset, x_offset, 
          rtw->columnWidth(col), rtw->header()->sectionSize(col), view_width, p.x(), p.y());  
  
  for(int i = 0; i < chans; ++i)
  {
    const RouteChannelsStruct& ch_struct = _channels.at(i);
    const QRect& ch_rect = ch_struct._buttonRect;
    if(ch_rect.contains(p))
      return i;
  }
  return -1;
}  

void RouteTreeWidgetItem::computeChannelYValues(int col_width)
{
  //_channelYValues.resize();
  if(type() != ChannelsItem)
    return;
  //_channelYValues.fill(-1);
  _channels.fillConnected(false);
  switch(_route.type)
  {
    case MusECore::Route::TRACK_ROUTE:
      if(_route.track)
      {
        //_channelYValues.fill(-1);
        //_channels.fillConnected(false);
        
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
        if(_isInput && _route.track->isMidiTrack())
          _channels.setConnected(static_cast<MusECore::MidiTrack*>(_route.track)->outChannel(), true);
        else
#endif          
        {
          const MusECore::RouteList* rl = _isInput ? _route.track->outRoutes() : _route.track->inRoutes();
          for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
          {
            switch(ir->type)
            {
              case MusECore::Route::TRACK_ROUTE:
                //if(ir->track && ir->channel != -1)
                if(ir->channel != -1)
                {
                  // Mark the channel as used.
                  _channels.setConnected(ir->channel, true);
                }
              break;

              case MusECore::Route::MIDI_PORT_ROUTE:
                if(ir->isValid() && ir->channel != -1)
                {
                  _channels.setConnected(ir->channel, true);
                }
              break;

              case MusECore::Route::JACK_ROUTE:
                if(ir->channel != -1)
                  _channels.setConnected(ir->channel, true);
              break;

              case MusECore::Route::MIDI_DEVICE_ROUTE:
              break;
            }
          }
        }
      }
    break;

    case MusECore::Route::JACK_ROUTE:
    case MusECore::Route::MIDI_DEVICE_ROUTE:
    case MusECore::Route::MIDI_PORT_ROUTE:
    break;
  }

  const int chans = _channels.size();
  int chans_per_w = _channels.channelsPerWidth(col_width);
  // Limit to actual number of channels available.
  if(chans_per_w > chans)
    chans_per_w = chans;

  //DEBUG_PRST_ROUTES(stderr, "RoutingItemDelegate::paint src list width:%d src viewport width:%d\n",
  //    router->newSrcList->width(), router->newSrcList->viewport()->width());  

  const int x_orig = RouteDialog::channelDotsMargin;
  int x = x_orig;
  int chan_y = 2 * RouteDialog::channelDotsMargin;

  DEBUG_PRST_ROUTES(stderr, "RouteTreeWidgetItem::computeChannelYValues() col_width:%d chans_per_w:%d\n", col_width, chans_per_w);  
  
  int line_y = 2 * RouteDialog::channelLinesMargin + 
    (_isInput ? 0 : (RouteDialog::channelBarHeight + RouteDialog::channelDotsMargin + RouteDialog::channelLinesMargin));

  int cur_chan = 0;
  for(int i = 0; i < chans; )
  {
    const bool is_connected = _channels.at(i)._connected;
    
    if(is_connected)
    {
      // Replace the value with an appropriate y value useful for drawing channel lines.
      _channels[i]._lineY = line_y;
    }
    
    if(!_isInput)
      _channels[i]._buttonRect = QRect(x, chan_y, RouteDialog::channelDotDiameter, RouteDialog::channelDotDiameter);
    
    ++i;
    const bool new_group = (i % RouteDialog::channelDotsPerGroup == 0);
    const bool new_section = (i % chans_per_w == 0);

    if(is_connected)
      line_y += RouteDialog::channelLineWidth + RouteDialog::channelLinesSpacing;
    
    if(_isInput)
    {
      // If we finished a section set button rects, or we reached the end
      //  set the remaining button rects, based on current line y (and x).
      if(new_section || i == chans)
      {
        x = x_orig;
        for( ; cur_chan < i; )
        {
          DEBUG_PRST_ROUTES(stderr, "RouteTreeWidgetItem::computeChannelYValues() i:%d cur_chan:%d x:%d\n", i, cur_chan, x);  
          _channels[cur_chan]._buttonRect = QRect(x, line_y + RouteDialog::channelLinesMargin, RouteDialog::channelDotDiameter, RouteDialog::channelDotDiameter);
          ++cur_chan;
          x += RouteDialog::channelDotDiameter + RouteDialog::channelDotSpacing;
          if(cur_chan % RouteDialog::channelDotsPerGroup == 0)
            x += RouteDialog::channelDotGroupSpacing;
        }
      }
    }

    if(new_section)
    {
      x = x_orig;  // Reset
      chan_y = line_y;
      line_y += RouteDialog::channelBarHeight;
    }
    else
    {
      x += RouteDialog::channelDotDiameter + RouteDialog::channelDotSpacing;
      if(new_group)
        x += RouteDialog::channelDotGroupSpacing;
    }
  }
}

bool RouteTreeWidgetItem::mousePressHandler(QMouseEvent* e, const QRect& rect)
{
  const QPoint pt = e->pos(); 
  const Qt::KeyboardModifiers km = e->modifiers();
  bool ctl = false;
  switch(_itemMode)
  {
    case ExclusiveMode:
      ctl = false;
    break;
    case NormalMode:
      ctl = km & Qt::ControlModifier;
    break;
  }
  //bool shift = km & Qt::ShiftModifier;

//   RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(itemAt(pt));
//   bool is_cur = item && currentItem() && (item == currentItem());

  //if(is_cur)
  //  QTreeWidget::mousePressEvent(e);
  
  switch(type())
  {
    case NormalItem:
    case CategoryItem:
    case RouteItem:
    break;  
    case ChannelsItem:
      switch(_route.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          if(_route.track && _route.channel != -1)
          {
            int ch = channelAt(pt, rect);
            
            const int ba_sz = _channels.size();
            bool changed = false;
            //if(!ctl)
            {
              for(int i = 0; i < ba_sz; ++i)
              {
                if(i == ch)
                {
                  if(ctl)
                  {
                    _channels[i].toggleSelected();
                    changed = true;
                  }
                  else
                  {
                    if(!_channels.at(i)._selected)
                      changed = true;
                    _channels[i]._selected = true;
                  }
                }
                else if(!ctl)
                {
                  if(_channels. at(i)._selected)
                    changed = true;
                  _channels[i]._selected = false;
                }
              }
            }

            //e->accept();
            return changed;
          }
        break;
        case MusECore::Route::JACK_ROUTE:
        case MusECore::Route::MIDI_DEVICE_ROUTE:
        case MusECore::Route::MIDI_PORT_ROUTE:
        break;
      }
      
    break;  
  }

  return false;
  
//   QTreeWidget::mousePressEvent(e);
}

bool RouteTreeWidgetItem::mouseMoveHandler(QMouseEvent* e, const QRect& rect)
{
  const Qt::MouseButtons mb = e->buttons();
  if(mb != Qt::LeftButton)
    return false;
  
  const QPoint pt = e->pos(); 
  const Qt::KeyboardModifiers km = e->modifiers();
  
  bool ctl = false;
  switch(_itemMode)
  {
    case ExclusiveMode:
      ctl = false;
    break;
    case NormalMode:
      //ctl = true;
      //ctl = km & Qt::ControlModifier;
      ctl = km & Qt::ShiftModifier;
    break;
  }
  //bool shift = km & Qt::ShiftModifier;

//   RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(itemAt(pt));
//   bool is_cur = item && currentItem() && (item == currentItem());

  //if(is_cur)
  //  QTreeWidget::mousePressEvent(e);
  
  switch(type())
  {
    case NormalItem:
    case CategoryItem:
    case RouteItem:
    break;  
    case ChannelsItem:
      switch(_route.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          if(_route.track && _route.channel != -1)
          {
            int ch = channelAt(pt, rect);
            
            const int ba_sz = _channels.size();
            bool changed = false;
            for(int i = 0; i < ba_sz; ++i)
            {
              if(i == ch)
              {
                {
                  if(!_channels.at(i)._selected)
                    changed = true;
                  _channels[i]._selected = true;
                }
              }
              else if(!ctl)
              {
                if(_channels. at(i)._selected)
                  changed = true;
                _channels[i]._selected = false;
              }
            }
            return changed;
          }
        break;
        case MusECore::Route::JACK_ROUTE:
        case MusECore::Route::MIDI_DEVICE_ROUTE:
        case MusECore::Route::MIDI_PORT_ROUTE:
        break;
      }
      
    break;  
  }

  return false;
  
//   QTreeWidget::mousePressEvent(e);
}

bool RouteTreeWidgetItem::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  {
    if(index.column() == RouteDialog::ROUTE_NAME_COL)
    {
      RouteTreeWidget* rtw = qobject_cast<RouteTreeWidget*>(treeWidget());
      if(!rtw)
        return false;
      
      switch(type())
      {
        case ChannelsItem:
        {
          if(!treeWidget()->viewport())
            return false;

          const int col_width = rtw->columnWidth(index.column()); 
          const int view_width = rtw->viewport()->width();
          const int chans = _channels.size();
          const int view_offset = rtw->header()->offset();
          const int x_offset = (_isInput ? 
                                //view_width - _channels.widthHint(rtw->wordWrap() ? view_width : -1) - view_offset : -view_offset);
                                col_width - _channels.widthHint(rtw->channelWrap() ? col_width : -1) - view_offset : -view_offset);

          DEBUG_PRST_ROUTES(stderr, "RouteTreeWidgetItem::paint() rect x:%d y:%d w:%d h:%d view_offset:%d x_offset:%d dev w:%d col w:%d header w:%d view w:%d\n", 
                  option.rect.x(), option.rect.y(), option.rect.width(), option.rect.height(), view_offset, x_offset, painter->device()->width(), 
                  rtw->columnWidth(index.column()), rtw->header()->sectionSize(index.column()), view_width);  
      
          
          // From QStyledItemDelegate::paint help: Necessary?
          // "After painting, you should ensure that the painter is returned to its the state it was supplied in when this function
          //  was called. For example, it may be useful to call QPainter::save() before painting and QPainter::restore() afterwards."
          painter->save();
          
          // Need to be able to paint beyond the right edge of the column width, 
          //  all the way to the view's right edge.
          //painter->setClipRect(option.rect);
          QRect clip_rect(option.rect);
          clip_rect.setWidth(view_width - option.rect.x());
          painter->setClipRect(clip_rect);
          
          if(index.parent().isValid() && (index.parent().row() & 0x01))
            painter->fillRect(option.rect, option.palette.alternateBase());
          int cur_chan = 0;
          QPen pen;

          // Set a small five-pixel font size for the numbers inside the dots.
          QFont fnt = font(index.column());
          //fnt.setStyleStrategy(QFont::NoAntialias);
          //fnt.setStyleStrategy(QFont::PreferBitmap);
          // -2 for the border, -2 for the margin, and +1 for setPixelSize which seems to like it.
          //fnt.setPixelSize(RouteDialog::channelDotDiameter - 2 - 2 + 1); 
          fnt.setPixelSize(RouteDialog::channelDotDiameter / 2 + 1); 
          painter->setFont(fnt);
          for(int i = 0; i < chans; ++i)
          {
            const RouteChannelsStruct& ch_struct = _channels.at(i);
            const QRect& ch_rect = ch_struct._buttonRect;
            
            QPainterPath path;
            path.addRoundedRect(x_offset + ch_rect.x(), option.rect.y() + ch_rect.y(), 
                                ch_rect.width(), ch_rect.height(), 
                                30, 30);
            if(ch_struct._selected)
              painter->fillPath(path, option.palette.highlight());
            //painter->setPen(ch_struct._selected ? option.palette.highlightedText().color() : option.palette.text().color());
            //painter->setPen(ch_struct._routeSelected ? Qt::yellow : option.palette.text().color());
            painter->setPen(option.palette.text().color());
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->drawPath(path);

            //const int ch_num = (i + 1) % 10;
            if(chans > RouteDialog::channelDotsPerGroup)
            {
              //if((i % RouteDialog::channelDotsPerGroup) == 0 || ((i + 1) % 10 == 0))
              if((i % RouteDialog::channelDotsPerGroup) == 0)
              {
                painter->setPen(ch_struct._selected ? option.palette.highlightedText().color() : option.palette.text().color());
                painter->drawText(x_offset + ch_rect.x(), option.rect.y() + ch_rect.y(), 
                                 ch_rect.width(), ch_rect.height(), 
                                 Qt::AlignCenter, 
                                 //QString::number((ch_num + 1) / 10));
                                 QString::number(i + 1));
              }
            }
            
            if(ch_struct._connected)
            {
              // Need sharp lines here.
              painter->setRenderHint(QPainter::Antialiasing, false);
              
              const int line_x = x_offset + ch_rect.x() + RouteDialog::channelDotDiameter / 2;
              const int line_y = option.rect.y() + ch_struct._lineY;
              if(_isInput)
              {
                const int ch_y = option.rect.y() + ch_rect.y() -1;
                DEBUG_PRST_ROUTES(stderr, "RouteTreeWidgetItem::paint() input: line_x:%d ch_y:%d line_y:%d view_w:%d\n", line_x, ch_y, line_y, view_width);
                pen.setBrush((ch_struct._selected && !ch_struct._routeSelected) ? option.palette.highlight() : option.palette.text());
                pen.setStyle(Qt::SolidLine);
                painter->setPen(pen);
                painter->drawLine(line_x, ch_y, line_x, line_y);
                painter->drawLine(line_x, line_y, view_width, line_y);
                if(ch_struct._routeSelected)
                {
                  pen.setBrush(Qt::yellow);
                  pen.setStyle(Qt::DotLine);
                  painter->setPen(pen);
                  painter->drawLine(line_x, ch_y, line_x, line_y);
                  painter->drawLine(line_x, line_y, view_width, line_y);
                }
              }
              else
              {
                const int ch_y = option.rect.y() + ch_rect.y() + ch_rect.height();
                pen.setBrush((ch_struct._selected && !ch_struct._routeSelected) ? option.palette.highlight() : option.palette.text());
                pen.setStyle(Qt::SolidLine);
                painter->setPen(pen);
                painter->drawLine(line_x, ch_y, line_x, line_y);
                painter->drawLine(x_offset, line_y, line_x, line_y);
                if(ch_struct._routeSelected)
                {
                  pen.setBrush(Qt::yellow);
                  pen.setStyle(Qt::DotLine);
                  painter->setPen(pen);
                  painter->drawLine(line_x, ch_y, line_x, line_y);
                  painter->drawLine(x_offset, line_y, line_x, line_y);
                }
              }
              ++cur_chan;
            }
          }
          painter->restore();
          return true;
        }
        break;
        
        case CategoryItem:
        case RouteItem:
        {
          if(const QStyle* st = rtw->style())
          {
            st = st->proxy();
            painter->save();
            painter->setClipRect(option.rect);
            
            const QRect cb_rect = st->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &option);
            const QRect ico_rect = st->subElementRect(QStyle::SE_ItemViewItemDecoration, &option);
            const QRect text_rect = st->subElementRect(QStyle::SE_ItemViewItemText, &option);

            // Draw the row background (alternating colours etc.)
            QPalette::ColorGroup cg = (/* widget ? widget->isEnabled() : */ (option.state & QStyle::State_Enabled)) ?
                                      QPalette::Normal : QPalette::Disabled;
            if(cg == QPalette::Normal && !(option.state & QStyle::State_Active))
              cg = QPalette::Inactive;

            if(!(flags() & Qt::ItemIsSelectable))
            {
              // If it's not selectable then it must be a category item. Draw the category label background.
              // To support stylesheets, categoryColor is a property.
              // TODO: Support live stylesheet editing: How to reset to default if the stylesheet
              //        or the 'categoryColor' statement goes away? We'll need to set up
              //        some kind of notification BEFORE we set the app's stylesheet.
              const QColor cat_col = rtw->categoryColor();
              if(cat_col.isValid())
                painter->fillRect(option.rect, cat_col);
              else
                painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Mid));
            }
            else if((option.state & QStyle::State_Selected) &&
                    st->styleHint(QStyle::SH_ItemView_ShowDecorationSelected, &option /*, widget*/))
              painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
            //else if(option.features & QStyleOptionViewItem::Alternate)
            // Hm, something else draws the alternating colours, no control over it here. 
            // Disabled it in the UI so it does not interfere here.
            //else if(treeWidget()->alternatingRowColors() && (index.row() & 0x01))
            else if((index.row() & 0x01))
              painter->fillRect(option.rect, option.palette.brush(cg, QPalette::AlternateBase));

            // Draw the item background.
            st->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter);
            
            // Draw the check mark
            if(option.features & QStyleOptionViewItem::HasCheckIndicator) 
            {
              QStyleOptionViewItem opt(option);
              opt.rect = cb_rect;
              opt.state = opt.state & ~QStyle::State_HasFocus;
              switch(option.checkState) 
              {
                case Qt::Unchecked:
                    opt.state |= QStyle::State_Off;
                break;
                case Qt::PartiallyChecked:
                    opt.state |= QStyle::State_NoChange;
                break;
                case Qt::Checked:
                    opt.state |= QStyle::State_On;
                break;
              }
              st->drawPrimitive(QStyle::PE_IndicatorViewItemCheck, &opt, painter);
            }
            
            // Draw the icon.
            QIcon::Mode mode = QIcon::Normal;
            if(!(option.state & QStyle::State_Enabled))
              mode = QIcon::Disabled;
            else if(option.state & QStyle::State_Selected)
              mode = QIcon::Selected;
            QIcon::State state = option.state & QStyle::State_Open ? QIcon::On : QIcon::Off;
            option.icon.paint(painter, ico_rect, option.decorationAlignment, mode, state);
            
            // Draw the text.
            st->drawItemText(painter, 
                              text_rect, 
                              //textAlignment(index.column()) | Qt::TextWordWrap | Qt::TextWrapAnywhere, 
                              option.displayAlignment | (rtw->wordWrap() ? (Qt::TextWordWrap | Qt::TextWrapAnywhere) : 0), 
                              //treeWidget()->palette(), 
                              option.palette, 
                              //!isDisabled(), 
                              option.state & QStyle::State_Enabled, 
                              //text(index.column()),
                              rtw->wordWrap() ? 
                                option.text : option.fontMetrics.elidedText(option.text, rtw->textElideMode(), text_rect.width()),
                              //isSelected() ? QPalette::HighlightedText : QPalette::Text
                              (option.state & QStyle::State_Selected) ? QPalette::HighlightedText : QPalette::Text
                              );
            
            // Draw the focus.
            if(option.state & QStyle::State_HasFocus)
            {
              QStyleOptionFocusRect o;
              o.QStyleOption::operator=(option);
              o.rect = st->subElementRect(QStyle::SE_ItemViewItemFocusRect, &option);
              o.state |= QStyle::State_KeyboardFocusChange;
              o.state |= QStyle::State_Item;
              QPalette::ColorGroup cg = 
                                  (option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
              o.backgroundColor = option.palette.color(cg, 
                                  (option.state & QStyle::State_Selected) ? QPalette::Highlight : QPalette::Window);
              st->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter);
            }
            
            painter->restore();
            return true;
          }
        }
        break;
        
        case NormalItem:
        break;
      }
    }
  }
  return false;
}

QSize RouteTreeWidgetItem::getSizeHint(int column, int width) const
{
  DEBUG_PRST_ROUTES(stderr, "RouteTreeWidgetItem::getSizeHint width:%d col:%d column width:%d\n",
           treeWidget()->width(), column, treeWidget()->columnWidth(column));
  
  if(column == RouteDialog::ROUTE_NAME_COL) 
  {
    RouteTreeWidget* rtw = qobject_cast<RouteTreeWidget*>(treeWidget());
    if(!rtw)
      return QSize();
    
    switch(type())
    {
      case NormalItem:
      break;
      
      case ChannelsItem:
        //fprintf(stderr, "RouteTreeWidgetItem::getSizeHint ChannelsItem w:%d\n", width);
        return _channels.sizeHint(rtw->channelWrap() ? width : -1);
      break;
      
      case CategoryItem:
      case RouteItem:
      {
        if(!rtw->wordWrap())
          return QSize();
        
        if(const QStyle* st = rtw->style())
        {
          st = st->proxy();
          QStyleOptionViewItem vopt;
          vopt.features = QStyleOptionViewItem::None;
          
          vopt.text = text(column);
          vopt.rect = QRect(0, 0, rtw->wordWrap() ? width : _VERY_LARGE_INTEGER_, -1);
          vopt.displayAlignment = Qt::Alignment(textAlignment(column));

          if(icon(column).isNull())
            vopt.decorationSize = QSize();
          else
          {
            vopt.features |= QStyleOptionViewItem::HasDecoration;
            vopt.decorationSize = rtw->iconSize();
            vopt.icon = icon(column);
          }
          
          if(rtw->wordWrap())
            vopt.features |= QStyleOptionViewItem::WrapText;
          vopt.features |= QStyleOptionViewItem::HasDisplay;
          
          vopt.font = font(column);
          vopt.fontMetrics = rtw->fontMetrics();
          
          vopt.state = QStyle::State_Active;
          if(!isDisabled())
            vopt.state |= QStyle::State_Enabled;
          if(flags() & Qt::ItemIsUserCheckable)
          {
            vopt.features |= QStyleOptionViewItem::HasCheckIndicator;
            vopt.checkState = checkState(column);
            if(checkState(column) == Qt::Unchecked)
              vopt.state |= QStyle::State_Off;
            else if(checkState(column) == Qt::Checked)
              vopt.state |= QStyle::State_On;
          }
          
          if(isSelected())
            vopt.state |= QStyle::State_Selected;
          
          QSize ct_sz = st->sizeFromContents(QStyle::CT_ItemViewItem, &vopt, QSize(rtw->wordWrap() ? width : _VERY_LARGE_INTEGER_, -1));
          const QRect text_rect = st->subElementRect(QStyle::SE_ItemViewItemText, &vopt);
          QRect r = st->itemTextRect(//treeWidget()->fontMetrics(),
                                     vopt.fontMetrics,
                                     text_rect, 
                                     //textAlignment(column) | Qt::TextWordWrap | Qt::TextWrapAnywhere,
                                     vopt.displayAlignment | Qt::TextWordWrap | Qt::TextWrapAnywhere,
                                     //!isDisabled(), 
                                     vopt.state & QStyle::State_Enabled, 
                                     //text(column));
                                     vopt.text);
          if(r.height() > ct_sz.height())
            ct_sz.setHeight(r.height());

          return ct_sz;
        }
      }
      break;
    }
  }
  
  return QSize();
}
 
bool RouteTreeWidgetItem::testForRelayout(int column, int old_width, int new_width)
{
  switch(type())
  {
    case NormalItem:
    break;
    
    case CategoryItem:
    case RouteItem:
    {
      if(column == RouteDialog::ROUTE_NAME_COL)
      {
//         if(const QStyle* st = treeWidget()->style())
//         {
//           st = st->proxy();
//           // Works fine with TextWrapAnywhere. The -1 represents 'infinite' vertical space - 
//           //  itemTextRect doesn't seem to care in this case with wrap anywhere.
//           QRect old_r = st->itemTextRect(treeWidget()->fontMetrics(), 
//                                         QRect(0, 0, old_width, -1),
//                                         textAlignment(RouteDialog::ROUTE_NAME_COL) | Qt::TextWordWrap | Qt::TextWrapAnywhere,
//                                         !isDisabled(), text(RouteDialog::ROUTE_NAME_COL));
//           QRect new_r = st->itemTextRect(treeWidget()->fontMetrics(), 
//                                         QRect(0, 0, new_width, -1),
//                                         textAlignment(RouteDialog::ROUTE_NAME_COL) | Qt::TextWordWrap | Qt::TextWrapAnywhere,
//                                         !isDisabled(), text(RouteDialog::ROUTE_NAME_COL));
//           return new_r.height() != old_r.height();
//         }
//         return new_sz.height() != old_sz.height();
        
        //if(MusEGlobal::config.routerExpandVertically)
        if(!treeWidget()->wordWrap())
          return false;
        
        return getSizeHint(column, new_width).height() != getSizeHint(column, old_width).height();
      }
    }
    break;
    
    case ChannelsItem:
    {
      if(column == RouteDialog::ROUTE_NAME_COL)
      {
//         // If the width hints are different we must (at least) update the channels' button rectangles.
//         if(_channels.widthHint(new_width) != _channels.widthHint(old_width))
//           computeChannelYValues(new_width);
//         // If the height hints are different we must trigger a relayout.
//         return _channels.heightHint(new_width) != _channels.heightHint(old_width);
        
        RouteTreeWidget* rtw = qobject_cast<RouteTreeWidget*>(treeWidget());
        if(!rtw)
          return false;
    
        if(!rtw->channelWrap())
          return false;
        
        const QSize old_sz = getSizeHint(column, old_width);
        const QSize new_sz = getSizeHint(column, new_width);
        // If the width hints are different we must (at least) update the channels' button rectangles.
        if(new_sz.width() != old_sz.width())
          computeChannelYValues(new_width);
        // If the height hints are different we must trigger a relayout.
        return new_sz.height() != old_sz.height();
      }
    }
    break;
  }
  return false;
}  
  
bool RouteTreeWidgetItem::routeNodeExists()
{
  switch(type())
  {
    case CategoryItem:
    case NormalItem:
      return true;
    break;
    
    case RouteItem:
    case ChannelsItem:
      return _route.exists();
    break;
  }
  return false;
}


//-----------------------------------
//   ConnectionsView
//-----------------------------------

ConnectionsView::ConnectionsView(QWidget* parent, RouteDialog* d)
        : QFrame(parent), _routeDialog(d)
{
  lastY = 0;
  setMinimumWidth(20);
  //setMaximumWidth(120);
  setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
}

ConnectionsView::~ConnectionsView()
{
}

int ConnectionsView::itemY(RouteTreeWidgetItem* item, bool /*is_input*/, int channel) const
{
  QRect rect;
  QTreeWidget* tree = item->treeWidget();

  QTreeWidgetItem* top_closed = 0;
  QTreeWidgetItem* parent = item;
  while(parent)
  {
    parent = parent->parent();
    if(!parent)
      break;
    if(!parent->isExpanded())
      top_closed = parent;
  }
  
  const int line_width = _routeDialog->newSrcList->lineWidth();
  
  //if(parent && !parent->isExpanded()) 
  if(top_closed) 
  {
    rect = tree->visualItemRect(top_closed);
    return line_width + rect.top() + rect.height() / 2;
  } 
//   else 
//   {
    rect = tree->visualItemRect(item);
    if(channel != -1)
      return line_width + rect.top() + item->channelYValue(channel);
    return line_width + rect.top() + rect.height() / 2;
//   }
}


void ConnectionsView::drawConnectionLine(QPainter* pPainter,
        int x1, int y1, int x2, int y2, int h1, int h2 )
{
  //DEBUG_PRST_ROUTES(stderr, "ConnectionsView::drawConnectionLine: x1:%d y1:%d x2:%d y2:%d h1:%d h2:%d\n", x1, y1, x2, y2, h1, h2);
  
  // Account for list view headers.
  y1 += h1;
  y2 += h2;

  // Invisible output ports don't get a connecting dot.
  if(y1 > h1)
    pPainter->drawLine(x1, y1, x1 + 4, y1);

//   if(1) 
  {
    // Setup control points
    QPolygon spline(4);
    const int cp = int(float(x2 - x1 - 8) * 0.4f);
    spline.putPoints(0, 4,
            x1 + 4, y1, x1 + 4 + cp, y1, 
            x2 - 4 - cp, y2, x2 - 4, y2);
    // The connection line, it self.
    QPainterPath path;
    path.moveTo(spline.at(0));
    path.cubicTo(spline.at(1), spline.at(2), spline.at(3));
    pPainter->strokePath(path, pPainter->pen());
  }
//   else 
//     pPainter->drawLine(x1 + 4, y1, x2 - 4, y2);

  // Invisible input ports don't get a connecting dot.
  if(y2 > h2)
    pPainter->drawLine(x2 - 4, y2, x2, y2);
}

void ConnectionsView::drawItem(QPainter* painter, QTreeWidgetItem* routesItem, const QColor& col)
{
  const int yc = QWidget::pos().y();
  const int yo = _routeDialog->newSrcList->pos().y();
  const int yi = _routeDialog->newDstList->pos().y();
  const int x1 = 0;
  const int x2 = QWidget::width();
  const int h1 = (_routeDialog->newSrcList->header())->sizeHint().height();
  const int h2 = (_routeDialog->newDstList->header())->sizeHint().height();
  int y1;
  int y2;
  QPen pen;
  const int pen_wid_norm = 0;
  const int pen_wid_wide = 3;
  
  if(routesItem->data(RouteDialog::ROUTE_SRC_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() && 
     routesItem->data(RouteDialog::ROUTE_DST_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
  {        
    const MusECore::Route src = routesItem->data(RouteDialog::ROUTE_SRC_COL, RouteDialog::RouteRole).value<MusECore::Route>();
    const MusECore::Route dst = routesItem->data(RouteDialog::ROUTE_DST_COL, RouteDialog::RouteRole).value<MusECore::Route>();
    RouteTreeWidgetItem* srcItem = _routeDialog->newSrcList->findItem(src);
    if(srcItem)
    {
      RouteTreeWidgetItem* dstItem = _routeDialog->newDstList->findItem(dst);
      if(dstItem)
      {
        int src_chan = src.channel;
        int dst_chan = dst.channel;
        bool src_wid = false;
        bool dst_wid = false;
        switch(src.type)
        {
          case MusECore::Route::TRACK_ROUTE:
            // Don't draw if channel is unavailable.
            if(src_chan >= srcItem->channelCount())
              return;
            if(src_chan == -1 && src.channels == -1) 
              src_wid = true;
          break;
          
          case MusECore::Route::MIDI_DEVICE_ROUTE:
          case MusECore::Route::MIDI_PORT_ROUTE:
            if(src_chan == -1 && src.channels == -1) 
              src_wid = true;
            // Support port/device items (no channel bar) to track channel item routes:
            // Set source channel to -1 so it draws to the vertical middle of the item.
            src_chan = -1;
          break;
          
          case MusECore::Route::JACK_ROUTE:
          break;
        }
        switch(dst.type)
        {
          case MusECore::Route::TRACK_ROUTE:
            // Don't draw if channel is unavailable.
            if(dst_chan >= dstItem->channelCount())
              return;
            if(dst_chan == -1 && dst.channels == -1) 
              dst_wid = true;
          break;
          
          case MusECore::Route::MIDI_DEVICE_ROUTE:
          case MusECore::Route::MIDI_PORT_ROUTE:
            if(dst_chan == -1 && dst.channels == -1) 
              dst_wid = true;
            // Support track channel items to port/device items (no channel bar) routes:
            // Set dest channel to -1 so it draws to the vertical middle of the item.
            dst_chan = -1;
          break;
          
          case MusECore::Route::JACK_ROUTE:
          break;
        }

        if(src_wid && dst_wid) 
          pen.setWidth(pen_wid_wide);
        else
          pen.setWidth(pen_wid_norm);
        
        pen.setColor(col);
        painter->setPen(pen);
        y1 = itemY(srcItem, true, src_chan) + (yo - yc);
        y2 = itemY(dstItem, false, dst_chan) + (yi - yc);
        drawConnectionLine(painter, x1, y1, x2, y2, h1, h2);
      }
//       else
//       {
//         fprintf(stderr, "ConnectionsView::drawItem: dstItem not found:\n");
//         src.dump();
//         dst.dump();
//       }
    }
//     else
//     {
//       fprintf(stderr, "ConnectionsView::drawItem: srcItem not found:\n");
//       src.dump();
//       dst.dump();
//     }
  }
}

// Draw visible port connection relation arrows.
void ConnectionsView::paintEvent(QPaintEvent*)
{
  //DEBUG_PRST_ROUTES(stderr, "ConnectionsView::paintEvent: _routeDialog:%p\n", _routeDialog);
  if(!_routeDialog)
    return;

  QPainter painter(this);
//   int i, rgb[3] = { 0x33, 0x66, 0x99 };
  int i, rgb[3] = { 0x33, 0x58, 0x7f };
  //int i, rgb[3] = { 0x00, 0x2c, 0x7f };

  // Inline adaptive to darker background themes...
  if(QWidget::palette().window().color().value() < 0x7f)
    for (i = 0; i < 3; ++i) 
      //rgb[i] += 0x33;
      //rgb[i] += 0x66;
      rgb[i] += 0x80;

  i = 0;
  
  const int iItemCount = _routeDialog->routeList->topLevelItemCount();
  // Draw unselected items behind selected items.
  for(int iItem = 0; iItem < iItemCount; ++iItem, ++i) 
  {
    QTreeWidgetItem* item = _routeDialog->routeList->topLevelItem(iItem);
    //++i;
    //if(!item)
    if(!item || item->isHidden() || item->isSelected())
      continue;
    drawItem(&painter, item, QColor(rgb[i % 3], rgb[(i / 3) % 3], rgb[(i / 9) % 3], 128));
  } 
  // Draw selected items on top of unselected items.
  for(int iItem = 0; iItem < iItemCount; ++iItem) 
  {
    QTreeWidgetItem* item = _routeDialog->routeList->topLevelItem(iItem);
    //if(!item)
    if(!item || item->isHidden() || !item->isSelected())
      continue;
    drawItem(&painter, item, Qt::yellow);
  } 
}

void ConnectionsView::mousePressEvent(QMouseEvent* e)
{
  e->setAccepted(true);
  lastY = e->y();
}

void ConnectionsView::mouseMoveEvent(QMouseEvent* e)
{
  e->setAccepted(true);
  const Qt::MouseButtons mb = e->buttons();
  const int y = e->y();
  const int ly = lastY;
  lastY = y;
  if(mb & Qt::LeftButton)
     emit scrollBy(0, ly - y);
}

void ConnectionsView::wheelEvent(QWheelEvent* e)
{
  e->accept();
  
  const QPoint pixelDelta = e->pixelDelta();
  const QPoint angleDegrees = e->angleDelta() / 8;
  int delta = 0;
  if(!pixelDelta.isNull())
    delta = pixelDelta.y();
  else if(!angleDegrees.isNull())
    delta = angleDegrees.y() / 15;
  else
    return;

  DEBUG_PRST_ROUTES(stderr, "ConnectionsView::wheelEvent: delta:%d\n", delta); 
  emit scrollBy(0, delta < 0 ? 1 : -1);
}

// Context menu request event handler.
void ConnectionsView::contextMenuEvent(QContextMenuEvent* /*pContextMenuEvent*/)
{
}



//-----------------------------------
//   RouteTreeWidget
//-----------------------------------

RouteTreeWidget::RouteTreeWidget(QWidget* parent, bool is_input)
  : QTreeWidget(parent), _isInput(is_input), _channelWrap(false)
{
  if(is_input)
    setObjectName(QStringLiteral("Router_input_tree"));
  else
    setObjectName(QStringLiteral("Router_output_tree"));
  
  if(header())
    connect(header(), SIGNAL(sectionResized(int,int,int)), SLOT(headerSectionResized(int,int,int))); 
}

RouteTreeWidget::~RouteTreeWidget()
{
}

void RouteTreeWidget::computeChannelYValues()
{
  const int ch_w = channelWrap() ? columnWidth(RouteDialog::ROUTE_NAME_COL) : -1;
  QTreeWidgetItemIterator itw(this);
  while(*itw)
  {
    RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(*itw);
//     item->computeChannelYValues(wordWrap() ? viewport()->width() : -1);
    //item->computeChannelYValues(wordWrap() ? columnWidth(RouteDialog::ROUTE_NAME_COL) : -1);
    //item->computeChannelYValues(MusEGlobal::config.routerExpandVertically ? columnWidth(RouteDialog::ROUTE_NAME_COL) : _VERY_LARGE_INTEGER_);
    //item->computeChannelYValues(wordWrap() ? columnWidth(RouteDialog::ROUTE_NAME_COL) : _VERY_LARGE_INTEGER_);
    item->computeChannelYValues(ch_w);
    ++itw;
  }
}

void RouteTreeWidget::headerSectionResized(int logicalIndex, int oldSize, int newSize)
{
  DEBUG_PRST_ROUTES(stderr, "RouteTreeWidget::headerSectionResized idx:%d old sz:%d new sz:%d\n", logicalIndex, oldSize, newSize);
  //scheduleDelayedItemsLayout();

  //if(!wordWrap())
  //  return;
   
  // Self adjust certain item heights...
  // NOTE: Delegate sizeHints are NOT called automatically. scheduleDelayedItemsLayout() seems to solve it. 
  //       But that is costly here! And results in some flickering especially at scrollbar on/off conditions as it fights with itself. 
  //       So check if we really need to do it...
  QTreeWidgetItemIterator ii(this);
  //bool do_layout = false;
  int relayouts = 0;
  while(*ii)
  {
    RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(*ii);
    if(item->testForRelayout(logicalIndex, oldSize, newSize))
    {
      const QModelIndex mdl_idx = indexFromItem(item);
      if(mdl_idx.isValid())
      {
        QAbstractItemDelegate* id = itemDelegate();
        if(RoutingItemDelegate* rid = qobject_cast<RoutingItemDelegate*>(id))
        {
          rid->emitSizeHintChanged(mdl_idx);
          ++relayouts;
        }
      }
    }
    
    ++ii;
  }
  
  if(relayouts)
  {
    //connectionsWidget->update();  // Redraw the connections. FIXME: TODO: Need to access the dialog
  }
  
}

RouteTreeWidgetItem* RouteTreeWidget::itemFromIndex(const QModelIndex& index) const
{
  return static_cast<RouteTreeWidgetItem*>(QTreeWidget::itemFromIndex(index));
}

RouteTreeWidgetItem* RouteTreeWidget::findItem(const MusECore::Route& r, int type)
{
  QTreeWidgetItemIterator ii(this);
  while(*ii)
  {
    QTreeWidgetItem* item = *ii;
    switch(item->type())
    {
      case RouteTreeWidgetItem::NormalItem:
      case RouteTreeWidgetItem::CategoryItem:
      break;
      
      case RouteTreeWidgetItem::RouteItem:
      case RouteTreeWidgetItem::ChannelsItem:
      {
        RouteTreeWidgetItem* rtwi = static_cast<RouteTreeWidgetItem*>(item);
        if((type == -1 || type == item->type()) && rtwi->route().compare(r))
          return rtwi;
      }
      break;
    }
    ++ii;
  }
  return NULL;
}

RouteTreeWidgetItem* RouteTreeWidget::findCategoryItem(const QString& name)
{
  const int cnt = topLevelItemCount(); 
  for(int i = 0; i < cnt; ++i)
  {
    RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(topLevelItem(i));
    if(item && item->type() == RouteTreeWidgetItem::CategoryItem && item->text(RouteDialog::ROUTE_NAME_COL) == name)
      return item;
  }
  return 0;
}
      
void RouteTreeWidget::getSelectedRoutes(MusECore::RouteList& routes)
{
  RouteTreeItemList sel = selectedItems();
  const int selSz = sel.size();
  if(selSz == 0)
    return;
  for(int idx = 0; idx < selSz; ++idx)
  {
    RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(sel.at(idx));
    if(!item)
      continue;
    item->getSelectedRoutes(routes);
  }
}

int RouteTreeWidget::channelAt(RouteTreeWidgetItem* item, const QPoint& pt)
{
  const QRect rect = visualItemRect(item);
  
  return item->channelAt(pt, rect);
}

void RouteTreeWidget::resizeEvent(QResizeEvent* event)
{
  DEBUG_PRST_ROUTES(stderr, "RouteTreeWidget::resizeEvent old w:%d h:%d new w:%d h:%d\n", event->oldSize().width(), event->oldSize().height(),
          event->size().width(), event->size().height());

  event->ignore();
  QTreeWidget::resizeEvent(event);
  //if(wordWrap())
  //if(MusEGlobal::config.routerExpandVertically)
//     headerSectionResized(RouteDialog::ROUTE_NAME_COL, event->oldSize().width(), event->size().width()); // ZZZ
}

void RouteTreeWidget::mousePressEvent(QMouseEvent* e)
{
  const QPoint pt = e->pos(); 
  //Qt::KeyboardModifiers km = e->modifiers();
  //bool ctl = km & Qt::ControlModifier;
  //bool shift = km & Qt::ShiftModifier;
  RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(itemAt(pt));
  bool is_cur = item && currentItem() && (item == currentItem());

  //if(is_cur)
  //  QTreeWidget::mousePressEvent(e);
  
  if(item)
  {
    bool changed = item->mousePressHandler(e, visualItemRect(item));
    if(changed)
    {
      //setCurrentItem(item);
      //update(visualItemRect(item));
      QRect r(visualItemRect(item));
      // Need to update from the item's right edge to the viewport right edge,
      //  for the connector lines.
      r.setRight(this->viewport()->geometry().right());
      setDirtyRegion(r);
      //emit itemSelectionChanged();
    }
    
    //if(!is_cur)
      QTreeWidget::mousePressEvent(e);

    if(changed && is_cur)
      //setCurrentItem(item);
      emit itemSelectionChanged();
      
    //e->accept();
    return;
    
  }
  QTreeWidget::mousePressEvent(e);
}    

void RouteTreeWidget::mouseMoveEvent(QMouseEvent* e)
{
  const QPoint pt = e->pos(); 
  //Qt::KeyboardModifiers km = e->modifiers();
  //bool ctl = km & Qt::ControlModifier;
  //bool shift = km & Qt::ShiftModifier;
  RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(itemAt(pt));
  bool is_cur = item && currentItem() && (item == currentItem());

  //if(is_cur)
  //  QTreeWidget::mouseMoveEvent(e);
  
  if(item)
  {
    bool changed = item->mouseMoveHandler(e, visualItemRect(item));
    if(changed)
    {
      //setCurrentItem(item);
      //update(visualItemRect(item));
      setDirtyRegion(visualItemRect(item));
      //emit itemSelectionChanged();
    }
    
    //if(!is_cur)
      QTreeWidget::mouseMoveEvent(e);

    if(changed && is_cur)
      //setCurrentItem(item);
      emit itemSelectionChanged();
      
    //e->accept();
    return;
    
  }
  QTreeWidget::mouseMoveEvent(e);
}    
    
QItemSelectionModel::SelectionFlags RouteTreeWidget::selectionCommand(const QModelIndex& index, const QEvent* e) const
{
  QItemSelectionModel::SelectionFlags flags = QTreeWidget::selectionCommand(index, e);
  DEBUG_PRST_ROUTES(stderr, "RouteTreeWidget::selectionCommand flags:%d row:%d col:%d ev type:%d\n", int(flags), index.row(), index.column(), e ? e->type() : -1);

  RouteTreeWidgetItem* item = itemFromIndex(index);

  if(item && item->type() == RouteTreeWidgetItem::ChannelsItem)
  {
    if(flags & QItemSelectionModel::Toggle)
    {
      flags &= ~QItemSelectionModel::Toggle;
      flags |= QItemSelectionModel::Select;
      DEBUG_PRST_ROUTES(stderr, "RouteTreeWidget::selectionCommand new flags:%d\n", int(flags));
    }
  }
  
  return flags;
}

void RouteTreeWidget::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  QModelIndexList mil = deselected.indexes();
  const int dsz = mil.size();
  DEBUG_PRST_ROUTES(stderr, "RouteTreeWidget::selectionChanged: selected size:%d deselected size:%d\n", selected.size(), dsz);
  for(int i = 0; i < dsz; ++i)
  {
    const QModelIndex& index = mil.at(i);
    RouteTreeWidgetItem* item = itemFromIndex(index);
    
    if(item && item->type() == RouteTreeWidgetItem::ChannelsItem)
      item->fillSelectedChannels(false);
  }    
  QTreeWidget::selectionChanged(selected, deselected);
}    

void RouteTreeWidget::scrollBy(int dx, int dy)
{
  DEBUG_PRST_ROUTES(stderr, "RouteTreeWidget::scrollBy: dx:%d dy:%d\n", dx, dy); 
  int hv = horizontalScrollBar()->value();
  int vv = verticalScrollBar()->value();
  if(dx)
  {
    hv += dx;
    horizontalScrollBar()->setValue(hv);
  }
  if(dy)
  {
    vv += dy;
    verticalScrollBar()->setValue(vv);
  }
}

void RouteTreeWidget::getItemsToDelete(QVector<QTreeWidgetItem*>& items_to_remove, bool showAllMidiPorts)
{
  QTreeWidgetItemIterator ii(this);
  while(*ii)
  {
    QTreeWidgetItem* item = *ii;
    if(item)
    {
      QTreeWidgetItem* twi = item;
      while((twi = twi->parent()))
      {
        if(items_to_remove.contains(twi))
          break;
      }
      // No parent found to be deleted. Determine if this should be deleted.
      if(!twi)
      {
        if(!items_to_remove.contains(item))
        {
          RouteTreeWidgetItem* rtwi = static_cast<RouteTreeWidgetItem*>(item);

          switch(rtwi->type())
          {
            case RouteTreeWidgetItem::NormalItem:
            case RouteTreeWidgetItem::CategoryItem:
            case RouteTreeWidgetItem::ChannelsItem:
            break;

            case RouteTreeWidgetItem::RouteItem:
            {
              const MusECore::Route& rt = rtwi->route();
              switch(rt.type)
              {
                case MusECore::Route::MIDI_DEVICE_ROUTE:
                case MusECore::Route::TRACK_ROUTE:
                case MusECore::Route::JACK_ROUTE:
                break;
                
                case MusECore::Route::MIDI_PORT_ROUTE:
                {
                  bool remove_port = false;
                  if(!rt.isValid())
                    remove_port = true;
                  else 
                  if(!showAllMidiPorts)
                  {
                    MusECore::MidiPort* mp = &MusEGlobal::midiPorts[rt.midiPort];
                    if(!mp->device() && (_isInput ? mp->outRoutes()->empty() : mp->inRoutes()->empty()))
                    {
                      
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
                      if(!_isInput)
                      {
                        MusECore::MidiTrackList* tl = MusEGlobal::song->midis();
                        MusECore::ciMidiTrack imt = tl->begin();
                        for( ; imt != tl->end(); ++imt)
                          if((*imt)->outPort() == rt.midiPort)
                            break;
                        if(imt == tl->end())
                          remove_port = true;
                      }
                      else
#endif  // _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
                        remove_port = true;

                    }
                  }

                  if(remove_port)
                    items_to_remove.append(item);
                  ++ii;
                  continue;
                }
                break;
              }
            }
            break;
          }

          if(!rtwi->routeNodeExists())
            items_to_remove.append(item);
        }
      }
    }
    ++ii;
  }
}

void RouteTreeWidget::selectRoutes(const QList<QTreeWidgetItem*>& routes, bool doNormalSelections)
{
  QTreeWidgetItemIterator ii(this);
  while(*ii)
  {
    RouteTreeWidgetItem* rtwi = static_cast<RouteTreeWidgetItem*>(*ii);
    switch(rtwi->type())
    {
      case RouteTreeWidgetItem::NormalItem:
      case RouteTreeWidgetItem::CategoryItem:
      case RouteTreeWidgetItem::RouteItem:
      break;
      
      case RouteTreeWidgetItem::ChannelsItem:
      {
        bool do_upd = rtwi->fillChannelsRouteSelected(false);
        if(doNormalSelections && rtwi->fillSelectedChannels(false))
          do_upd = true;
        const MusECore::Route& rtwi_route = rtwi->route();
        const int sz = routes.size();
        for(int i = 0; i < sz; ++i)
        {
          const QTreeWidgetItem* routes_item = routes.at(i);
          const MusECore::Route r = 
            routes_item->data(isInput() ? RouteDialog::ROUTE_SRC_COL : RouteDialog::ROUTE_DST_COL, RouteDialog::RouteRole).value<MusECore::Route>();
          if(rtwi_route.compare(r))
          {
            const int chan = r.channel;
            if(chan >= 0)
            {
              //if(!rtwi->channelRouteSelected(chan))
              //{
                rtwi->routeSelectChannel(chan, true);
                do_upd = true;
              //}
              //if(doNormalSelections && !rtwi->channelSelected(chan))
              if(doNormalSelections)
              {
                rtwi->selectChannel(chan, true);
                do_upd = true;
              }
            }
          }
        }
        if(do_upd)
        {
          QRect r(visualItemRect(rtwi));
          // Need to update from the item's right edge to the viewport right edge,
          //  for the connector lines.
          r.setRight(this->viewport()->geometry().right());
          setDirtyRegion(r);
        }
      }
      break;
    }
    ++ii;
  }
}  

//-----------------------------------
//   RoutingItemDelegate
//-----------------------------------

RoutingItemDelegate::RoutingItemDelegate(bool is_input, RouteTreeWidget* tree, QWidget *parent) 
                    : QStyledItemDelegate(parent), _tree(tree), _isInput(is_input)
{
  _firstPress = true;
}

void RoutingItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const
{
//   DEBUG_PRST_ROUTES(stderr, "RoutingItemDelegate::paint row:%d col:%d, rect x:%d y:%d w:%d h:%d showDecorationSelected:%d\n",
//           index.row(), index.column(),
//           option.rect.x(), option.rect.y(), option.rect.width(), option.rect.height(),
//           option.showDecorationSelected);

  RouteTreeWidgetItem* item = _tree->itemFromIndex(index);
  if(item)
  {
    // Required. option is not automatically filled from index.
    QStyleOptionViewItem vopt(option);
    initStyleOption(&vopt, index);
  
    if(item->paint(painter, vopt, index))
      return;
  }
  QStyledItemDelegate::paint(painter, option, index);
}  

void RoutingItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                 const QModelIndex &index) const
{
  DEBUG_PRST_ROUTES(stderr, "RoutingItemDelegate::setModelData\n");

  switch(index.column())
  {

    default:
       QStyledItemDelegate::setModelData(editor, model, index);
  }
}

QSize RoutingItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  DEBUG_PRST_ROUTES(stderr, "RoutingItemDelegate::sizeHint\n");
  
  if(RouteTreeWidgetItem* item = _tree->itemFromIndex(index))
  {
    const QSize sz = item->getSizeHint(index.column(), _tree->columnWidth(RouteDialog::ROUTE_NAME_COL)); 
    if(sz.isValid())
    {
      //fprintf(stderr, "RoutingItemDelegate::sizeHint w:%d h:%d\n", sz.width(), sz.height());
      return sz;
    }
  }
  return QStyledItemDelegate::sizeHint(option, index);
}  

bool RoutingItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  return QStyledItemDelegate::editorEvent(event, model, option, index);
}


bool RoutingItemDelegate::eventFilter(QObject* editor, QEvent* event)
{
  return QStyledItemDelegate::eventFilter(editor, event);
}





//---------------------------------------------------------
//   RouteDialog
//---------------------------------------------------------

RouteDialog::RouteDialog(QWidget* parent)
   : QDialog(parent)
{
  setupUi(this);

//   newSrcList->setWordWrap(false);
//   newDstList->setWordWrap(false);
//   routeList->setWordWrap(false);
//   newSrcList->setTextElideMode(Qt::ElideNone);
//   newDstList->setTextElideMode(Qt::ElideNone);
//   routeList->setTextElideMode(Qt::ElideNone);
  
  filterSrcButton->setIcon(*routeSourceSVGIcon);
  filterDstButton->setIcon(*routeDestSVGIcon);
  srcRoutesButton->setIcon(*routeSelSourceSVGIcon);
  dstRoutesButton->setIcon(*routeSelDestSVGIcon);
  allMidiPortsButton->setIcon(*ankerSVGIcon);
  verticalLayoutButton->setIcon(*routeAutoAdjustSVGIcon);
  
  routeAliasList->addItem(tr("Normal"), QVariant::fromValue<int>(MusEGlobal::RoutePreferCanonicalName));
  routeAliasList->addItem(tr("Alias 1"), QVariant::fromValue<int>(MusEGlobal::RoutePreferFirstAlias));
  routeAliasList->addItem(tr("Alias 2"), QVariant::fromValue<int>(MusEGlobal::RoutePreferSecondAlias));

//   verticalLayoutButton->setChecked(newSrcList->wordWrap() || newDstList->wordWrap());
  
  //newSrcList->viewport()->setLayoutDirection(Qt::LeftToRight);
  
  //_srcFilterItem = NULL;
  //_dstFilterItem = NULL;

  srcItemDelegate = new RoutingItemDelegate(true, newSrcList, this);
  dstItemDelegate = new RoutingItemDelegate(false, newDstList, this);
  
  newSrcList->setItemDelegate(srcItemDelegate);
  newDstList->setItemDelegate(dstItemDelegate);
  
  //newSrcList->setItemsExpandable(false);  // REMOVE Tim. For test only.
  //newDstList->setItemsExpandable(false);  // REMOVE Tim. For test only.
  
  connectionsWidget->setRouteDialog(this);

  QStringList columnnames;
  columnnames << tr("Source");
  newSrcList->setColumnCount(columnnames.size());
  newSrcList->setHeaderLabels(columnnames);
  for (int i = 0; i < columnnames.size(); ++i) {
        //setWhatsThis(newSrcList->horizontalHeaderItem(i), i);
        //setToolTip(newSrcList->horizontalHeaderItem(i), i);
        }
        
  columnnames.clear();
  columnnames << tr("Destination");
  newDstList->setColumnCount(columnnames.size());
  newDstList->setHeaderLabels(columnnames);
  for (int i = 0; i < columnnames.size(); ++i) {
        //setWhatsThis(newDstList->horizontalHeaderItem(i), i);
        //setToolTip(newDstList->horizontalHeaderItem(i), i);
        }

  newSrcList->setTreePosition(1);
  newDstList->setTreePosition(1);
  
  // Need this. Don't remove.
//   newSrcList->header()->setSectionResizeMode(QHeaderView::Stretch);
//   newDstList->header()->setSectionResizeMode(QHeaderView::Stretch);
//   newSrcList->header()->setSectionResizeMode(QHeaderView::Interactive);
//   newDstList->header()->setSectionResizeMode(QHeaderView::Interactive);

  //newSrcList->header()->setStretchLastSection(false);
  //newDstList->header()->setStretchLastSection(false);
  
  newSrcList->setTextElideMode(Qt::ElideMiddle);
  newDstList->setTextElideMode(Qt::ElideMiddle);

  
  columnnames.clear();
  columnnames << tr("Source")
              << tr("Destination");
  routeList->setColumnCount(columnnames.size());
  routeList->setHeaderLabels(columnnames);
  for (int i = 0; i < columnnames.size(); ++i) {
        //setWhatsThis(routeList->horizontalHeaderItem(i), i);
        //setToolTip(routeList->horizontalHeaderItem(i), i);
        }
  
  // Make it so that the column(s) cannot be shrunk below the size of one group of channels in a ChannelsItem.
  newSrcList->header()->setMinimumSectionSize(RouteChannelsList::minimumWidthHint());
  newDstList->header()->setMinimumSectionSize(RouteChannelsList::minimumWidthHint());
  
  verticalLayoutButton->setChecked(MusEGlobal::config.routerExpandVertically);
  if(MusEGlobal::config.routerExpandVertically)
  {
//     newSrcList->resizeColumnToContents(ROUTE_NAME_COL);
//     newDstList->resizeColumnToContents(ROUTE_NAME_COL);
    newSrcList->setWordWrap(true);
    newDstList->setWordWrap(true);
    newSrcList->setChannelWrap(true);
    newDstList->setChannelWrap(true);
    newSrcList->header()->setSectionResizeMode(QHeaderView::Stretch);
    newDstList->header()->setSectionResizeMode(QHeaderView::Stretch);
    newSrcList->setColumnWidth(ROUTE_NAME_COL, RouteChannelsList::minimumWidthHint());
    newDstList->setColumnWidth(ROUTE_NAME_COL, RouteChannelsList::minimumWidthHint());
  }
  else
  {
    newSrcList->setWordWrap(false);
    newDstList->setWordWrap(false);
    newSrcList->setChannelWrap(true);
    newDstList->setChannelWrap(true);
    newSrcList->header()->setSectionResizeMode(QHeaderView::Interactive);
    newDstList->header()->setSectionResizeMode(QHeaderView::Interactive);
  }
  
  songChanged(SC_EVERYTHING);

  connect(newSrcList->verticalScrollBar(), SIGNAL(rangeChanged(int,int)), srcTreeScrollBar, SLOT(setRange(int,int))); 
  connect(newDstList->verticalScrollBar(), SIGNAL(rangeChanged(int,int)), dstTreeScrollBar, SLOT(setRange(int,int))); 
  connect(newSrcList->verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(srcTreeScrollValueChanged(int))); 
  connect(newDstList->verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(dstTreeScrollValueChanged(int))); 
  connect(srcTreeScrollBar, SIGNAL(valueChanged(int)), SLOT(srcScrollBarValueChanged(int))); 
  connect(dstTreeScrollBar, SIGNAL(valueChanged(int)), SLOT(dstScrollBarValueChanged(int))); 
  
  connect(routeList, SIGNAL(itemSelectionChanged()), SLOT(routeSelectionChanged()));
  connect(newSrcList, SIGNAL(itemSelectionChanged()), SLOT(srcSelectionChanged()));
  connect(newDstList, SIGNAL(itemSelectionChanged()), SLOT(dstSelectionChanged()));
  //connect(newSrcList->verticalScrollBar(), SIGNAL(sliderMoved(int)), connectionsWidget, SLOT(update()));
  //connect(newDstList->verticalScrollBar(), SIGNAL(sliderMoved(int)), connectionsWidget, SLOT(update()));
  connect(newSrcList->verticalScrollBar(), SIGNAL(valueChanged(int)), connectionsWidget, SLOT(update()));
  connect(newDstList->verticalScrollBar(), SIGNAL(valueChanged(int)), connectionsWidget, SLOT(update()));
  connect(newSrcList, SIGNAL(itemCollapsed(QTreeWidgetItem*)), connectionsWidget, SLOT(update()));
  connect(newSrcList, SIGNAL(itemExpanded(QTreeWidgetItem*)), connectionsWidget, SLOT(update()));
  connect(newDstList, SIGNAL(itemCollapsed(QTreeWidgetItem*)), connectionsWidget, SLOT(update()));
  connect(newDstList, SIGNAL(itemExpanded(QTreeWidgetItem*)), connectionsWidget, SLOT(update()));
  connect(connectionsWidget, SIGNAL(scrollBy(int, int)), newSrcList, SLOT(scrollBy(int, int)));
  connect(connectionsWidget, SIGNAL(scrollBy(int, int)), newDstList, SLOT(scrollBy(int, int)));
  connect(removeButton, SIGNAL(clicked()), SLOT(disconnectClicked()));
  connect(connectButton, SIGNAL(clicked()), SLOT(connectClicked()));
  connect(allMidiPortsButton, SIGNAL(clicked(bool)), SLOT(allMidiPortsClicked(bool)));
  connect(verticalLayoutButton, SIGNAL(clicked(bool)), SLOT(verticalLayoutClicked(bool)));
  connect(filterSrcButton, SIGNAL(clicked(bool)), SLOT(filterSrcClicked(bool)));
  connect(filterDstButton, SIGNAL(clicked(bool)), SLOT(filterDstClicked(bool)));
  connect(srcRoutesButton, SIGNAL(clicked(bool)), SLOT(filterSrcRoutesClicked(bool)));
  connect(dstRoutesButton, SIGNAL(clicked(bool)), SLOT(filterDstRoutesClicked(bool)));
  connect(routeAliasList, SIGNAL(activated(int)), SLOT(preferredRouteAliasChanged(int)));
  connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));
}

void RouteDialog::srcTreeScrollValueChanged(int value)
{
  // Prevent recursion
  srcTreeScrollBar->blockSignals(true);
  srcTreeScrollBar->setValue(value);
  srcTreeScrollBar->blockSignals(false);
}

void RouteDialog::dstTreeScrollValueChanged(int value)
{
  // Prevent recursion
  dstTreeScrollBar->blockSignals(true);
  dstTreeScrollBar->setValue(value);
  dstTreeScrollBar->blockSignals(false);
}

void RouteDialog::srcScrollBarValueChanged(int value)
{
  // Prevent recursion
  newSrcList->blockSignals(true);
  newSrcList->verticalScrollBar()->setValue(value);
  newSrcList->blockSignals(false);
}

void RouteDialog::dstScrollBarValueChanged(int value)
{
  // Prevent recursion
  newDstList->blockSignals(true);
  newDstList->verticalScrollBar()->setValue(value);
  newDstList->blockSignals(false);
}

// void RouteDialog::routeSplitterMoved(int pos, int index)
// {
//   DEBUG_PRST_ROUTES(stderr, "RouteDialog::routeSplitterMoved pos:%d index:%d\n", pos, index);
//   
// }


void RouteDialog::preferredRouteAliasChanged(int /*idx*/)
{
  if(routeAliasList->currentData().canConvert<int>())
  {
    bool ok = false;
    const int n = routeAliasList->currentData().toInt(&ok);
    if(ok)
    {
      switch(n)
      {
        case MusEGlobal::RoutePreferCanonicalName:
        case MusEGlobal::RoutePreferFirstAlias:
        case MusEGlobal::RoutePreferSecondAlias:
          MusEGlobal::config.preferredRouteNameOrAlias = MusEGlobal::RouteNameAliasPreference(n);
          MusEGlobal::song->update(SC_PORT_ALIAS_PREFERENCE);
        break;
        default:
        break;
      }
    }
  }
}

void RouteDialog::verticalLayoutClicked(bool v)
{
  if(v)
  {
//     fprintf(stderr, "RouteDialog::verticalLayoutClicked v:%d calling src resizeColumnToContents\n", v); 
//     newSrcList->resizeColumnToContents(ROUTE_NAME_COL);
//     fprintf(stderr, "RouteDialog::verticalLayoutClicked v:%d calling dst resizeColumnToContents\n", v); 
//     newDstList->resizeColumnToContents(ROUTE_NAME_COL);
    MusEGlobal::config.routerExpandVertically = v;
    newSrcList->setWordWrap(v);
    newDstList->setWordWrap(v);
    newSrcList->setChannelWrap(v);
    newDstList->setChannelWrap(v);
    newSrcList->header()->setSectionResizeMode(QHeaderView::Stretch);
    newDstList->header()->setSectionResizeMode(QHeaderView::Stretch);
    newSrcList->setColumnWidth(ROUTE_NAME_COL, RouteChannelsList::minimumWidthHint());
    newDstList->setColumnWidth(ROUTE_NAME_COL, RouteChannelsList::minimumWidthHint());
  }
  else
  {
    MusEGlobal::config.routerExpandVertically = v;
    newSrcList->setWordWrap(v);
    newDstList->setWordWrap(v);
    newSrcList->setChannelWrap(true);
    newDstList->setChannelWrap(true);
    newSrcList->header()->setSectionResizeMode(QHeaderView::Interactive);
    newDstList->header()->setSectionResizeMode(QHeaderView::Interactive);
  }
  //fprintf(stderr, "RouteDialog::verticalLayoutClicked v:%d calling dst computeChannelYValues\n", v); 
  newDstList->computeChannelYValues();
  newSrcList->computeChannelYValues();
  //fprintf(stderr, "RouteDialog::verticalLayoutClicked v:%d calling src computeChannelYValues\n", v); 
  connectionsWidget->update();  // Redraw the connections.
}

void RouteDialog::allMidiPortsClicked(bool v)
{
  // TODO: This is a bit brutal and sweeping... Refine this down to needed parts only.
//   routingChanged();  
  
  
  // Refill the lists of available external ports.
//   tmpJackOutPorts = MusEGlobal::audioDevice->outputPorts();
//   tmpJackInPorts = MusEGlobal::audioDevice->inputPorts();
//   tmpJackMidiOutPorts = MusEGlobal::audioDevice->outputPorts(true);
//   tmpJackMidiInPorts = MusEGlobal::audioDevice->inputPorts(true);
  if(v)
    addItems();                   // Add any new items.
  else
    removeItems();                // Remove unused items.
    
//   newSrcList->resizeColumnToContents(ROUTE_NAME_COL);
//   newDstList->resizeColumnToContents(ROUTE_NAME_COL);
  routeList->resizeColumnToContents(ROUTE_SRC_COL);
  routeList->resizeColumnToContents(ROUTE_DST_COL);
  
  // Now that column resizing is done, update all channel y values in source and destination lists.
  // Must be done here because it relies on the column width.
  newDstList->computeChannelYValues();
  newSrcList->computeChannelYValues();
  
  routeSelectionChanged();      // Init remove button.
  srcSelectionChanged();        // Init select button.
  connectionsWidget->update();  // Redraw the connections.
}

void RouteDialog::filterSrcClicked(bool v)
{
  if(dstRoutesButton->isChecked())
  {
    dstRoutesButton->blockSignals(true);
    dstRoutesButton->setChecked(false);
    dstRoutesButton->blockSignals(false);
  }
  filter(v ? newSrcList->selectedItems() : RouteTreeItemList(), RouteTreeItemList(), true, false);
}

void RouteDialog::filterDstClicked(bool v)
{
  if(srcRoutesButton->isChecked())
  {
    srcRoutesButton->blockSignals(true);
    srcRoutesButton->setChecked(false);
    srcRoutesButton->blockSignals(false);
  }
 filter(RouteTreeItemList(), v ? newDstList->selectedItems() : RouteTreeItemList(), false, true);
}

void RouteDialog::filterSrcRoutesClicked(bool /*v*/)
{
  if(dstRoutesButton->isChecked())
  {
    dstRoutesButton->blockSignals(true);
    dstRoutesButton->setChecked(false);
    dstRoutesButton->blockSignals(false);
  }
  if(filterDstButton->isChecked())
  {
    filterDstButton->blockSignals(true);
    filterDstButton->setChecked(false);
    filterDstButton->blockSignals(false);
  }
  // Unfilter the entire destination list, while (un)filtering with the 'show only possible routes' part.
  filter(RouteTreeItemList(), RouteTreeItemList(), false, true);
}

void RouteDialog::filterDstRoutesClicked(bool /*v*/)
{
  if(srcRoutesButton->isChecked())
  {
    srcRoutesButton->blockSignals(true);
    srcRoutesButton->setChecked(false);
    srcRoutesButton->blockSignals(false);
  }
  if(filterSrcButton->isChecked())
  {
    filterSrcButton->blockSignals(true);
    filterSrcButton->setChecked(false);
    filterSrcButton->blockSignals(false);
  }
  // Unfilter the entire source list, while (un)filtering with the 'show only possible routes' part.
  filter(RouteTreeItemList(), RouteTreeItemList(), true, false);
}

void RouteDialog::filter(const RouteTreeItemList& srcFilterItems, 
                         const RouteTreeItemList& dstFilterItems,
                         bool filterSrc, 
                         bool filterDst)
{
  bool src_changed = false;
  bool dst_changed = false;
  const RouteTreeItemList src_sel_items = newSrcList->selectedItems();
  const RouteTreeItemList dst_sel_items = newDstList->selectedItems();
  bool hide;

  QTreeWidgetItemIterator iSrcTree(newSrcList);
  while(*iSrcTree)
  {
    QTreeWidgetItem* item = *iSrcTree;
    hide = item->isHidden();

    if(filterSrc)
    {
      hide = false;
      if(!srcFilterItems.isEmpty())
      {
        RouteTreeItemList::const_iterator ciFilterItems = srcFilterItems.cbegin();
        for( ; ciFilterItems != srcFilterItems.cend(); ++ciFilterItems)
        {
          QTreeWidgetItem* flt_item = *ciFilterItems;
          QTreeWidgetItem* twi = flt_item;
          while(twi != item && (twi = twi->parent()))  ;
          
          if(twi == item)
            break;
          QTreeWidgetItem* twi_p = item;
          while(twi_p != flt_item && (twi_p = twi_p->parent()))  ;

          if(twi_p == flt_item)
            break;
        }
        hide = ciFilterItems == srcFilterItems.cend();
      }
    }
    //else
    
    if(!filterSrc || srcFilterItems.isEmpty())
    {
      hide = false;
      //// Is the item slated to hide? Check finally the 'show only possible routes' settings...
      //if(hide && dstRoutesButton->isChecked())
      // Check finally the 'show only possible routes' settings...
      if(dstRoutesButton->isChecked())
      {
        switch(item->type())
        {
          case RouteTreeWidgetItem::NormalItem:
          case RouteTreeWidgetItem::CategoryItem:
            hide = true;
          break;
          
          case RouteTreeWidgetItem::RouteItem:
          case RouteTreeWidgetItem::ChannelsItem:
          {
            RouteTreeWidgetItem* rtwi = static_cast<RouteTreeWidgetItem*>(item);
            RouteTreeItemList::const_iterator iSelItems = dst_sel_items.cbegin();
            for( ; iSelItems != dst_sel_items.cend(); ++iSelItems)
            {
              RouteTreeWidgetItem* sel_dst_item = static_cast<RouteTreeWidgetItem*>(*iSelItems);
              //if(sel_dst_item->type() == item->type() && MusECore::routesCompatible(rtwi->route(), sel_dst_item->route(), false))
              if(MusECore::routesCompatible(rtwi->route(), sel_dst_item->route(), true))
              {
                //hide = false;
                // Hm, Qt doesn't seem to do this for us:
                QTreeWidgetItem* twi = item;
                while((twi = twi->parent()))
                {
                  //if(twi->isHidden() != hide)
                  if(twi->isHidden())
                  {
                    //twi->setHidden(hide);
                    twi->setHidden(false);
                    src_changed = true;
                  }
                }
                break;
              }
            }
            hide = iSelItems == dst_sel_items.cend();
          }
          break;
        }
      }
    }
    
    if(item->isHidden() != hide)
    {
      item->setHidden(hide);
      src_changed = true;
    }
    
    ++iSrcTree;
  }

  
  QTreeWidgetItemIterator iDstTree(newDstList);
  while(*iDstTree)
  {
    QTreeWidgetItem* item = *iDstTree;
    hide = item->isHidden();
    
    if(filterDst)
    {
      hide = false;
      if(!dstFilterItems.isEmpty())
      {
        RouteTreeItemList::const_iterator ciFilterItems = dstFilterItems.cbegin();
        for( ; ciFilterItems != dstFilterItems.cend(); ++ciFilterItems)
        {
          QTreeWidgetItem* flt_item = *ciFilterItems;
          QTreeWidgetItem* twi = flt_item;
          while(twi != item && (twi = twi->parent()))  ;
          
          if(twi == item)
            break;
          QTreeWidgetItem* twi_p = item;
          while(twi_p != flt_item && (twi_p = twi_p->parent()))  ;
          
          if(twi_p == flt_item)
            break;
        }
        hide = ciFilterItems == dstFilterItems.cend();
      }
    }
    //else
    
    if(!filterDst || dstFilterItems.isEmpty())
    {
      hide = false;
      //// Is the item slated to hide? Check finally the 'show only possible routes' settings...
      //if(hide && srcRoutesButton->isChecked())
      // Check finally the 'show only possible routes' settings...
      if(srcRoutesButton->isChecked())
      {
        switch(item->type())
        {
          case RouteTreeWidgetItem::NormalItem:
          case RouteTreeWidgetItem::CategoryItem:
            hide = true;
          break;
          
          case RouteTreeWidgetItem::RouteItem:
          case RouteTreeWidgetItem::ChannelsItem:
          {
            RouteTreeWidgetItem* rtwi = static_cast<RouteTreeWidgetItem*>(item);
            RouteTreeItemList::const_iterator iSelItems = src_sel_items.cbegin();
            for( ; iSelItems != src_sel_items.cend(); ++iSelItems)
            {
              RouteTreeWidgetItem* sel_src_item = static_cast<RouteTreeWidgetItem*>(*iSelItems);

              MusECore::Route& src = sel_src_item->route();
              MusECore::Route& dst = rtwi->route();
              
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
// Special: Allow simulated midi track to midi port route (a route found in our 'local' routelist
//           but not in any track or port routelist) until multiple output routes are allowed
//           instead of just single port and channel properties. The route is exclusive.
              bool is_compatible = false;
              switch(src.type)
              {
                case MusECore::Route::TRACK_ROUTE:
                  switch(dst.type)
                  {
                    case MusECore::Route::MIDI_PORT_ROUTE:
                      if(src.track->isMidiTrack())
                        is_compatible = true;
                    break;
                    
                    case MusECore::Route::TRACK_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
                      break;
                  }
                break;
                
                case MusECore::Route::MIDI_PORT_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
                break;
              }
              
              if(is_compatible ||
#else
              if(
#endif
                 MusECore::routesCompatible(sel_src_item->route(), rtwi->route(), true))
              {
                //hide = false;
                // Hm, Qt doesn't seem to do this for us:
                QTreeWidgetItem* twi = item;
                while((twi = twi->parent()))
                {
                  //if(twi->isHidden() != hide)
                  if(twi->isHidden())
                  {
                    //twi->setHidden(hide);
                    twi->setHidden(false);
                    dst_changed = true;
                  }
                }
                break;
              }
            }
            hide = iSelItems == src_sel_items.cend();
          }
          break;
        }
      }
    }
    
    if(item->isHidden() != hide)
    {
      item->setHidden(hide);
      dst_changed = true;
    }
    
    ++iDstTree;
  }
  
  
  
  // Update the connecting lines.
  if(src_changed)
  {
//     QTreeWidgetItemIterator iSrcList(newSrcList);
//     while(*iSrcList)
//     {
//       RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(*iSrcList);
//       item->computeChannelYValues();
//       ++iSrcList;
//     }
    newSrcList->computeChannelYValues();
  }
  if(dst_changed)
  {
//     QTreeWidgetItemIterator iDstList(newDstList);
//     while(*iDstList)
//     {
//       RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(*iDstList);
//       item->computeChannelYValues();
//       ++iDstList;
//     }
    newDstList->computeChannelYValues();
  }
  
  if(src_changed || dst_changed)  
    connectionsWidget->update();  // Redraw the connections.
  
  QTreeWidgetItemIterator iRouteTree(routeList);
//   //RouteTreeItemList::iterator iFilterItems;
//   //filterItems::iterator iFilterItems;
//   
  while(*iRouteTree)
  {
    QTreeWidgetItem* item = *iRouteTree;
    //if(item->data(isDestination ? RouteDialog::ROUTE_DST_COL : RouteDialog::ROUTE_SRC_COL, 
    //              RouteDialog::RouteRole).canConvert<MusECore::Route>())
    if(item && item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() && item->data(ROUTE_DST_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
    {        
      //const MusECore::Route r = item->data(isDestination ? RouteDialog::ROUTE_DST_COL : RouteDialog::ROUTE_SRC_COL, 
      //                                     RouteDialog::RouteRole).value<MusECore::Route>();
      const MusECore::Route src = item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).value<MusECore::Route>();
      const MusECore::Route dst = item->data(ROUTE_DST_COL, RouteDialog::RouteRole).value<MusECore::Route>();
      
//       bool hide = false;
//       
//       //if(_srcFilterItems.isEmpty())
//       //  item->setHidden(false);
//       //else
//       //{
//         RouteTreeItemList::const_iterator ciFilterItems = _srcFilterItems.begin();
//         for( ; ciFilterItems != _srcFilterItems.end(); ++ciFilterItems)
//           if(src.compare(static_cast<RouteTreeWidgetItem*>(*ciFilterItems)->route()))  // FIXME Ugly
//             break;
//         if(ciFilterItems == _srcFilterItems.end())
//           hide = true;
//         item->setHidden(ciFilterItems == filterItems.end());
//       //}
        
      RouteTreeWidgetItem* src_item = newSrcList->findItem(src);
      RouteTreeWidgetItem* dst_item = newDstList->findItem(dst);
      item->setHidden((src_item && src_item->isHidden()) || (dst_item && dst_item->isHidden()));
    }
    ++iRouteTree;
  }
  
  //routingChanged();
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void RouteDialog::songChanged(MusECore::SongChangedStruct_t v)
{
  if(v & SC_PORT_ALIAS_PREFERENCE)
  {
    const int idx = routeAliasList->findData(QVariant::fromValue<int>(MusEGlobal::config.preferredRouteNameOrAlias));
    if(idx != -1 && idx != routeAliasList->currentIndex())
    {
      routeAliasList->blockSignals(true);
      routeAliasList->setCurrentIndex(idx);
      routeAliasList->blockSignals(false);
    }
  }
  
  if(v & (SC_ROUTE | SC_CONFIG))
  {
    // Refill the lists of available external ports.
    tmpJackOutPorts = MusEGlobal::audioDevice->outputPorts();
    tmpJackInPorts = MusEGlobal::audioDevice->inputPorts();
    tmpJackMidiOutPorts = MusEGlobal::audioDevice->outputPorts(true);
    tmpJackMidiInPorts = MusEGlobal::audioDevice->inputPorts(true);
  }
  
  if(v & (SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED |
          SC_ROUTE | SC_CONFIG | SC_CHANNELS | SC_PORT_ALIAS_PREFERENCE)) 
  {
    removeItems();                // Remove unused items.
    addItems();                   // Add any new items.
    routeList->resizeColumnToContents(ROUTE_SRC_COL);
    routeList->resizeColumnToContents(ROUTE_DST_COL);
    
    // Now that column resizing is done, update all channel y values in source and destination lists.
    newDstList->computeChannelYValues();
    newSrcList->computeChannelYValues();
//     newDstList->scheduleDelayedLayout();
//     newSrcList->scheduleDelayedLayout();
    
    routeSelectionChanged();      // Init remove button.
    srcSelectionChanged();        // Init select button.
    connectionsWidget->update();  // Redraw the connections.
  }
}

//---------------------------------------------------------
//   routeSelectionChanged
//---------------------------------------------------------

void RouteDialog::routeSelectionChanged()
{
  QTreeWidgetItem* item = routeList->currentItem();
  if(item == 0)
  {
    connectButton->setEnabled(false);
    removeButton->setEnabled(false);
    return;
  }
  if(!item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() || !item->data(ROUTE_DST_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
  {
    connectButton->setEnabled(false);
    removeButton->setEnabled(false);
    return;
  }
  const MusECore::Route src = item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).value<MusECore::Route>();
  const MusECore::Route dst = item->data(ROUTE_DST_COL, RouteDialog::RouteRole).value<MusECore::Route>();
  RouteTreeWidgetItem* srcItem = newSrcList->findItem(src);
  RouteTreeWidgetItem* dstItem = newDstList->findItem(dst);
  newSrcList->blockSignals(true);
  newSrcList->setCurrentItem(srcItem);
  newSrcList->blockSignals(false);
  newDstList->blockSignals(true);
  newDstList->setCurrentItem(dstItem);
  newDstList->blockSignals(false);
  selectRoutes(true);
  if(srcItem)
    newSrcList->scrollToItem(srcItem, QAbstractItemView::PositionAtCenter);
    //newSrcList->scrollToItem(srcItem, QAbstractItemView::EnsureVisible);
  if(dstItem)
    newDstList->scrollToItem(dstItem, QAbstractItemView::PositionAtCenter);
    //newDstList->scrollToItem(dstItem, QAbstractItemView::EnsureVisible);
  connectionsWidget->update();
//   connectButton->setEnabled(MusECore::routeCanConnect(src, dst));
  connectButton->setEnabled(false);
//   removeButton->setEnabled(MusECore::routeCanDisconnect(src, dst));
//   removeButton->setEnabled(true);
  
  
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
// Special: Allow simulated midi track to midi port route (a route found in our 'local' routelist
//           but not in any track or port routelist) until multiple output routes are allowed
//           instead of just single port and channel properties. The route is exclusive.
      switch(src.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          switch(dst.type)
          {
            case MusECore::Route::MIDI_PORT_ROUTE:
              if(src.track->isMidiTrack())
              {
                MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(src.track);
                // We cannot 'remove' a simulated midi track output port and channel route.
                // (Midi port cannot be -1 meaning 'no port'.)
                // Only remove it if it's a different port or channel. 
                removeButton->setEnabled(mt->outPort() != dst.midiPort || mt->outChannel() != src.channel);
                return;
              }
            break;
            
            case MusECore::Route::TRACK_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
              break;
          }
        break;
        
        case MusECore::Route::MIDI_PORT_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
        break;
      }
#endif
  
  removeButton->setEnabled(true);
  
}

//---------------------------------------------------------
//   disconnectClicked
//---------------------------------------------------------

void RouteDialog::disconnectClicked()
{
  MusECore::PendingOperationList operations;
  QTreeWidgetItemIterator ii(routeList);
  while(*ii)
  {
    QTreeWidgetItem* item = *ii;
    if(item && item->isSelected() &&
       item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() && 
       item->data(ROUTE_DST_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
    {
      const MusECore::Route src = item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).value<MusECore::Route>();
      const MusECore::Route dst = item->data(ROUTE_DST_COL, RouteDialog::RouteRole).value<MusECore::Route>();
      //if(MusECore::routeCanDisconnect(src, dst))
//         operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
        
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
// Special: Allow simulated midi track to midi port route (a route found in our 'local' routelist
//           but not in any track or port routelist) until multiple output routes are allowed
//           instead of just single port and channel properties. The route is exclusive.
      switch(src.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          switch(dst.type)
          {
            case MusECore::Route::MIDI_PORT_ROUTE:
              if(src.track->isMidiTrack())
              {
//                 MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(src.track);
                // We cannot 'remove' a simulated midi track output port and channel route.
                // (Midi port cannot be -1 meaning 'no port'.)
                // Only remove it if it's a different port or channel. 
//                 if(mt->outPort() != dst.midiPort || mt->outChannel() != src.channel)
//                   operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
                ++ii;
                continue;
              }
            break;
            
            case MusECore::Route::TRACK_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
              break;
          }
        break;
        
        case MusECore::Route::MIDI_PORT_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
        break;
      }
#endif
        
      operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
        
    }      
    ++ii;
  }
  
  if(!operations.empty())
  {
    MusEGlobal::audio->msgExecutePendingOperations(operations, true);
  }
}

//---------------------------------------------------------
//   connectClicked
//---------------------------------------------------------

void RouteDialog::connectClicked()
{
  MusECore::PendingOperationList operations;
  MusECore::RouteList srcList;
  MusECore::RouteList dstList;
  newSrcList->getSelectedRoutes(srcList);
  newDstList->getSelectedRoutes(dstList);
  const int srcSelSz = srcList.size();
  const int dstSelSz = dstList.size();
  bool upd_trk_props = false;
  MusECore::MidiTrack::ChangedType_t changed = MusECore::MidiTrack::NothingChanged;

  for(int srcIdx = 0; srcIdx < srcSelSz; ++srcIdx)
  {
    const MusECore::Route& src = srcList.at(srcIdx);
    for(int dstIdx = 0; dstIdx < dstSelSz; ++dstIdx)
    {
      const MusECore::Route& dst = dstList.at(dstIdx);
      
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
// Special: Allow simulated midi track to midi port route (a route found in our 'local' routelist
//           but not in any track or port routelist) until multiple output routes are allowed
//           instead of just single port and channel properties. The route is exclusive.
      switch(src.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          switch(dst.type)
          {
            case MusECore::Route::MIDI_PORT_ROUTE:
              if(src.track->isMidiTrack())
              {
                MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(src.track);
                // We cannot 'remove' a simulated midi track output port and channel route.
                // (Midi port cannot be -1 meaning 'no port'.)
                // Only remove it if it's a different port or channel. 
                if(src.channel >= 0 && src.channel < MusECore::MUSE_MIDI_CHANNELS && (mt->outPort() != dst.midiPort || mt->outChannel() != src.channel))
                {
                  MusEGlobal::audio->msgIdle(true);
                  changed |= mt->setOutPortAndChannelAndUpdate(dst.midiPort, src.channel, false);
                  MusEGlobal::audio->msgIdle(false);
                  //MusEGlobal::audio->msgUpdateSoloStates();
                  //MusEGlobal::song->update(SC_MIDI_TRACK_PROP);
                  upd_trk_props = true;
                }
                continue;
              }
            break;
            
            case MusECore::Route::TRACK_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
              break;
          }
        break;
        
        case MusECore::Route::MIDI_PORT_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
        break;
      }
#endif
      
      if(MusECore::routeCanConnect(src, dst))
        operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::AddRoute));
    }
  }

  if(!operations.empty())
  {
    operations.add(MusECore::PendingOperationItem((MusECore::TrackList*)NULL, MusECore::PendingOperationItem::UpdateSoloStates));
    MusEGlobal::audio->msgExecutePendingOperations(operations, true,
      upd_trk_props ? (SC_ROUTE | ((changed & MusECore::MidiTrack::DrumMapChanged) ? SC_DRUMMAP : 0)) : 0);
//     MusEGlobal::song->update(SC_ROUTE | (upd_trk_props ? SC_MIDI_TRACK_PROP : 0));
    //MusEGlobal::song->update(SC_SOLO);
    //routingChanged();
  }
  else if(upd_trk_props)
    MusEGlobal::song->update(SC_ROUTE | ((changed & MusECore::MidiTrack::DrumMapChanged) ? SC_DRUMMAP : 0));
    
}  

//---------------------------------------------------------
//   srcSelectionChanged
//---------------------------------------------------------

void RouteDialog::srcSelectionChanged()
{
  DEBUG_PRST_ROUTES(stderr, "RouteDialog::srcSelectionChanged\n");  

  MusECore::RouteList srcList;
  MusECore::RouteList dstList;
  newSrcList->getSelectedRoutes(srcList);
  newDstList->getSelectedRoutes(dstList);
  const int srcSelSz = srcList.size();
  const int dstSelSz = dstList.size();
  //if(srcSelSz == 0 || dstSelSz == 0 || (srcSelSz > 1 && dstSelSz > 1))
  //{
  //  connectButton->setEnabled(false);
  //  removeButton->setEnabled(false);
  //  return;
  //}

  routeList->blockSignals(true);
  routeList->clearSelection();
  bool canConnect = false;
  QTreeWidgetItem* routesItem = 0;
  int routesSelCnt = 0;
  int routesRemoveCnt = 0;
  for(int srcIdx = 0; srcIdx < srcSelSz; ++srcIdx)
  {
    MusECore::Route& src = srcList.at(srcIdx);
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    int mt_to_mp_cnt = 0;
#endif
    for(int dstIdx = 0; dstIdx < dstSelSz; ++dstIdx)
    {
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
      bool useMTOutProps = false;
#endif
      MusECore::Route& dst = dstList.at(dstIdx);
      // Special for some item type combos: Before searching, cross-update the routes if necessary.
      // To minimize route copying, here on each iteration we alter each list route, but should be OK.
      switch(src.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          switch(dst.type)
          {
            case MusECore::Route::MIDI_PORT_ROUTE:
              if(src.track->isMidiTrack())
              {
                dst.channel = src.channel;
                
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
// Special: Allow simulated midi track to midi port route (a route found in our 'local' routelist
//           but not in any track or port routelist) until multiple output routes are allowed
//           instead of just single port and channel properties. The route is exclusive.
                useMTOutProps = true;
                MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(src.track);
                if(src.channel >= 0 && src.channel < MusECore::MUSE_MIDI_CHANNELS && (mt->outPort() != dst.midiPort || mt->outChannel() != src.channel))
                  ++mt_to_mp_cnt;
#endif
                
              }  
            break;
            
            case MusECore::Route::TRACK_ROUTE: case MusECore::Route::JACK_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: break;
          }
        break;

        case MusECore::Route::MIDI_PORT_ROUTE:
          switch(dst.type)
          {
            case MusECore::Route::TRACK_ROUTE: src.channel = dst.channel; break;
            case MusECore::Route::MIDI_PORT_ROUTE: case MusECore::Route::JACK_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: break;
          }
        break;
        
        case MusECore::Route::JACK_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: break;
      }
      
      QTreeWidgetItem* ri = findRoutesItem(src, dst);
      if(ri)
      {
        ri->setSelected(true);
        routesItem = ri;
        ++routesSelCnt;
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
        if(!useMTOutProps)
#endif
          ++routesRemoveCnt;
      }
      
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
      if(useMTOutProps)
        continue;
#endif
      
      if(MusECore::routeCanConnect(src, dst))
        canConnect = true;
//       if(MusECore::routeCanDisconnect(src, dst))
//         canDisconnect = true;
    }
    
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    // If there was one and only one selected midi port for a midi track, allow (simulated) connection.
    if(mt_to_mp_cnt == 1)
      canConnect = true;
#endif
    
  }
  
  if(routesSelCnt == 0)
    routeList->setCurrentItem(0);
  //routeList->setCurrentItem(routesItem);
  routeList->blockSignals(false);
  if(routesSelCnt == 1)
    routeList->scrollToItem(routesItem, QAbstractItemView::PositionAtCenter);

  selectRoutes(false);
  
  connectionsWidget->update();
  //connectButton->setEnabled(can_connect && (srcSelSz == 1 || dstSelSz == 1));
  connectButton->setEnabled(canConnect);
  //removeButton->setEnabled(can_disconnect);
//   removeButton->setEnabled(routesSelCnt > 0);
  removeButton->setEnabled(routesRemoveCnt != 0);
}

//---------------------------------------------------------
//   dstSelectionChanged
//---------------------------------------------------------

void RouteDialog::dstSelectionChanged()
{
  srcSelectionChanged();  
}

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void RouteDialog::closeEvent(QCloseEvent* e)
{
  emit closed();
  e->accept();
}

QTreeWidgetItem* RouteDialog::findRoutesItem(const MusECore::Route& src, const MusECore::Route& dst)
{
  int cnt = routeList->topLevelItemCount(); 
  for(int i = 0; i < cnt; ++i)
  {
    QTreeWidgetItem* item = routeList->topLevelItem(i);
    if(!item || !item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() || !item->data(ROUTE_DST_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
      continue;
    if(item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).value<MusECore::Route>() == src && item->data(ROUTE_DST_COL, RouteDialog::RouteRole).value<MusECore::Route>() == dst)
      return item;
  }
  return 0;
}
      
void RouteDialog::removeItems()
{
  QVector<QTreeWidgetItem*> itemsToDelete;
  
  newSrcList->getItemsToDelete(itemsToDelete);
  newDstList->getItemsToDelete(itemsToDelete);
  getRoutesToDelete(routeList, itemsToDelete);


  newSrcList->blockSignals(true);
  newDstList->blockSignals(true);
  routeList->blockSignals(true);
  
  if(!itemsToDelete.empty())
  {
    int cnt = itemsToDelete.size();
    for(int i = 0; i < cnt; ++i)
      delete itemsToDelete.at(i);
  }

  selectRoutes(false);
  
  routeList->blockSignals(false);
  newDstList->blockSignals(false);
  newSrcList->blockSignals(false);
  
  //connectionsWidget->update();
}

void RouteDialog::getRoutesToDelete(QTreeWidget* tree, QVector<QTreeWidgetItem*>& items_to_remove)
{
  const int iItemCount = tree->topLevelItemCount();
  for (int iItem = 0; iItem < iItemCount; ++iItem) 
  {
    QTreeWidgetItem *item = tree->topLevelItem(iItem);
    if(item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() && item->data(ROUTE_DST_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
    {        
      const MusECore::Route src = item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).value<MusECore::Route>();
      const MusECore::Route dst = item->data(ROUTE_DST_COL, RouteDialog::RouteRole).value<MusECore::Route>();
      
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
// Special: Allow simulated midi track to midi port route (a route found in our 'local' routelist
//           but not in any track or port routelist) until multiple output routes are allowed
//           instead of just single port and channel properties. The route is exclusive.
      switch(src.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          switch(dst.type)
          {
            case MusECore::Route::MIDI_PORT_ROUTE:
              if(src.track->isMidiTrack())
              {
                MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(src.track);
                // We cannot 'remove' a simulated midi track output port and channel route.
                // (Midi port cannot be -1 meaning 'no port'.)
                // Only remove it if it's a different port or channel. 
                if(mt->outPort() != dst.midiPort || mt->outChannel() != src.channel)
                  items_to_remove.append(item);
                continue;
              }
            break;
            
            case MusECore::Route::TRACK_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
              break;
          }
        break;
        
        case MusECore::Route::MIDI_PORT_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
        break;
      }
#endif

      if(!MusECore::routeCanDisconnect(src, dst))
        items_to_remove.append(item);
    }
  }
}

void RouteDialog::selectRoutes(bool doNormalSelections)
{
  const QList<QTreeWidgetItem*> route_list = routeList->selectedItems();
  newSrcList->selectRoutes(route_list, doNormalSelections);
  newDstList->selectRoutes(route_list, doNormalSelections);
}  

void RouteDialog::addItems()
{
  RouteTreeWidgetItem* srcCatItem;
  RouteTreeWidgetItem* dstCatItem;
  RouteTreeWidgetItem* item;
  RouteTreeWidgetItem* subitem;
  QTreeWidgetItem* routesItem;
  // Tried wrap flags: Doesn't work (at least not automatically).
  //const int align_flags = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap | Qt::TextWrapAnywhere;
  const int align_flags = Qt::AlignLeft | Qt::AlignVCenter;
  
  //
  // Tracks:
  //
  
  dstCatItem = newDstList->findCategoryItem(tracksCat);
  srcCatItem = newSrcList->findCategoryItem(tracksCat);
  MusECore::TrackList* tl = MusEGlobal::song->tracks();
  for(MusECore::ciTrack i = tl->begin(); i != tl->end(); ++i) 
  {
    MusECore::Track* track = *i;
    const MusECore::RouteCapabilitiesStruct rcaps = track->routeCapabilities();
    int src_chans = 0;
    int dst_chans = 0;
    bool src_routable = false;
    bool dst_routable = false;

    switch(track->type())
    {
      case MusECore::Track::AUDIO_INPUT:
        src_chans = rcaps._trackChannels._outChannels;
        dst_chans = rcaps._jackChannels._inChannels;
        src_routable = rcaps._trackChannels._outRoutable;
        dst_routable = rcaps._jackChannels._inRoutable || rcaps._trackChannels._inRoutable; // Support Audio Out to Audio In omni route.
      break;
      case MusECore::Track::AUDIO_OUTPUT:
        src_chans = rcaps._jackChannels._outChannels;
        dst_chans = rcaps._trackChannels._inChannels;
        src_routable = rcaps._jackChannels._outRoutable || rcaps._trackChannels._outRoutable; // Support Audio Out to Audio In omni route.
        dst_routable = rcaps._trackChannels._inRoutable;
      break;
      case MusECore::Track::MIDI:
      case MusECore::Track::DRUM:
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
        src_chans = MusECore::MUSE_MIDI_CHANNELS;
#else        
        src_chans = rcaps._midiPortChannels._outChannels;
#endif        
        
        dst_chans = rcaps._midiPortChannels._inChannels;
        src_routable = rcaps._midiPortChannels._outRoutable || rcaps._trackChannels._outRoutable; // Support Midi Track to Audio In omni route.
        dst_routable = rcaps._midiPortChannels._inRoutable;
      break;
      case MusECore::Track::WAVE:
      case MusECore::Track::AUDIO_AUX:
      case MusECore::Track::AUDIO_SOFTSYNTH:
      case MusECore::Track::AUDIO_GROUP:
        src_chans = rcaps._trackChannels._outChannels;
        dst_chans = rcaps._trackChannels._inChannels;
        src_routable = rcaps._trackChannels._outRoutable;
        dst_routable = rcaps._trackChannels._inRoutable;
      break;
    }
  
    
      //
      // DESTINATION section:
      //
    
      if(dst_routable || dst_chans != 0)
      {
        const MusECore::Route r(track, -1);
        item = newDstList->findItem(r, RouteTreeWidgetItem::RouteItem);
        if(item)
        {
          // Update the text.
          item->setText(ROUTE_NAME_COL, track->displayName());
        }
        else
        {
          if(!dstCatItem)
          {
            newDstList->blockSignals(true);
            //dstCatItem = new QTreeWidgetItem(newDstList, QStringList() << tracksCat << QString() );
            dstCatItem = new RouteTreeWidgetItem(newDstList, QStringList() << tracksCat, RouteTreeWidgetItem::CategoryItem, false);
            dstCatItem->setFlags(Qt::ItemIsEnabled);
            QFont fnt = dstCatItem->font(ROUTE_NAME_COL);
            fnt.setBold(true);
            fnt.setItalic(true);
            //fnt.setPointSize(fnt.pointSize() + 2);
            dstCatItem->setFont(ROUTE_NAME_COL, fnt);
            dstCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
            dstCatItem->setExpanded(true);
            newDstList->blockSignals(false);
          }
          newDstList->blockSignals(true);
          //item = new QTreeWidgetItem(dstCatItem, QStringList() << track->name() << trackLabel );
          item = new RouteTreeWidgetItem(dstCatItem, QStringList() << track->displayName(), RouteTreeWidgetItem::RouteItem, false, r);
          //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(r));
          item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
          item->setTextAlignment(ROUTE_NAME_COL, align_flags);
          newDstList->blockSignals(false);
          //dstCatItem->setExpanded(true); // REMOVE Tim. For test only.
        }
        if(QPixmap* r_pm = r.icon(false, false))
          item->setIcon(ROUTE_NAME_COL, QIcon(*r_pm));
        
        if(dst_chans != 0)
        {
          const MusECore::Route sub_r(track, 0, track->isMidiTrack() ? -1 : 1);
          subitem = newDstList->findItem(sub_r, RouteTreeWidgetItem::ChannelsItem);
          if(subitem)
          {
            // Update the channel y values.
            //subitem->computeChannelYValues();
            // Update the number of channels.
            subitem->setChannels();
          }
          else
//             if(!subitem)
          {
            newDstList->blockSignals(true);
            item->setExpanded(true);
            subitem = new RouteTreeWidgetItem(item, QStringList() << QString(), RouteTreeWidgetItem::ChannelsItem, false, sub_r);
            subitem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            subitem->setTextAlignment(ROUTE_NAME_COL, align_flags);
            newDstList->blockSignals(false);
          }
          // Update the channel y values.
          //subitem->computeChannelYValues();
        }
      }

      const MusECore::RouteList* irl = track->inRoutes();
      for(MusECore::ciRoute r = irl->begin(); r != irl->end(); ++r) 
      {
        // Ex. Params:  src: TrackA, Channel  2, Remote Channel -1   dst: TrackB channel  4 Remote Channel -1
        //      After: [src  TrackA, Channel  4, Remote Channel  2]  dst: TrackB channel  2 Remote Channel  4
        //
        // Ex.
        //     Params:  src: TrackA, Channel  2, Remote Channel -1   dst: JackAA channel -1 Remote Channel -1
        //      After: (src  TrackA, Channel -1, Remote Channel  2)  dst: JackAA channel  2 Remote Channel -1
        MusECore::Route src;
        MusECore::Route dst;
        QString srcName;
        QString dstName = track->displayName();
        switch(r->type)
        {
          case MusECore::Route::JACK_ROUTE: 
            src = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, -1, -1, -1, r->persistentJackPortName);
            dst = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, track,       r->channel,       1, -1, 0);
            srcName = r->displayName(MusEGlobal::config.preferredRouteNameOrAlias);
          break;  
          case MusECore::Route::MIDI_DEVICE_ROUTE: 
            continue;
          break;  
          // Midi ports taken care of below...
          case MusECore::Route::MIDI_PORT_ROUTE: 
            continue;
          break;  
            
          case MusECore::Route::TRACK_ROUTE: 
            src = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, r->track, r->remoteChannel, r->channels, -1, 0);
            dst = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, track,    r->channel,       r->channels, -1, 0);
            srcName = r->displayName();
          break;  
        }

        if(src.channel != -1)
          srcName += QString(" [") + QString::number(src.channel) + QString("]");
        if(dst.channel != -1)
          dstName += QString(" [") + QString::number(dst.channel) + QString("]");
  
        routesItem = findRoutesItem(src, dst);
        if(routesItem)
        {
          // Update the text.
          routesItem->setText(ROUTE_SRC_COL, srcName);
          routesItem->setText(ROUTE_DST_COL, dstName);
        }
        else
        {
          routeList->blockSignals(true);
          routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
          routesItem->setTextAlignment(ROUTE_SRC_COL, align_flags);
          routesItem->setTextAlignment(ROUTE_DST_COL, align_flags);
          routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
          routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
          if(QPixmap* src_pm = src.icon(true, false))
            routesItem->setIcon(ROUTE_SRC_COL, QIcon(*src_pm));
          if(QPixmap* dst_pm = dst.icon(false, false))
            routesItem->setIcon(ROUTE_DST_COL, QIcon(*dst_pm));
          routeList->blockSignals(false);
        }
      }

      //
      // SOURCE section:
      //

      if(src_routable || src_chans != 0
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
         || track->isMidiTrack()
#endif                
        )
      {
        const MusECore::Route r(track, -1);
        item = newSrcList->findItem(r, RouteTreeWidgetItem::RouteItem);
        if(item)
        {
          // Update the text.
          item->setText(ROUTE_NAME_COL, track->displayName());
        }
        else
        {
          if(!srcCatItem)
          {
            newSrcList->blockSignals(true);
            //srcCatItem = new QTreeWidgetItem(newSrcList, QStringList() << tracksCat << QString() );
            srcCatItem = new RouteTreeWidgetItem(newSrcList, QStringList() << tracksCat, RouteTreeWidgetItem::CategoryItem, true);
            srcCatItem->setFlags(Qt::ItemIsEnabled);
            QFont fnt = srcCatItem->font(ROUTE_NAME_COL);
            fnt.setBold(true);
            fnt.setItalic(true);
            //fnt.setPointSize(fnt.pointSize() + 2);
            srcCatItem->setFont(ROUTE_NAME_COL, fnt);
            srcCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
            srcCatItem->setExpanded(true);
            newSrcList->blockSignals(false);
          }
          newSrcList->blockSignals(true);
          //item = new QTreeWidgetItem(srcCatItem, QStringList() << track->name() << trackLabel );
          item = new RouteTreeWidgetItem(srcCatItem, QStringList() << track->displayName(), RouteTreeWidgetItem::RouteItem, true, r);
          //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(r));
          item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
          item->setTextAlignment(ROUTE_NAME_COL, align_flags);
          newSrcList->blockSignals(false);
        }
        if(QPixmap* r_pm = r.icon(true, false))
          item->setIcon(ROUTE_NAME_COL, QIcon(*r_pm));
        
        if(src_chans != 0
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
             || track->isMidiTrack()
#endif                
            )
        {
        //for(int channel = 0; channel < MusECore::MUSE_MIDI_CHANNELS; ++channel)
        //{
          const MusECore::Route sub_r(track, 0, track->isMidiTrack() ? -1 : 1);
          subitem = newSrcList->findItem(sub_r, RouteTreeWidgetItem::ChannelsItem);
          if(subitem)
          {
            // Update the channel y values.
            //subitem->computeChannelYValues();
            // Update the number of channels.
            subitem->setChannels();
          }
          else
//             if(!subitem)
          {
            newSrcList->blockSignals(true);
            item->setExpanded(true);
            
            subitem = new RouteTreeWidgetItem(item, QStringList() << QString(), RouteTreeWidgetItem::ChannelsItem, true, sub_r
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
            , track->isMidiTrack() ? RouteTreeWidgetItem::ExclusiveMode : RouteTreeWidgetItem::NormalMode
#endif                
            );
            
            subitem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            subitem->setTextAlignment(ROUTE_NAME_COL, align_flags);
            newSrcList->blockSignals(false);
          }
          // Update the channel y values.
          //subitem->computeChannelYValues();
        }
      }

      const MusECore::RouteList* orl = track->outRoutes();
      for(MusECore::ciRoute r = orl->begin(); r != orl->end(); ++r) 
      {
        // Ex. Params:  src: TrackA, Channel  2, Remote Channel -1   dst: TrackB channel  4 Remote Channel -1
        //      After:  src: TrackA, Channel  4, Remote Channel  2  [dst: TrackB channel  2 Remote Channel  4]
        //
        // Ex.
        //     Params:  src: TrackA, Channel  2, Remote Channel -1   dst: JackAA channel -1 Remote Channel -1
        //      After: (src: TrackA, Channel -1, Remote Channel  2)  dst: JackAA channel  2 Remote Channel -1
        MusECore::Route src;
        MusECore::Route dst;
        QString srcName = track->displayName();
        QString dstName;
        switch(r->type)
        {
          case MusECore::Route::JACK_ROUTE: 
            src = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, track,       r->channel,       1, -1, 0);
            dst = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, -1, -1, -1, r->persistentJackPortName);
            dstName = r->displayName(MusEGlobal::config.preferredRouteNameOrAlias);
          break;  
          case MusECore::Route::MIDI_DEVICE_ROUTE: 
            continue;  // TODO
          break;  
          // Midi ports taken care of below...
          case MusECore::Route::MIDI_PORT_ROUTE: 
            continue;
          break;  
          
          case MusECore::Route::TRACK_ROUTE: 
            src = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, track, r->channel, r->channels, -1, 0);
            dst = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, r->track, r->remoteChannel, r->channels, -1, 0);
            dstName = r->displayName();
          break;  
        }

        if(src.channel != -1)
          srcName += QString(" [") + QString::number(src.channel) + QString("]");
        if(dst.channel != -1)
          dstName += QString(" [") + QString::number(dst.channel) + QString("]");
        
        routesItem = findRoutesItem(src, dst);
        if(routesItem)
        {
          // Update the text.
          routesItem->setText(ROUTE_SRC_COL, srcName);
          routesItem->setText(ROUTE_DST_COL, dstName);
        }
        else
        {
          routeList->blockSignals(true);
          routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
          routesItem->setTextAlignment(ROUTE_SRC_COL, align_flags);
          routesItem->setTextAlignment(ROUTE_DST_COL, align_flags);
          routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
          routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
          if(QPixmap* src_pm = src.icon(true, false))
            routesItem->setIcon(ROUTE_SRC_COL, QIcon(*src_pm));
          if(QPixmap* dst_pm = dst.icon(false, false))
            routesItem->setIcon(ROUTE_DST_COL, QIcon(*dst_pm));
          routeList->blockSignals(false);
        }
      }
    }
  //}

  
  //
  // MIDI ports:
  //
  
  const QString none_str = tr("<none>");
  dstCatItem = newDstList->findCategoryItem(midiPortsCat);
  srcCatItem = newSrcList->findCategoryItem(midiPortsCat);
  for(int i = 0; i < MusECore::MIDI_PORTS; ++i) 
  {
    MusECore::MidiPort* mp = &MusEGlobal::midiPorts[i];
    if(!mp)
      continue;
    MusECore::MidiDevice* md = mp->device();
    // Synth are tracks and devices. Don't list them as devices here, list them as tracks, above.
    //if(md && md->deviceType() == MusECore::MidiDevice::SYNTH_MIDI)
    //  continue;
    
    QString mdname;
    mdname = QString::number(i + 1) + QString(":");
    mdname += md ? md->name() : none_str;
      
    //
    // DESTINATION section:
    //


#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    bool non_route_found = false;
    // Simulate routes for each midi track's output port and channel properties.
    MusECore::MidiTrackList* tl = MusEGlobal::song->midis();
    for(MusECore::ciMidiTrack imt = tl->begin(); imt != tl->end(); ++imt)
    {
      MusECore::MidiTrack* mt = *imt;
      const int port = mt->outPort();
      const int chan = mt->outChannel();
      if(port != i)
        continue;
      non_route_found = true;
      const MusECore::Route src = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, mt, chan, -1, -1, NULL);
      const MusECore::Route dst(i, chan);
      const QString srcName = mt->displayName() + QString(" [") + QString::number(chan) + QString("]");
      const QString dstName = mdname;
      routesItem = findRoutesItem(src, dst);
      if(routesItem)
      {
        // Update the text.
        routesItem->setText(ROUTE_SRC_COL, srcName);
        routesItem->setText(ROUTE_DST_COL, dstName);
      }
      else
      {
        routeList->blockSignals(true);
        routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
        routesItem->setTextAlignment(ROUTE_SRC_COL, align_flags);
        routesItem->setTextAlignment(ROUTE_DST_COL, align_flags);
        routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
        routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
        if(QPixmap* src_pm = src.icon(true, true))
          routesItem->setIcon(ROUTE_SRC_COL, QIcon(*src_pm));
        if(QPixmap* dst_pm = dst.icon(false, true))
          routesItem->setIcon(ROUTE_DST_COL, QIcon(*dst_pm));
        routeList->blockSignals(false);
      }
    }
#endif  // _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    
    //if(md->rwFlags() & 0x02) // Readable
//     if(md->rwFlags() & 0x01)   // Writeable
    //if(md->rwFlags() & 0x03) // Both readable and writeable need to be shown
    
    // Show either all midi ports, or only ports that have a device or have input routes.
    if(allMidiPortsButton->isChecked() || md || !mp->inRoutes()->empty()
       #ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
       || non_route_found
       #endif
    )
    {
      const MusECore::Route dst(i, -1);
      item = newDstList->findItem(dst, RouteTreeWidgetItem::RouteItem);
      if(item)
      {
        // Update the text.
        item->setText(ROUTE_NAME_COL, mdname);
      }
      else
      {
        if(!dstCatItem)
        {
          newDstList->blockSignals(true);
          //dstCatItem = new QTreeWidgetItem(newDstList, QStringList() << midiDevicesCat << QString() );
          dstCatItem = new RouteTreeWidgetItem(newDstList, QStringList() << midiPortsCat, RouteTreeWidgetItem::CategoryItem, false);
          dstCatItem->setFlags(Qt::ItemIsEnabled);
          QFont fnt = dstCatItem->font(ROUTE_NAME_COL);
          fnt.setBold(true);
          fnt.setItalic(true);
          //fnt.setPointSize(fnt.pointSize() + 2);
          dstCatItem->setFont(ROUTE_NAME_COL, fnt);
          dstCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
          dstCatItem->setExpanded(true);
          dstCatItem->setIcon(ROUTE_NAME_COL, QIcon(*ankerSVGIcon));
          newDstList->blockSignals(false);
        }
        newDstList->blockSignals(true);
          
        item = new RouteTreeWidgetItem(dstCatItem, QStringList() << mdname, RouteTreeWidgetItem::RouteItem, false, dst);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setTextAlignment(ROUTE_NAME_COL, align_flags);
        newDstList->blockSignals(false);
      }

      const MusECore::RouteList* rl = mp->inRoutes();
      for(MusECore::ciRoute r = rl->begin(); r != rl->end(); ++r) 
      {
        switch(r->type)
        {
          case MusECore::Route::TRACK_ROUTE: 
            
#ifndef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
          {
            if(!r->track || !r->track->isMidiTrack())
              continue;
            
            const MusECore::Route& src = *r;
            QString srcName = r->displayName();
            QString dstName = mdname;
            //const MusECore::Route dst(i, -1);
            const MusECore::Route dst(i, src.channel);

            if(src.channel != -1)
              srcName += QString(" [") + QString::number(src.channel) + QString("]");
            
            routesItem = findRoutesItem(src, dst);
            if(routesItem)
            {
              // Update the text.
              routesItem->setText(ROUTE_SRC_COL, srcName);
              routesItem->setText(ROUTE_DST_COL, dstName);
            }
            else
            {
              routeList->blockSignals(true);
              routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
              routesItem->setTextAlignment(ROUTE_SRC_COL, align_flags);
              routesItem->setTextAlignment(ROUTE_DST_COL, align_flags);
              routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
              routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
              if(QPixmap* src_pm = src.icon(true, true))
                routesItem->setIcon(ROUTE_SRC_COL, QIcon(*src_pm));
              if(QPixmap* dst_pm = dst.icon(false, true))
                routesItem->setIcon(ROUTE_DST_COL, QIcon(*dst_pm));
              routeList->blockSignals(false);
            }
          }
#endif  // _USE_MIDI_TRACK_OUT_ROUTES_

          break;
          
          case MusECore::Route::JACK_ROUTE: 
          case MusECore::Route::MIDI_PORT_ROUTE: 
          case MusECore::Route::MIDI_DEVICE_ROUTE: 
          break;
        }
      }
    }
    
    //
    // SOURCE section:
    //
    
    //if(md->rwFlags() & 0x01) // Writeable
//     if(md->rwFlags() & 0x02) // Readable
    //if(md->rwFlags() & 0x03) // Both readable and writeable need to be shown
    
    // Show only ports that have a device, or have output routes.
    if(allMidiPortsButton->isChecked() || md || !mp->outRoutes()->empty())
    {
      const MusECore::Route src(i, -1);
      item = newSrcList->findItem(src, RouteTreeWidgetItem::RouteItem);
      if(item)
      {
        // Update the text.
        item->setText(ROUTE_NAME_COL, mdname);
      }
      else
      {
        if(!srcCatItem)
        {
          newSrcList->blockSignals(true);
          //srcCatItem = new QTreeWidgetItem(newSrcList, QStringList() << midiDevicesCat << QString() );
          srcCatItem = new RouteTreeWidgetItem(newSrcList, QStringList() << midiPortsCat, RouteTreeWidgetItem::CategoryItem, true);
          srcCatItem->setFlags(Qt::ItemIsEnabled);
          QFont fnt = srcCatItem->font(ROUTE_NAME_COL);
          fnt.setBold(true);
          fnt.setItalic(true);
          //fnt.setPointSize(fnt.pointSize() + 2);
          srcCatItem->setFont(ROUTE_NAME_COL, fnt);
          srcCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
          srcCatItem->setExpanded(true);
          srcCatItem->setIcon(ROUTE_NAME_COL, QIcon(*ankerSVGIcon));
          newSrcList->blockSignals(false);
        }
        newSrcList->blockSignals(true);
        
        //item = new QTreeWidgetItem(srcCatItem, QStringList() << mdname << midiDeviceLabel );
        item = new RouteTreeWidgetItem(srcCatItem, QStringList() << mdname, RouteTreeWidgetItem::RouteItem, true, src);
        //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setTextAlignment(ROUTE_NAME_COL, align_flags);
        newSrcList->blockSignals(false);
      }

      const MusECore::RouteList* rl = mp->outRoutes();
      for(MusECore::ciRoute r = rl->begin(); r != rl->end(); ++r) 
      {
        // Ex. Params:  src: TrackA, Channel  2, Remote Channel -1   dst: TrackB channel  4 Remote Channel -1
        //      After:  src: TrackA, Channel  4, Remote Channel  2  [dst: TrackB channel  2 Remote Channel  4]
        //
        // Ex.
        //     Params:  src: TrackA, Channel  2, Remote Channel -1   dst: JackAA channel -1 Remote Channel -1
        //      After: (src: TrackA, Channel -1, Remote Channel  2)  dst: JackAA channel  2 Remote Channel -1
        //MusECore::Route src(md, -1);
        //MusECore::Route dst;
        //QString srcName = mdname;
        //QString dstName;
        switch(r->type)
        {
          case MusECore::Route::TRACK_ROUTE: 
          {
            if(!r->track || !r->track->isMidiTrack())
              continue;
            
            //const MusECore::Route dst = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, -1, -1, -1, r->persistentJackPortName);
            const MusECore::Route& dst = *r;
            QString dstName = r->displayName();
            QString srcName = mdname;
            //const MusECore::Route src(i, -1);
            const MusECore::Route src(i, dst.channel);

            //if(src.channel != -1)
            //  srcName += QString(" [") + QString::number(src.channel) + QString("]");
            if(dst.channel != -1)
              dstName += QString(" [") + QString::number(dst.channel) + QString("]");
            
            routesItem = findRoutesItem(src, dst);
            if(routesItem)
            {
              // Update the text.
              routesItem->setText(ROUTE_SRC_COL, srcName);
              routesItem->setText(ROUTE_DST_COL, dstName);
            }
            else
            {
              routeList->blockSignals(true);
              routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
              routesItem->setTextAlignment(ROUTE_SRC_COL, align_flags);
              routesItem->setTextAlignment(ROUTE_DST_COL, align_flags);
              routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
              routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
              if(QPixmap* src_pm = src.icon(true, true))
                routesItem->setIcon(ROUTE_SRC_COL, QIcon(*src_pm));
              if(QPixmap* dst_pm = dst.icon(false, true))
                routesItem->setIcon(ROUTE_DST_COL, QIcon(*dst_pm));
              routeList->blockSignals(false);
            }
//             if(!r->jackPort)
//               routesItem->setBackground(ROUTE_DST_COL, routesItem->background(ROUTE_DST_COL).color().darker());
          }
          break;
          
          case MusECore::Route::JACK_ROUTE:
          case MusECore::Route::MIDI_DEVICE_ROUTE: 
          case MusECore::Route::MIDI_PORT_ROUTE: 
            continue;
        }
      }
    }
  }

  
  //
  // MIDI devices:
  //
  
  dstCatItem = newDstList->findCategoryItem(midiDevicesCat);
  srcCatItem = newSrcList->findCategoryItem(midiDevicesCat);
  for(MusECore::iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
  {
    MusECore::MidiDevice* md = *i;
    // Synth are tracks and devices. Don't list them as devices here, list them as tracks, above.
    if(md->deviceType() == MusECore::MidiDevice::SYNTH_MIDI)
      continue;
    
//     QString mdname;
//     if(md->midiPort() != -1)
//       mdname = QString::number(md->midiPort() + 1) + QString(":");
//     mdname += md->name();
    QString mdname = md->name();
    //
    // DESTINATION section:
    //
    
    //if(md->rwFlags() & 0x02) // Readable
    //if(md->rwFlags() & 0x01)   // Writeable
    if(md->rwFlags() & 0x03) // Both readable and writeable need to be shown
    {
      const MusECore::Route dst(md, -1);
      item = newDstList->findItem(dst, RouteTreeWidgetItem::RouteItem);
      if(item)
      {
        // Update the text.
        item->setText(ROUTE_NAME_COL, mdname);
      }
      else
      {
        if(!dstCatItem)
        {
          newDstList->blockSignals(true);
          //dstCatItem = new QTreeWidgetItem(newDstList, QStringList() << midiDevicesCat << QString() );
          dstCatItem = new RouteTreeWidgetItem(newDstList, QStringList() << midiDevicesCat, RouteTreeWidgetItem::CategoryItem, false);
          dstCatItem->setFlags(Qt::ItemIsEnabled);
          QFont fnt = dstCatItem->font(ROUTE_NAME_COL);
          fnt.setBold(true);
          fnt.setItalic(true);
          //fnt.setPointSize(fnt.pointSize() + 2);
          dstCatItem->setFont(ROUTE_NAME_COL, fnt);
          dstCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
          dstCatItem->setExpanded(true);
          newDstList->blockSignals(false);
        }
        newDstList->blockSignals(true);
          
        //item = new QTreeWidgetItem(dstCatItem, QStringList() << mdname << midiDeviceLabel );
        item = new RouteTreeWidgetItem(dstCatItem, QStringList() << mdname, RouteTreeWidgetItem::RouteItem, false, dst);
        //item->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setTextAlignment(ROUTE_NAME_COL, align_flags);
        newDstList->blockSignals(false);
      }

      const MusECore::RouteList* rl = md->inRoutes();
      for(MusECore::ciRoute r = rl->begin(); r != rl->end(); ++r) 
      {
        // Ex. Params:  src: TrackA, Channel  2, Remote Channel -1   dst: TrackB channel  4 Remote Channel -1
        //      After: [src  TrackA, Channel  4, Remote Channel  2]  dst: TrackB channel  2 Remote Channel  4
        //
        // Ex.
        //     Params:  src: TrackA, Channel  2, Remote Channel -1   dst: JackAA channel -1 Remote Channel -1
        //      After: (src  TrackA, Channel -1, Remote Channel  2)  dst: JackAA channel  2 Remote Channel -1
        switch(r->type)
        {
          case MusECore::Route::JACK_ROUTE: 
          {
            const MusECore::Route src = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, -1, -1, -1, r->persistentJackPortName);
            QString srcName = r->displayName(MusEGlobal::config.preferredRouteNameOrAlias);
            QString dstName = mdname;
            const MusECore::Route dst(md, -1);

            if(src.channel != -1)
              srcName += QString(" [") + QString::number(src.channel) + QString("]");
            if(dst.channel != -1)
              dstName += QString(" [") + QString::number(dst.channel) + QString("]");
            
            routesItem = findRoutesItem(src, dst);
            if(routesItem)
            {
              // Update the text.
              routesItem->setText(ROUTE_SRC_COL, srcName);
              routesItem->setText(ROUTE_DST_COL, dstName);
            }
            else
            {
              routeList->blockSignals(true);
              routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
              routesItem->setTextAlignment(ROUTE_SRC_COL, align_flags);
              routesItem->setTextAlignment(ROUTE_DST_COL, align_flags);
              routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
              routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
              if(QPixmap* src_pm = src.icon(true, true))
                routesItem->setIcon(ROUTE_SRC_COL, QIcon(*src_pm));
              if(QPixmap* dst_pm = dst.icon(false, true))
                routesItem->setIcon(ROUTE_DST_COL, QIcon(*dst_pm));
              routeList->blockSignals(false);
            }

            QBrush br;
            if(!r->jackPort) {
              br = QBrush(Qt::red);
              routesItem->setBackground(ROUTE_SRC_COL, br);
            }
          }
          break;
          
          case MusECore::Route::MIDI_DEVICE_ROUTE: 
          case MusECore::Route::MIDI_PORT_ROUTE: 
          case MusECore::Route::TRACK_ROUTE: 
            continue;
        }
      }
    }
    
    //
    // SOURCE section:
    //
    
    //if(md->rwFlags() & 0x01) // Writeable
    //if(md->rwFlags() & 0x02) // Readable
    if(md->rwFlags() & 0x03) // Both readable and writeable need to be shown
    {
      const MusECore::Route src(md, -1);
      item = newSrcList->findItem(src, RouteTreeWidgetItem::RouteItem);
      if(item)
      {
        // Update the text.
        item->setText(ROUTE_NAME_COL, mdname);
      }
      else
      {
        if(!srcCatItem)
        {
          newSrcList->blockSignals(true);
          //srcCatItem = new QTreeWidgetItem(newSrcList, QStringList() << midiDevicesCat << QString() );
          srcCatItem = new RouteTreeWidgetItem(newSrcList, QStringList() << midiDevicesCat, RouteTreeWidgetItem::CategoryItem, true);
          srcCatItem->setFlags(Qt::ItemIsEnabled);
          QFont fnt = srcCatItem->font(ROUTE_NAME_COL);
          fnt.setBold(true);
          fnt.setItalic(true);
          //fnt.setPointSize(fnt.pointSize() + 2);
          srcCatItem->setFont(ROUTE_NAME_COL, fnt);
          srcCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
          srcCatItem->setExpanded(true);
          newSrcList->blockSignals(false);
        }
        newSrcList->blockSignals(true);
        //item = new QTreeWidgetItem(srcCatItem, QStringList() << mdname << midiDeviceLabel );
        item = new RouteTreeWidgetItem(srcCatItem, QStringList() << mdname, RouteTreeWidgetItem::RouteItem, true, src);
        //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setTextAlignment(ROUTE_NAME_COL, align_flags);
        newSrcList->blockSignals(false);
      }

      const MusECore::RouteList* rl = md->outRoutes();
      for(MusECore::ciRoute r = rl->begin(); r != rl->end(); ++r) 
      {
        // Ex. Params:  src: TrackA, Channel  2, Remote Channel -1   dst: TrackB channel  4 Remote Channel -1
        //      After:  src: TrackA, Channel  4, Remote Channel  2  [dst: TrackB channel  2 Remote Channel  4]
        //
        // Ex.
        //     Params:  src: TrackA, Channel  2, Remote Channel -1   dst: JackAA channel -1 Remote Channel -1
        //      After: (src: TrackA, Channel -1, Remote Channel  2)  dst: JackAA channel  2 Remote Channel -1
        switch(r->type)
        {
          case MusECore::Route::JACK_ROUTE:
          {
            const MusECore::Route dst = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, -1, -1, -1, r->persistentJackPortName);
            QString dstName = r->displayName(MusEGlobal::config.preferredRouteNameOrAlias);
            QString srcName = mdname;
            const MusECore::Route src(md, -1);

            if(src.channel != -1)
              srcName += QString(" [") + QString::number(src.channel) + QString("]");
            if(dst.channel != -1)
              dstName += QString(" [") + QString::number(dst.channel) + QString("]");
            
            routesItem = findRoutesItem(src, dst);
            if(routesItem)
            {
              // Update the text.
              routesItem->setText(ROUTE_SRC_COL, srcName);
              routesItem->setText(ROUTE_DST_COL, dstName);
            }
            else
            {
              routeList->blockSignals(true);
              routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
              routesItem->setTextAlignment(ROUTE_SRC_COL, align_flags);
              routesItem->setTextAlignment(ROUTE_DST_COL, align_flags);
              routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
              routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
              if(QPixmap* src_pm = src.icon(true, true))
                routesItem->setIcon(ROUTE_SRC_COL, QIcon(*src_pm));
              if(QPixmap* dst_pm = dst.icon(false, true))
                routesItem->setIcon(ROUTE_DST_COL, QIcon(*dst_pm));
              routeList->blockSignals(false);
            }
            
            QBrush br;
            if(!r->jackPort) {
              br = QBrush(Qt::red);
              routesItem->setBackground(ROUTE_DST_COL, br);
            }
          }
          break;
          
          case MusECore::Route::MIDI_DEVICE_ROUTE: 
          case MusECore::Route::MIDI_PORT_ROUTE: 
          case MusECore::Route::TRACK_ROUTE: 
            continue;
        }
      }
    }
  }

  //
  // JACK ports:
  //
  
  if(MusEGlobal::checkAudioDevice())
  {
    //------------
    // Jack audio:
    //------------
    
    srcCatItem = newSrcList->findCategoryItem(jackCat);
    MusECore::RouteList in_rl;
    for(std::list<QString>::iterator i = tmpJackOutPorts.begin(); i != tmpJackOutPorts.end(); ++i)
    {
      const MusECore::Route in_r(*i, false, -1, MusECore::Route::JACK_ROUTE);
      item = newSrcList->findItem(in_r, RouteTreeWidgetItem::RouteItem);
      if(item)
      {
        // Update the text.
        item->setText(ROUTE_NAME_COL, in_r.displayName(MusEGlobal::config.preferredRouteNameOrAlias));
      }
      else
      {
        if(!srcCatItem)
        {
          newSrcList->blockSignals(true);
          //srcCatItem = new QTreeWidgetItem(newSrcList, QStringList() << jackCat << QString() );
          srcCatItem = new RouteTreeWidgetItem(newSrcList, QStringList() << jackCat, RouteTreeWidgetItem::CategoryItem, true);
          srcCatItem->setFlags(Qt::ItemIsEnabled);
          QFont fnt = srcCatItem->font(ROUTE_NAME_COL);
          fnt.setBold(true);
          fnt.setItalic(true);
          //fnt.setPointSize(fnt.pointSize() + 2);
          srcCatItem->setFont(ROUTE_NAME_COL, fnt);
          srcCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
          srcCatItem->setExpanded(true);
          srcCatItem->setIcon(ROUTE_NAME_COL, QIcon(*routesInIcon));
          newSrcList->blockSignals(false);
        }
        newSrcList->blockSignals(true);
        //item = new QTreeWidgetItem(srcCatItem, QStringList() << in_r.displayName() << jackLabel );
        item = new RouteTreeWidgetItem(srcCatItem, 
                                       QStringList() << in_r.displayName(MusEGlobal::config.preferredRouteNameOrAlias), 
                                       RouteTreeWidgetItem::RouteItem, 
                                       true, 
                                       in_r);
        //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(in_r));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setTextAlignment(ROUTE_NAME_COL, align_flags);
        newSrcList->blockSignals(false);
      }
      in_rl.push_back(in_r);
    }

    dstCatItem = newDstList->findCategoryItem(jackCat);
    for(std::list<QString>::iterator i = tmpJackInPorts.begin(); i != tmpJackInPorts.end(); ++i)
    {
      const MusECore::Route out_r(*i, true, -1, MusECore::Route::JACK_ROUTE);
      item = newDstList->findItem(out_r, RouteTreeWidgetItem::RouteItem);
      if(item)
      {
        // Update the text.
        item->setText(ROUTE_NAME_COL, out_r.displayName(MusEGlobal::config.preferredRouteNameOrAlias));
      }
      else
      {
        if(!dstCatItem)
        {
          newDstList->blockSignals(true);
          //dstCatItem = new QTreeWidgetItem(newDstList, QStringList() << jackCat << QString() );
          dstCatItem = new RouteTreeWidgetItem(newDstList, QStringList() << jackCat, RouteTreeWidgetItem::CategoryItem, false);
          dstCatItem->setFlags(Qt::ItemIsEnabled);
          QFont fnt = dstCatItem->font(ROUTE_NAME_COL);
          fnt.setBold(true);
          fnt.setItalic(true);
          //fnt.setPointSize(fnt.pointSize() + 2);
          dstCatItem->setFont(ROUTE_NAME_COL, fnt);
          dstCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
          dstCatItem->setExpanded(true);
          dstCatItem->setIcon(ROUTE_NAME_COL, QIcon(*routesOutIcon));
          newDstList->blockSignals(false);
        }
        newDstList->blockSignals(true);
        //item = new QTreeWidgetItem(dstCatItem, QStringList() << out_r.displayName() << jackLabel );
        item = new RouteTreeWidgetItem(dstCatItem, 
                                       QStringList() << out_r.displayName(MusEGlobal::config.preferredRouteNameOrAlias), 
                                       RouteTreeWidgetItem::RouteItem, 
                                       false, 
                                       out_r);
        //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(out_r));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setTextAlignment(ROUTE_NAME_COL, align_flags);
        newDstList->blockSignals(false);
      }
      const QIcon src_ico(*routesInIcon);
      const QIcon dst_ico(*routesOutIcon);
      for(MusECore::ciRoute i = in_rl.begin(); i != in_rl.end(); ++i)
      {
        const MusECore::Route& in_r = *i;
        if(MusECore::routeCanDisconnect(in_r, out_r))
        {
          routesItem = findRoutesItem(in_r, out_r);
          if(routesItem)
          {
            // Update the text.
            routesItem->setText(ROUTE_SRC_COL, in_r.displayName(MusEGlobal::config.preferredRouteNameOrAlias));
            routesItem->setText(ROUTE_DST_COL, out_r.displayName(MusEGlobal::config.preferredRouteNameOrAlias));
          }
          else
          {
            routeList->blockSignals(true);
            routesItem = new QTreeWidgetItem(routeList, 
            QStringList() << in_r.displayName(MusEGlobal::config.preferredRouteNameOrAlias) <<
                             out_r.displayName(MusEGlobal::config.preferredRouteNameOrAlias));
            routesItem->setTextAlignment(ROUTE_SRC_COL, align_flags);
            routesItem->setTextAlignment(ROUTE_DST_COL, align_flags);
            routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(in_r));
            routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(out_r));
            routesItem->setIcon(ROUTE_SRC_COL, src_ico);
            routesItem->setIcon(ROUTE_DST_COL, dst_ico);
            routeList->blockSignals(false);
          }
        }
      }
    }
  
    //------------
    // Jack midi:
    //------------
    
    srcCatItem = newSrcList->findCategoryItem(jackMidiCat);
    in_rl.clear();
    for(std::list<QString>::iterator i = tmpJackMidiOutPorts.begin(); i != tmpJackMidiOutPorts.end(); ++i)
    {
      const MusECore::Route in_r(*i, false, -1, MusECore::Route::JACK_ROUTE);
      item = newSrcList->findItem(in_r, RouteTreeWidgetItem::RouteItem);
      if(item)
      {
        // Update the text.
        item->setText(ROUTE_NAME_COL, in_r.displayName(MusEGlobal::config.preferredRouteNameOrAlias));
      }
      else
      {
        if(!srcCatItem)
        {
          newSrcList->blockSignals(true);
          //srcCatItem = new QTreeWidgetItem(newSrcList, QStringList() << jackMidiCat << QString() );
          srcCatItem = new RouteTreeWidgetItem(newSrcList, QStringList() << jackMidiCat, RouteTreeWidgetItem::CategoryItem, true);
          srcCatItem->setFlags(Qt::ItemIsEnabled);
          QFont fnt = srcCatItem->font(ROUTE_NAME_COL);
          fnt.setBold(true);
          fnt.setItalic(true);
          //fnt.setPointSize(fnt.pointSize() + 2);
          srcCatItem->setFont(ROUTE_NAME_COL, fnt);
          srcCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
          srcCatItem->setExpanded(true);
          srcCatItem->setIcon(ROUTE_NAME_COL, QIcon(*routesMidiInIcon));
          newSrcList->blockSignals(false);
        }
        newSrcList->blockSignals(true);
        //item = new QTreeWidgetItem(srcCatItem, QStringList() << in_r.displayName() << jackMidiLabel );
        item = new RouteTreeWidgetItem(srcCatItem, 
                                       QStringList() << in_r.displayName(MusEGlobal::config.preferredRouteNameOrAlias), 
                                       RouteTreeWidgetItem::RouteItem, 
                                       true, 
                                       in_r);
        //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(in_r));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setTextAlignment(ROUTE_NAME_COL, align_flags);
        newSrcList->blockSignals(false);
      }
      in_rl.push_back(in_r);
    }
    
    dstCatItem = newDstList->findCategoryItem(jackMidiCat);
    for(std::list<QString>::iterator i = tmpJackMidiInPorts.begin(); i != tmpJackMidiInPorts.end(); ++i)
    {
      const MusECore::Route out_r(*i, true, -1, MusECore::Route::JACK_ROUTE);
      item = newDstList->findItem(out_r, RouteTreeWidgetItem::RouteItem);
      if(item)
      {
        // Update the text.
        item->setText(ROUTE_NAME_COL, out_r.displayName(MusEGlobal::config.preferredRouteNameOrAlias));
      }
      else
      {
        if(!dstCatItem)
        {
          newDstList->blockSignals(true);
          //dstCatItem = new QTreeWidgetItem(newDstList, QStringList() << jackMidiCat << QString() );
          dstCatItem = new RouteTreeWidgetItem(newDstList, QStringList() << jackMidiCat, RouteTreeWidgetItem::CategoryItem, false);
          dstCatItem->setFlags(Qt::ItemIsEnabled);
          QFont fnt = dstCatItem->font(ROUTE_NAME_COL);
          fnt.setBold(true);
          fnt.setItalic(true);
          //fnt.setPointSize(fnt.pointSize() + 2);
          dstCatItem->setFont(ROUTE_NAME_COL, fnt);
          dstCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
          dstCatItem->setExpanded(true);
          dstCatItem->setIcon(ROUTE_NAME_COL, QIcon(*routesMidiOutIcon));
          newDstList->blockSignals(false);
        }
        newDstList->blockSignals(true);
        //item = new QTreeWidgetItem(dstCatItem, QStringList() << out_r.displayName() << jackMidiLabel );
        item = new RouteTreeWidgetItem(dstCatItem, 
                                       QStringList() << out_r.displayName(MusEGlobal::config.preferredRouteNameOrAlias), 
                                       RouteTreeWidgetItem::RouteItem, 
                                       false, 
                                       out_r);
        //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(out_r));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setTextAlignment(ROUTE_NAME_COL, align_flags);
        newDstList->blockSignals(false);
      }
      const QIcon src_ico(*routesMidiInIcon);
      const QIcon dst_ico(*routesMidiOutIcon);
      for(MusECore::ciRoute i = in_rl.begin(); i != in_rl.end(); ++i)
      {
        const MusECore::Route& in_r = *i;
        if(MusECore::routeCanDisconnect(in_r, out_r))
        {
          routesItem = findRoutesItem(in_r, out_r);
          if(routesItem)
          {
            // Update the text.
            routesItem->setText(ROUTE_SRC_COL, in_r.displayName(MusEGlobal::config.preferredRouteNameOrAlias));
            routesItem->setText(ROUTE_DST_COL, out_r.displayName(MusEGlobal::config.preferredRouteNameOrAlias));
          }
          else
          {
            routeList->blockSignals(true);
            routesItem = new QTreeWidgetItem(routeList, 
              QStringList() << in_r.displayName(MusEGlobal::config.preferredRouteNameOrAlias) <<
                               out_r.displayName(MusEGlobal::config.preferredRouteNameOrAlias));
            routesItem->setTextAlignment(ROUTE_SRC_COL, align_flags);
            routesItem->setTextAlignment(ROUTE_DST_COL, align_flags);
            routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(in_r));
            routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(out_r));
            routesItem->setIcon(ROUTE_SRC_COL, src_ico);
            routesItem->setIcon(ROUTE_DST_COL, dst_ico);
            routeList->blockSignals(false);
          }
        }
      }
    }
  }
}

void MusE::startRouteDialog()
{
  if(routeDialog == nullptr)
    // NOTE: For deleting parentless dialogs and widgets, please add them to MusE::deleteParentlessDialogs().
    routeDialog = new MusEGui::RouteDialog;
  routeDialog->show();
  routeDialog->raise();
}


} // namespace MusEGui
