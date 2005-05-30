
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
