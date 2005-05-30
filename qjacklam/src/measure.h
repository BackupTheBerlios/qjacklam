
class Measurement {
  int Row;
  class QCustomEvent *E;
  void Return();

  friend class LatencyMeter;
  jack_nframes_t SendTime;

  struct sPoints {
    bool Triggered;
    jack_nframes_t MinSet, MaxSet;
    float Min, Max;

    void Trigger();

    bool receive(long numSampsToProcess, jack_nframes_t T, float *indata);
  } *Point;
  int Points;

 public:
  bool Measuring() {
    return E;
  }
  int GetRow() {
    return Row;
  }
  Measurement(int _Points);
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
