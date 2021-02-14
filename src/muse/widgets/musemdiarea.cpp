#include <QKeyEvent>
#include "musemdiarea.h"

MuseMdiArea::MuseMdiArea(QWidget *parent)
    : QMdiArea(parent)
{

}

void MuseMdiArea::keyPressEvent(QKeyEvent *event)
{
    event->ignore();
}
