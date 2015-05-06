#ifndef WAVEPREVIEW_H
#define WAVEPREVIEW_H

#include <stdio.h>
#include <sndfile.h>
#include <QString>
#include <QFileDialog>
#include <QComboBox>

namespace MusECore
{

class WavePreview
{
private:
   SNDFILE *sf;
   SF_INFO sfi;
   bool isPlaying;
   float *tmpbuffer;
public:
   WavePreview();
   virtual ~WavePreview();
   void play(QString path);
   void stop();
   void addData(int channels, int nframes, float *buffer []);

};

class AudioPreviewDialog : public QFileDialog{
   Q_OBJECT
private:
    QComboBox *cb;
private slots:
    void urlChanged(const QString &str);
    void stopWave();
public:
   AudioPreviewDialog(QWidget *parent);
   ~AudioPreviewDialog();
};


extern void initWavePreview();
extern void exitWavePreview();

};



namespace MusEGlobal
{
extern MusECore::WavePreview *wavePreview;
};

#endif
