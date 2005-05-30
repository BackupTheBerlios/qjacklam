#include "MainWindow.h"

class MainWindowI: public MainWindow {
  Q_OBJECT
    const char ** InPorts;
    const char ** OutPorts;
    Measurement *M;
    struct Job *J;
    int TimeOut;
    bool HideSilent;
    void DoRow(int row);
 protected slots:
    void RowClicked(int row);
    bool PortsChanged();
    bool PortsChanged(const char ** was, const char ** now);
    void SendAllClicked();
    void RepeatToggled(bool);
    void TimeOutChanged(int);
    void HideSilentToggled(bool);
 protected:
  virtual void customEvent( QCustomEvent * E );

 public:
  MainWindowI( QWidget* parent = 0, const char* name = 0, WFlags fl = WType_TopLevel );

  ~MainWindowI();
};
