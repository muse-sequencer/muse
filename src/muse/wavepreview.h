#ifndef WAVEPREVIEW_H
#define WAVEPREVIEW_H

#include <stdio.h>
#include <sndfile.h>
#include <samplerate.h>
#include <QString>
#include <QFileDialog>
#include <QComboBox>
#include <QSemaphore>
#include <QCheckBox>
#include <QPushButton>

namespace MusECore
{

class WavePreview
{
private:
   SNDFILE *sf;
   SF_INFO sfi;
   SRC_STATE *src;
   bool isPlaying;
   float *tmpbuffer;
   float *srcbuffer;
   int segSize;
   float *p1;
   float *p2;      
   int f1;
   int f2;
   SRC_DATA sd;
   sf_count_t nread;
   QSemaphore sem;
   static long static_srcCallback (void *cb_data, float **data);
public:
   WavePreview(int segmentSize);
   virtual ~WavePreview();
   void play(QString path, int systemSampleRate);
   void stop();
   void addData(int channels, int nframes, float *buffer []);
   bool getIsPlaying() { return isPlaying; }

};

class AudioPreviewDialog : public QFileDialog{
   Q_OBJECT
private:
    QCheckBox *chAutoPlay;
    QPushButton *btnStop;
    bool lastIsPlaying;
    int _systemSampleRate;
private slots:
    void urlChanged(const QString &str);
    void startStopWave();
public slots:
    virtual int exec();
public:
   AudioPreviewDialog(QWidget *parent, int systemSampleRate);
   ~AudioPreviewDialog();
   void timerEvent(QTimerEvent *);
};


extern void initWavePreview(int segmentSize);
extern void exitWavePreview();

}



namespace MusEGlobal
{
extern MusECore::WavePreview *wavePreview;
}

#endif
