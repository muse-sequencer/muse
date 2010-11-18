//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mittranspose.h,v 1.1.1.1 2003/10/27 18:52:40 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MITTRANSPOSE_H__
#define __MITTRANSPOSE_H__

#include "mitplugin.h"
#include "ui_mittransposebase.h"
#include <list>

class QCloseEvent;

struct KeyOn {
      unsigned char pitch;
      unsigned char channel;
      unsigned char port;
      char transpose;
      KeyOn(int a, int b, int c, int d) {
            pitch = a;
            transpose = d;
            channel = b;
            port = c;
            }
      };

typedef std::list<KeyOn > KeyOnList;
typedef KeyOnList::iterator iKeyOn;

class Xml;

//---------------------------------------------------------
//   MITTransposeBaseWidget
//   Wrapper around Ui::MITTransposeBase
//---------------------------------------------------------

class MITTransposeBaseWidget : public QWidget, public Ui::MITTransposeBase
{
      Q_OBJECT

   public:
      MITTransposeBaseWidget(QWidget *parent = 0, Qt::WFlags f = 0) : QWidget(parent, f) { setupUi(this); }
};

//---------------------------------------------------------
//   MITPluginTranspose
//---------------------------------------------------------

class MITPluginTranspose : public MITTransposeBaseWidget, public MITPlugin {
      Q_OBJECT

      KeyOnList keyOnList;
      int transpose;    // current pitch offset
      int trigger;
      bool on;
      bool transposeChangedFlag;

      void transposeChanged();
      virtual void closeEvent(QCloseEvent*);

   signals:
      void hideWindow();

   private slots:
      void onToggled(bool);
      void triggerKeyChanged(int);
      void noteReceived();

   public:
      MITPluginTranspose(QWidget* parent = 0, Qt::WFlags fl = 0);
      virtual void process(MEvent&);
      virtual void readStatus(Xml&);
      virtual void writeStatus(int, Xml&) const;
      };

extern MITPluginTranspose* mitPluginTranspose;

#endif

