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

class Measurement {
  int Row;
  class QCustomEvent *E;
  void Return();

  friend class LatencyMeter;
  jack_nframes_t SendTime;
  jack_nframes_t TimeOut;

  struct sPoints {
    bool Triggered;
    jack_nframes_t MinSet, MaxSet;
    float Min, Max;

    void Trigger();

    bool receive(long numSampsToProcess, jack_nframes_t T, float *indata);
  } *Point;
  int Points;

 public:
/*   void GetTimeOut() { */
/*     return TimeOut; */
/*   } */
  void SetTimeOut(int to) {
    TimeOut = to;
  }
  bool Measuring() {
    return E;
  }
  int GetRow() {
    return Row;
  }
  Measurement(int _Points, int to);
  void Arm(int _Row);
  ~Measurement()
    {
      free(Point);
    }
  void Dump();
  int Frames(int point) {
    if (point >= Points)
      return -1;
    sPoints &P(Point[point]);
    if (P.Triggered)
      return -1;
    return (P.MinSet + P.MaxSet) / 2 - SendTime;
  }
};
