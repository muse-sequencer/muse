#ifndef POSTOOLBAR_H
#define POSTOOLBAR_H

#include <QToolBar>
#include <QSlider>

#include "pos.h"
#include "posedit.h"
#include "type_defs.h"

namespace MusEGui {

class PosToolbar : public QToolBar
{
    Q_OBJECT

public:
    PosToolbar(const QString& title, QWidget* parent = nullptr);

public slots:
   void setPos(int, unsigned, bool);
   void songChanged(MusECore::SongChangedStruct_t);

private:
   PosEdit *markerLeft;
   PosEdit *markerRight;
   PosEdit *time;
   PosEdit *timeSmpte;
   QSlider *slider;

private slots:
    void cposChanged(const MusECore::Pos&);
    void cposChanged(int);
    void lposChanged(const MusECore::Pos&);
    void rposChanged(const MusECore::Pos&);

};

}

#endif // POSTOOLBAR_H
