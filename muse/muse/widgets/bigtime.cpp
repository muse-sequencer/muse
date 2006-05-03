#include "al/al.h"
#include "globals.h"
#include "bigtime.h"
#include "song.h"
// #include "app.h"
#include "gconfig.h"
#include "al/sig.h"
#include "al/tempo.h"

//
// the bigtime widget
// display is split into several parts to avoid flickering.
//

//---------------------------------------------------------
//   BigTime
//---------------------------------------------------------

BigTime::BigTime(QWidget* parent)
   : QWidget(parent)
      {
	setWindowFlags(Qt::WindowStaysOnTopHint);

      barLabel   = new QLabel(this);
      beatLabel  = new QLabel(this);
      tickLabel  = new QLabel(this);
      hourLabel  = new QLabel(this);
      minLabel   = new QLabel(this);
      secLabel   = new QLabel(this);
      frameLabel = new QLabel(this);
      sep1       = new QLabel(QString("."), this);
      sep2       = new QLabel(QString("."), this);
      sep3       = new QLabel(QString(":"), this);
      sep4       = new QLabel(QString(":"), this);
      sep5       = new QLabel(QString(":"), this);

      oldbar = oldbeat = oldtick = oldhour = oldmin = oldsec = oldframe = -1;

      setString(song->cPos());

      configChanged();

      QFont f(QString("Courier"));
      f.setPixelSize(10);
      setFont(f);
      setWindowTitle(tr("MusE: Bigtime"));
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void BigTime::configChanged()
      {
      setBgColor(config.bigTimeBackgroundColor);
      setFgColor(config.bigTimeForegroundColor);
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void BigTime::closeEvent(QCloseEvent *ev)
      {
      emit closed();
      QWidget::closeEvent(ev);
      }

//---------------------------------------------------------
//   setString
//---------------------------------------------------------

bool BigTime::setString(AL::Pos pos)
      {
      int bar, beat, tick;
      pos.mbt(&bar, &beat, &tick);
      int min, sec, hour, frame, subframe;
      pos.msf(&min, &sec, &frame, &subframe);

      hour = min / 60;
      min  %= 60;

      QString s;

      if(oldbar != bar) {
	      s.sprintf("%04d", bar+1);
	      barLabel->setText(s);
	      oldbar = bar;
      }

      if(oldbeat != beat) {
	      s.sprintf("%02d", beat+1);
	      beatLabel->setText(s);
	      oldbeat = beat;
      }

      if(oldtick != tick) {
	      s.sprintf("%03d", tick);
	      tickLabel->setText(s);
	      oldtick = tick;
      }

      if(oldhour != hour) {
	      s.sprintf("%02d", hour);
	      hourLabel->setText(s);
	      oldhour = hour;
      }

      if(oldmin != min) {
	      s.sprintf("%02d", min);
	      minLabel->setText(s);
	      oldmin = min;
      }

      if(oldsec != sec) {
	      s.sprintf("%02d", sec);
	      secLabel->setText(s);
	      oldsec = sec;
      }

      if(oldframe != frame) {
	      s.sprintf("%02d", frame);
	      frameLabel->setText(s);
	      oldframe = frame;
      }

      return false;
      }

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void BigTime::setPos(int idx, AL::Pos pos, bool)
      {
      if (idx == 0)
            setString(pos);
      }

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void BigTime::resizeEvent(QResizeEvent *ev)
      {
	QFont f    = font();
      QFontMetrics fm(f);
	int fs     = f.pixelSize();
	int hspace = 20;
	int tw     = fm.width(QString("00:00:00:00"));
	fs         = ((ev->size().width() - hspace*2)*fs) / tw;

// printf("resize BigTime %d -> %d, w %d\n", fs, nfs, ev->size().width());

	// set min/max
	if (fs < 10)
            fs = 10;
	else if (fs > 256)
            fs = 256;
	f.setPixelSize(fs);
	setFont(f);

	int digitWidth = fontMetrics().width(QString("0"));
	
	int vspace = (ev->size().height() - (fs*2)) / 3;
	int tickY = vspace;
	int timeY = vspace*2 + fs;

	barLabel->resize(digitWidth*4, fs);
	beatLabel->resize(digitWidth*2, fs);
	tickLabel->resize(digitWidth*3, fs);
	hourLabel->resize(digitWidth*2, fs);
	minLabel->resize(digitWidth*2, fs);
	secLabel->resize(digitWidth*2, fs);
	frameLabel->resize(digitWidth*2, fs);

	sep1->resize(digitWidth, fs);
	sep2->resize(digitWidth, fs);
	sep3->resize(digitWidth, fs);
	sep4->resize(digitWidth, fs);
	sep5->resize(digitWidth, fs);
	
	barLabel->move(		hspace + (digitWidth*0), tickY);
	sep1->move(		hspace + (digitWidth*4), tickY);
	beatLabel->move(	hspace + (digitWidth*5), tickY);
	sep2->move(		hspace + (digitWidth*7), tickY);
	tickLabel->move(	hspace + (digitWidth*8), tickY);

	hourLabel->move(	hspace + (digitWidth*0), timeY);
	sep3->move(		hspace + (digitWidth*2), timeY);
	minLabel->move(		hspace + (digitWidth*3), timeY);
	sep4->move(		hspace + (digitWidth*5), timeY);
	secLabel->move(		hspace + (digitWidth*6), timeY);
	sep5->move(		hspace + (digitWidth*8), timeY);
	frameLabel->move(	hspace + (digitWidth*9), timeY);
      }

//---------------------------------------------------------
//   setForegroundColor
//---------------------------------------------------------

void BigTime::setFgColor(QColor c)
      {
        QPalette cg = palette();
        cg.setColor(QPalette::Foreground, c);
        setPalette(cg);

        barLabel->setPalette(cg);
        beatLabel->setPalette(cg);
        tickLabel->setPalette(cg);
        hourLabel->setPalette(cg);
        minLabel->setPalette(cg);
        secLabel->setPalette(cg);
        frameLabel->setPalette(cg);

        sep1->setPalette(cg);
        sep2->setPalette(cg);
        sep3->setPalette(cg);
        sep4->setPalette(cg);
        sep5->setPalette(cg);
      }

//---------------------------------------------------------
//   setBackgroundColor
//---------------------------------------------------------

void BigTime::setBgColor(QColor c)
      {
        QPalette cg = palette();
        cg.setColor(QPalette::Background, c);
        setPalette(cg);

        barLabel->setPalette(cg);
        beatLabel->setPalette(cg);
        tickLabel->setPalette(cg);
        hourLabel->setPalette(cg);
        minLabel->setPalette(cg);
        secLabel->setPalette(cg);
        frameLabel->setPalette(cg);

        sep1->setPalette(cg);
        sep2->setPalette(cg);
        sep3->setPalette(cg);
        sep4->setPalette(cg);
        sep5->setPalette(cg);
      }

