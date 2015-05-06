#include "wavepreview.h"
#include "globals.h"
#include <QPushButton>

namespace MusEGlobal
{
MusECore::WavePreview *wavePreview;
};

namespace MusECore
{

WavePreview::WavePreview():
   sf(0),
   isPlaying(false)
{
   tmpbuffer = new float [MusEGlobal::segmentSize * 2];
}

WavePreview::~WavePreview()
{
   stop();
}

void WavePreview::play(QString path)
{
   stop();
   memset(&sfi, 0, sizeof(sfi));
   sf = sf_open(path.toUtf8().data(), SFM_READ, &sfi);
   if(sf)
   {
      isPlaying = true;

   }

}

void WavePreview::stop()
{
   isPlaying = false;
   if(sf)
   {
      sf_close(sf);
      sf = 0;
   }
}

void WavePreview::addData(int channels, int nframes, float *buffer[])
{
   if(sf && isPlaying)
   {
      sf_count_t nread = sf_readf_float(sf, tmpbuffer, nframes);
      if(nread <= 0)
      {
         isPlaying = false;
         return;
      }
      int chns = std::min(channels, sfi.channels);
      for(int i = 0; i < chns; i++)
      {
         for(int k = 0; k < nread; k++)
         {
            buffer [i] [k] += tmpbuffer [(k + i)*sfi.channels];
         }
      }
      for(int i = chns; i < channels; i++)
      {
         for(int k = 0; k < nread; k++)
         {
            buffer [i] [k] += tmpbuffer [(k + i)*sfi.channels];
         }
      }
   }
}

void initWavePreview()
{
   MusEGlobal::wavePreview = new WavePreview();
}

void exitWavePreview()
{
   if(MusEGlobal::wavePreview)
   {
      delete MusEGlobal::wavePreview;
   }
}

void AudioPreviewDialog::urlChanged(const QString &str)
{
   QFileInfo fi(str);
   if(fi.isDir()){
      return;
   }
   MusEGlobal::wavePreview->play(str);
}

void AudioPreviewDialog::stopWave()
{
   MusEGlobal::wavePreview->stop();
}

AudioPreviewDialog::AudioPreviewDialog(QWidget *parent)
   :QFileDialog(parent)
{
    setOption(QFileDialog::DontUseNativeDialog);
    setNameFilter("*.mid *.midi");
    cb = new QComboBox;
    cb->setEditable(false);
    //cb->addItems(list_ports());
    cb->setCurrentIndex(cb->count() - 1);

    QPushButton *btnStop = new QPushButton("Stop");
    connect(btnStop, SIGNAL(clicked()), this, SLOT(stopWave()));


    //QListView *v = this->findChild<QListView *>("listView", Qt::FindChildrenRecursively);
    QObject::connect(this, SIGNAL(currentChanged(const QString&)), this, SLOT(urlChanged(const QString&)));
    //this->layout()->addWidget(new QLabel("Midi device: "));
    //this->layout()->addWidget(cb);
    //this->layout()->addWidget(btnStop);
}

AudioPreviewDialog::~AudioPreviewDialog()
{
   MusEGlobal::wavePreview->stop();
}

};
