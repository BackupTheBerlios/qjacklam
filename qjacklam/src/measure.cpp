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
#include "userevent.h"

extern MainWindow *PW;

Measurement::Measurement(int _Points)
  :E(NULL)
    {
      Points = _Points;
      Point = (sPoints *)calloc(Points, sizeof(Point[0]));
    }

void Measurement::Return()
{
  //  std::cout << __PRETTY_FUNCTION__ << " " << __LINE__ << " " << SendTime << std::endl;
  E->setData(this);
  QApplication::postEvent(PW, E);
  E = NULL;
}

static const float PulsValue = 0.1;

float Pulse[500];



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

class LatencyMeter {
  int PulseAt;
  Measurement *M;
  int Send;
//   static int ProcessCallback1Client(jack_nframes_t nframes, void *arg)
//   {
//     LatencyMeter *info = (LatencyMeter*) arg;
//     info->receive(nframes, Sender->GetSampleRate(), Receiver->getBuffer(nframes, false));
//     info->send(nframes, Sender->GetSampleRate(), Sender->getBuffer(nframes, true));
//     return 0;
//   }

  static int ProcessCallbackSend(jack_nframes_t nframes, void *arg)
  {
    //    std::cout << __PRETTY_FUNCTION__ << nframes << std::endl;
    LatencyMeter *info = (LatencyMeter*) arg;
    float *outdata = Sender->getBuffer(nframes, true, 0);
    if (outdata)
      info->send(nframes, Sender->GetSampleRate(), outdata);
    return 0;
  }

  static int ProcessCallbackReceive(jack_nframes_t nframes, void *arg)
  {
    LatencyMeter *lam = (LatencyMeter*) arg;
    Measurement *M = lam->M;
    if (M && !lam->Send) {
      int i;
      float *indata;
      bool Complete = true;
      jack_nframes_t T = Receiver->LastFrameTime();

      for (i = 0; indata = Receiver->getBuffer(nframes, false, i); ++i) {
	if (M->Points > i) {
	  if (M->Point[i].receive(nframes, T, indata)) {
	    Complete = false;
	  }
	}
      }
      if (Complete || T + PulseLen / 2 - M->SendTime >= Sender->GetSampleRate()) {
// 	std::cout << __PRETTY_FUNCTION__ << " " << __LINE__ << " " << Complete << " " << T << " "<< M->SendTime << std::endl;
	lam->M = NULL;
	M->Return();
      }
    }
    return 0;
  }

public:

  void send(long numSampsToProcess, int sampleRate, float *outdata)
  {
    int outSamplesSet = 0;
    if (PulseForRate != sampleRate)
      initPulse(sampleRate);

    if (M) {
      if (PulseAt) {
	outSamplesSet = PulseLen - PulseAt;
	if (outSamplesSet > numSampsToProcess) {
	  outSamplesSet = numSampsToProcess;
	}
	memcpy(outdata, Pulse + PulseAt, outSamplesSet * sizeof(Pulse[0]));
	if ((PulseAt += outSamplesSet) >= PulseLen)
	  PulseAt = 0;
      } else
	if (Send && Send-- <= 1) {
	  Send = 0;
	  outSamplesSet = PulseLen;
	  if (outSamplesSet > numSampsToProcess) {
	    PulseAt = outSamplesSet = numSampsToProcess;
	  }
	  memcpy(outdata, Pulse, outSamplesSet * sizeof(Pulse[0]));
	  M->SendTime = Sender->LastFrameTime() + PulseLen / 2;
	  int i;
	  for (i = 0; i < M->Points; i++)
	    M->Point[i].Trigger();
	}
    }
    memset(outdata + outSamplesSet, 0, sizeof(*outdata) * (numSampsToProcess - outSamplesSet));
  }

//   int setup1Client() {
//     thread_info_o = thread_info_i = new jackClient(1, 1);
//     return thread_info_o->jackUp("lam", ProcessCallback1Client, this);
//   }

  int setup2Clients() {
    Sender = new jackClient();
    Receiver = new jackClient();
    return Sender->jackUp("lamS", ProcessCallbackSend, this, true) |
      Receiver->jackUp("lamR", ProcessCallbackReceive, this);
  }
  LatencyMeter()
    :PulseAt(0)
    ,M(NULL)
    ,Send(0)
  {}

  void SetM(Measurement *_M) {
    PulseAt = 0;
    Send = 2;
    M = _M;
  }
};

static LatencyMeter LAM;

void Measurement::Arm(int _Row)
{
  Row = _Row;
  E = new QCustomEvent(QCustomEvent::User + eeLatencyValue);
  LAM.SetM(this);
}

bool Measurement::sPoints::receive(long numSampsToProcess, jack_nframes_t T, float *indata)
{
  if (Triggered) {
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
	    /* 		Latency = (MinSet + MaxSet) / 2 - SendTime; */
	    /* 		QCustomEvent * E =  new QCustomEvent(QCustomEvent::User + eeLatencyValue); */
	    /* 		E->setData((void*)Latency); */
	    /* 		QApplication::postEvent(PW, E); */

	    //	    std::cout << Latency << std::endl;
	    //	    std::cout << __LINE__ << __FUNCTION__ << std::endl;
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
  //  std::cout <<  Triggered << __LINE__ << __FUNCTION__ << std::endl;
  return Triggered;
}

void Measurement::sPoints::Trigger()
{
  Triggered = true;
  //  std::cout <<  Triggered << __LINE__ << __FUNCTION__ << std::endl;
  Min = -0.01;
  Max = 0.01;
  MinSet = MaxSet = 0;
}
void Measurement::Dump()
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
  int i;
  for (i = 0; i < Points; i++) {
    sPoints &P = Point[i];
    std::cout << P.Triggered;
    if (!P.Triggered)
      std::cout << "\t" << (P.MinSet + P.MaxSet) / 2 - SendTime;
    std::cout << std::endl;
  }
    
}
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
  Receiver->jackDown();
  Sender->jackDown();
}
