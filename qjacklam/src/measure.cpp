/***************************************************************************
 *   Copyright (C) 2005 by Karsten Wiese                                   *
 *   annabellesgarden@yahoo.de                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <iostream>
#include <string.h>
#include <math.h>
#include <qapplication.h>
#include "MainWindow.h"
#include "jack.h"

extern MainWindow *PW;

static const float PulsValue = 0.1;

float Pulse[500];

static float LastValue;


static int Latency;
static int PulseForRate;
static int PulseLen;

static void initPulse(int sampleRate)
{
  PulseLen = (sampleRate / 1500) * 2;
  int s;
  Pulse[0] = Pulse[PulseLen / 2] = 0.0;
  for (s = 1; s < PulseLen / 2; s++) {
    float V;
    double r;

    r = (2.0 * M_PI) / PulseLen  * s;
    V = sin(r) * (1.0 + cos(r - M_PI)) / 2;

    Pulse[s] = V;
    Pulse[PulseLen - s] = -V;
  }

//   for (s = 0; s < PulseLen; s++) {
// //     float V;
// //     double r;

// //     r = (2 * M_PI) / PulseLen  * s;
// //     V = sin(r) * (1 + cos(r - M_PI)) / 2;

// //     Pulse[s] = V;
//     std::cout << s << " " << Pulse[s] << std::endl;
//   }


  PulseForRate = sampleRate;
}


// void MeasureLatency(long numSampsToProcess, int sampleRate, float *indata, float *outdata)
// {
//   static bool Triggered;
//   static int MinSet, MaxSet;
//   static float Min, Max;

//   if (Triggered) {
//     int s;
//     for (s = 0; s < numSampsToProcess; s++) {
//       float Value = indata[s];
//       if (Value < Min) {
// 	Min = Value;
// 	MinSet = Samples + s;
//       } else
// 	if (Value > Max) {
// 	  Max = Value;
// 	  MaxSet = Samples + s;
// 	} else
// 	  if (MinSet && MaxSet) {
// 	    Latency = (MinSet + MaxSet) / 2;
// 	    QCustomEvent * E =  new QCustomEvent(QCustomEvent::User + eeLatencyValue);
// 	    E->setData((void*)Latency);
// 	    QApplication::postEvent(PW, E);

// 	    //	    std::cout << Latency << std::endl;
// 	    Triggered = false;
// 	    break;
// 	  }



// //       if (LastValue < -PulsValue / 4  &&  Value > PulsValue / 4) {
// // 	Latency = Samples + s;
// // 	std::cout << Latency << std::endl;
// // 	Triggered = false;
// //       }
// //       LastValue = Value;
//     }
//   }

//   if (PulseForRate != sampleRate)
//     initPulse(sampleRate);

//   Samples += numSampsToProcess;
//   static int PulseAt;
//   int outSamplesSet = 0;
//   if (PulseAt) {
//       outSamplesSet = PulseLen - PulseAt;
//       if (outSamplesSet > numSampsToProcess) {
// 	outSamplesSet = numSampsToProcess;
//       }
//       memcpy(outdata, Pulse + PulseAt, outSamplesSet * sizeof(Pulse[0]));
//       if ((PulseAt += outSamplesSet) >= PulseLen)
// 	PulseAt = 0;
//   } else
//     if (Samples >= sampleRate) {
//       outSamplesSet = PulseLen;
//       if (outSamplesSet > numSampsToProcess) {
// 	PulseAt = outSamplesSet = numSampsToProcess;
//       }
//       memcpy(outdata, Pulse, outSamplesSet * sizeof(Pulse[0]));
//       Samples = numSampsToProcess - PulseLen / 2;
//       LastValue = 0.0;
//       if (Triggered) {
// 	QCustomEvent * E =  new QCustomEvent(QCustomEvent::User + eeLatencyValueUnavailable);
// 	QApplication::postEvent(PW, E);
//       }
//       Triggered = true;
//       Min = -0.01;
//       Max = 0.01;
//       MinSet = MaxSet = 0;
//     }
//   memset(outdata + outSamplesSet, 0, sizeof(*outdata) * (numSampsToProcess - outSamplesSet));
// }


class LatencyMeter {
  bool Triggered;
  jack_nframes_t MinSet, MaxSet;
  float Min, Max;
  int PulseAt;
  int Samples;
  jack_nframes_t SendTime;

  static int ProcessCallback1Client(jack_nframes_t nframes, void *arg)
  {
    LatencyMeter *info = (LatencyMeter*) arg;
    info->receive(nframes, thread_info_o->GetSampleRate(), thread_info_i->getBuffer(nframes, false));
    info->send(nframes, thread_info_o->GetSampleRate(), thread_info_o->getBuffer(nframes, true));
    return 0;
  }
  static int ProcessCallbackSend(jack_nframes_t nframes, void *arg)
  {
    LatencyMeter *info = (LatencyMeter*) arg;
    info->send(nframes, thread_info_o->GetSampleRate(), thread_info_o->getBuffer(nframes, true));
    return 0;
  }
  static int ProcessCallbackReceive(jack_nframes_t nframes, void *arg)
  {
    LatencyMeter *info = (LatencyMeter*) arg;
    info->receive(nframes, thread_info_o->GetSampleRate(), thread_info_i->getBuffer(nframes, false));
    return 0;
  }

public:
  void receive(long numSampsToProcess, int sampleRate __attribute__ ((unused)), float *indata)
  {
    if (Triggered) {
      jack_nframes_t T = thread_info_i->LastFrameTime();
      int s;
      for (s = 0; s < numSampsToProcess; s++) {
	float Value = indata[s];
	if (Value < Min) {
	  Min = Value;
	  MinSet = T + s;
	} else
	  if (Value > Max) {
	    Max = Value;
	    MaxSet = T + s;
	  } else
	    if (MinSet && MaxSet) {
	      Latency = (MinSet + MaxSet) / 2 - SendTime;
	      QCustomEvent * E =  new QCustomEvent(QCustomEvent::User + eeLatencyValue);
	      E->setData((void*)Latency);
	      QApplication::postEvent(PW, E);

	    //	    std::cout << Latency << std::endl;
	      Triggered = false;
	      break;
	    }
	
	
	
//       if (LastValue < -PulsValue / 4  &&  Value > PulsValue / 4) {
// 	Latency = Samples + s;
// 	std::cout << Latency << std::endl;
// 	Triggered = false;
//       }
//       LastValue = Value;
      }
    }
  }

  void send(long numSampsToProcess, int sampleRate, float *outdata)
  {
    if (PulseForRate != sampleRate)
      initPulse(sampleRate);
  
    Samples += numSampsToProcess;
    int outSamplesSet = 0;
    if (PulseAt) {
      outSamplesSet = PulseLen - PulseAt;
      if (outSamplesSet > numSampsToProcess) {
	outSamplesSet = numSampsToProcess;
      }
      memcpy(outdata, Pulse + PulseAt, outSamplesSet * sizeof(Pulse[0]));
      if ((PulseAt += outSamplesSet) >= PulseLen)
	PulseAt = 0;
    } else
      if (Samples >= sampleRate) {
	outSamplesSet = PulseLen;
	if (outSamplesSet > numSampsToProcess) {
	  PulseAt = outSamplesSet = numSampsToProcess;
	}
	memcpy(outdata, Pulse, outSamplesSet * sizeof(Pulse[0]));
	Samples = 0;
	SendTime = thread_info_o->LastFrameTime() + PulseLen / 2;
	LastValue = 0.0;
	if (Triggered) {
	  QCustomEvent * E =  new QCustomEvent(QCustomEvent::User + eeLatencyValueUnavailable);
	  QApplication::postEvent(PW, E);
	}
	Triggered = true;
	Min = -0.01;
	Max = 0.01;
	MinSet = MaxSet = 0;
      }
    memset(outdata + outSamplesSet, 0, sizeof(*outdata) * (numSampsToProcess - outSamplesSet));
  }

  int setup1Client() {
    thread_info_o = thread_info_i = new jackClient(1, 1);
    return thread_info_o->jackUp("lam", ProcessCallback1Client, this);
  }

  int setup2Clients() {
    thread_info_o = new jackClient(0, 1);
    thread_info_i = new jackClient(1, 0);
    return thread_info_o->jackUp("lamS", ProcessCallbackSend, this) |
      thread_info_i->jackUp("lamR", ProcessCallbackReceive, this);
  }
};

static LatencyMeter LAM;


// int jackUp()
// {
//   return LAM.setup1Client();
// }

// void jackDown()
// {
//   thread_info_o->jackDown();
// }

int jackUp()
{
  return LAM.setup2Clients();
}

void jackDown()
{
  thread_info_i->jackDown();
  thread_info_o->jackDown();
}
