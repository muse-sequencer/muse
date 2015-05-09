#include "wavepreview.h"
#include "globals.h"
#include <QPushButton>
#include <QLayout>


namespace MusEGlobal
{
MusECore::WavePreview *wavePreview;
}

namespace MusECore
{

WavePreview::WavePreview():
   sf(0),
   src(0),
   isPlaying(false),
   sem(1)
{
   segSize = MusEGlobal::segmentSize * 10;
   tmpbuffer = new float [segSize];
   srcbuffer = new float [segSize];
}

WavePreview::~WavePreview()
{
   stop();
   delete tmpbuffer;
   delete srcbuffer;
}

long WavePreview::static_srcCallback (void *cb_data, float **data)
{
   MusECore::WavePreview *wp = (MusECore::WavePreview *)cb_data;
   wp->nread = sf_readf_float(wp->sf, wp->tmpbuffer, 1);
   *data = wp->tmpbuffer;
   return wp->nread;
}

void WavePreview::play(QString path)
{
   stop();
   memset(&sfi, 0, sizeof(sfi));   
   sf = sf_open(path.toUtf8().data(), SFM_READ, &sfi);
   if(sf)
   {
      int err = 0;
      //src = src_new(SRC_SINC_BEST_QUALITY, sfi.channels, &err);
      src = src_callback_new(static_srcCallback, SRC_SINC_MEDIUM_QUALITY, sfi.channels, &err, this);
      if(src)
      {
         p1 = tmpbuffer;
         p2 = srcbuffer;      
         f1 = 0;
         f2 = 0;
         nread = 0;
         sd.src_ratio = ((double)MusEGlobal::sampleRate) / (double)sfi.samplerate;
         isPlaying = true;      
      }
      else
      {
         sf_close(sf);
         sf = 0;
      }

   }

}

void WavePreview::stop()
{
   isPlaying = false;
   sem.acquire();
   if(sf)
   {
      sf_close(sf);
      sf = 0;
   }
   if(src)
   {
      src_delete(src);
      src = 0;           
   }
   sem.release();
}

void WavePreview::addData(int channels, int nframes, float *buffer[])
{   
   if(sf && isPlaying)
   {     
      sem.acquire();
      memset(srcbuffer, 0, sizeof(segSize) * sizeof(float));
      /*p2 = srcbuffer;
      f2 = 0; 
      
      while(true)
      {         
         if(nread <= 0)
         {
            f1 = 0;
            p1 = tmpbuffer;
            nread = sf_readf_float(sf, tmpbuffer, nframes);

            if(nread <= 0)
            {
               isPlaying = false;
               return;
            }
         }
         sd.data_in = p1;
         sd.data_out = p2;
         sd.end_of_input = (nread == nframes) ? false : true;
         sd.input_frames = nread;
         sd.output_frames = nframes - f2;
         sd.input_frames_used = sd.output_frames_gen = 0;
         int err = src_process(src, &sd);
         if(err != 0)
         {
            break;
         }
         p1 += sd.input_frames_used * sfi.channels;
         p2 += sd.output_frames_gen * sfi.channels;
         f1 += sd.input_frames_used;
         f2 += sd.output_frames_gen;
         nread -= sd.input_frames_used;
         if((f2 >= nframes) || sd.end_of_input)
         {
            break;
         }         
      }*/
      
      int rd = src_callback_read(src, sd.src_ratio, nframes, srcbuffer);
      if(rd < nframes)
      {
         isPlaying = false;
      }

      if(rd == 0)
      {
         return;
      }
      
      int chns = std::min(channels, sfi.channels);
      for(int i = 0; i < chns; i++)
      {
         for(int k = 0; k < nframes; k++)
         {
            buffer [i] [k] += srcbuffer [(k + i)*sfi.channels];
         }
      }
      for(int i = chns; i < channels; i++)
      {
         for(int k = 0; k < nframes; k++)
         {
            buffer [i] [k] += srcbuffer [(k + i)*sfi.channels];
         }
      }
      sem.release();
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
   if(chAutoPlay->isChecked())
   {
      MusEGlobal::wavePreview->play(str);
   }
}

void AudioPreviewDialog::stopWave()
{
   MusEGlobal::wavePreview->stop();
}

AudioPreviewDialog::AudioPreviewDialog(QWidget *parent)
   :QFileDialog(parent)
{
    setOption(QFileDialog::DontUseNativeDialog);
    setNameFilter(QString("Samples *.wav *.ogg *.flac (*.wav *.WAV *.ogg *.flac);;All files (*)"));
    //cb = new QComboBox;
    //cb->setEditable(false);
    //cb->addItems(list_ports());
    //cb->setCurrentIndex(cb->count() - 1);

    chAutoPlay = new QCheckBox(this);
    chAutoPlay->setText(tr("Auto play"));


    QPushButton *btnStop = new QPushButton(tr("Stop"));
    connect(btnStop, SIGNAL(clicked()), this, SLOT(stopWave()));


    //QListView *v = this->findChild<QListView *>("listView", Qt::FindChildrenRecursively);
    QObject::connect(this, SIGNAL(currentChanged(const QString&)), this, SLOT(urlChanged(const QString&)));
    //this->layout()->addWidget(new QLabel("Midi device: "));
    //this->layout()->addWidget(cb);
    this->layout()->addWidget(chAutoPlay);
    this->layout()->addWidget(btnStop);

}

AudioPreviewDialog::~AudioPreviewDialog()
{
   MusEGlobal::wavePreview->stop();
}

}
