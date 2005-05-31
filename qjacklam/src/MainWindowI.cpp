#include "MainWindowI.h"
#include <qtable.h> 
#include <qlabel.h> 
#include <qpushbutton.h> 
#include <qcheckbox.h> 
#include <qspinbox.h> 
#include <iostream>
#include "userevent.h"

using namespace std;

struct Job {
  int Rows;
  int Row;
  bool Repeat;
  Job():Repeat(false){}
};


MainWindowI::MainWindowI( QWidget* parent, const char* name, WFlags fl)
  :MainWindow(parent, name, fl)
   ,InPorts(NULL)
   ,OutPorts(NULL)
   ,M(NULL)
   ,J(NULL)
   ,TimeOut(20000)
   ,HideSilent(false)
{
  connect(table->verticalHeader(), SIGNAL( clicked(int) ), this, SLOT( RowClicked(int) ));
  connect(ButtonSendAll, SIGNAL( clicked() ), this, SLOT( SendAllClicked() ));
  connect(checkBoxRepeat, SIGNAL( toggled(bool) ), this, SLOT( RepeatToggled(bool) ));
  TimeOutSpinBox->setValue(TimeOut);
  connect(TimeOutSpinBox, SIGNAL( valueChanged(int) ), this, SLOT( TimeOutChanged(int) ));
  connect(checkBoxHideSilent, SIGNAL( toggled(bool) ), this, SLOT( HideSilentToggled(bool) ));
  //  table->setSelectionMode(QTable::SingleRow);
}

MainWindowI::~MainWindowI()
    {}

void MainWindowI::HideSilentToggled(bool hs)
{
  HideSilent = hs;
  if (!HideSilent) {
    int i;
    for (i = 0; i < table->numRows(); ++i)
      table->showRow(i);
    for (i = 0; i < table->numCols(); ++i)
      table->showColumn(i);
  }//  else {
//     int i;
//     for (i = 0; i < table->numRows(); ++i)
//       table->hideRow(i);
//     for (i = 0; i < table->numCols(); ++i)
//       table->hideColumn(i);
//   }
}

void  MainWindowI::RepeatToggled(bool Selected)
{
  //  cout << Selected << endl;
  if (J) {
    if (!Selected)
      J->Repeat = false;
  } else {
    Job *j = new Job;
    j->Repeat = true;
    J = j;
  }
}

void MainWindowI::SendAllClicked()
{
  if (!J)
    J = new Job();
  J->Rows = table->verticalHeader()->count();
  if (M->Measuring())
    return;
  J->Row = 0;
  DoRow(0);
}

void MainWindowI::DoRow(int row)
{
  //  cout << __PRETTY_FUNCTION__ << row <<endl;
  table->selectRow(row);
  Sender->Connect(table->verticalHeader()->label(row), true);
  M->Arm(row);
}

void MainWindowI::RowClicked(int row)
{
  if (J) {
    J->Rows = 1;
    J->Row = row;
  }
  if (M->Measuring())
    return;
  //  cout << __PRETTY_FUNCTION__ << row <<endl;
  DoRow(row);
}

void MainWindowI::TimeOutChanged(int to)
{
  TimeOut = to;
  if (M)
    M->SetTimeOut(TimeOut);
  TimeOutSpinBox->setLineStep(to / 4);
}

void MainWindowI::customEvent(QCustomEvent *E)
{
    switch (E->type() - QCustomEvent::User) {
//     case eeLatencyValueUnavailable: {
//       textLabel1->setText("?");
//     }break;
    
    case eeLatencyValue: {
      Sender->Connect(NULL, true);
      int sel = 0;
      while (sel < table->numSelections()) {
	table->removeSelection (sel++);
      }
      table->updateHeaderStates();
//       cout << M << endl;
//       cout << (void*)E->data() << endl;
//       M->Dump();
      //      textLabel1->setText(QString("%1 frames").arg(frames));
      int col;
      bool Show = false;
      for (col = 0; col < table->numCols(); ++col) {
	int Frames = M->Frames(col);
	QString S;
	if (Frames >= 0) {
	  Show = true;
	  S = QString("%1").arg(Frames);
	} else
	  S= "?";
	table->setText(M->GetRow(), col, S);
      }
      if (HideSilent) {
	if (Show)
	  table->showRow(M->GetRow());
	else
	  table->hideRow(M->GetRow());
	for (col = 0; col < table->numCols(); ++col) {
	  Show = false;
	  for (int row = 0; row < table->numRows(); ++row) {
	    string s = table->text(row, col);
	    if (!(s == "?")) {
	      Show = true;
	      break;
	    }
	  }
	  if (Show) {
	    table->adjustColumn(col);
	    table->showColumn(col);
	  } else {
	    table->hideColumn(col);
	  }
	}
      }
      if (J) {
	if (!J->Repeat && (1 + J->Row) >= J->Rows) {
	  delete J;
	  J = NULL;
	} else {
	  if (J->Rows > 1)
	    if (++J->Row >= J->Rows)
	      J->Row = 0;
	  DoRow(J->Row);
	} 
      }
    }break;

    case eeGraphOrderChanged:
      if (PortsChanged()) {
	Receiver->UnregisterPorts();
	Sender->UnregisterPorts();
	delete M;
	const char **Ports = Sender->getPorts(JackPortIsInput);
	int n_o = 0;
	if (Ports) {
	  int i;
	  for (i = 0; Ports[i]; i++);
	  table->setNumRows(i);
	  while (--i >= 0) {
	    table->verticalHeader()->setLabel(i, Ports[i]);
	  }
	  free(Ports);
	}
	Ports = Sender->getPorts(JackPortIsOutput);
	if (Ports) {
	  int i;
	  for (i = 0; Ports[i]; i++);
	  table->setNumCols(n_o = i);
	  while (--i >= 0) {
	    table->horizontalHeader()->setLabel(i, Ports[i]);
	  }
	  free(Ports);
	}
	//	table->setColumnWidth ( 1, 0);
	Sender->SetupPorts(0, 1);
	Receiver->SetupPorts(n_o, 0);
	int col;
	for (col = 0; col < Receiver->Channels(false); ++col) {
	  Receiver->Connect(table->horizontalHeader()->label(col), false, col);
	}
	InPorts = Sender->getPorts(JackPortIsInput);
	OutPorts = Sender->getPorts(JackPortIsOutput);
	M = new Measurement(n_o, TimeOut);
	//	ButtonSendAll->setFixedSize(table->verticalHeader()->width()-1, table->horizontalHeader()->height());
      }
//       {
// 	const char ** O, **I;
// 	jackConnections(O, I);

// 	int i;
// 	QString CSO;	    	    
// 	if (O) {
// 	  for (i=0; O[i]; i++) {
// 	    if (!CSO.isEmpty())
// 	      CSO += " + ";
// 	    CSO += O[i];
// 	  }
// 	  free(O);
// 	}
// 	QString CSI;	    	    
// 	if (I) {
// 	  for (i=0; I[i]; i++) {
// 	    if (!CSI.isEmpty())
// 	      CSI += " + ";
// 	    CSI += I[i];
// 	  }
// 	  free(I);
// 	}
// 	QString Cap("qjacklam");
// 	if (!(CSO.isEmpty() && CSI.isEmpty())) {
// 	  Cap += " @";
// 	  if (!CSO.isEmpty())
// 	    (Cap += " Output ") += CSO;
// 	  if (!CSI.isEmpty())
// 	    (Cap += " Input ") += CSI;
// 	}
// 	setCaption(Cap);
//       }
      break;
    }
}

bool MainWindowI::PortsChanged(const char ** was, const char ** now)
{
  int i;
  for (i = 0; was[i] && now[i]; i++)
    if (was[i] != now[i])
      return true;
  return was[i] || now[i];
}


bool MainWindowI::PortsChanged()
{
  if (!InPorts || !OutPorts)
    return true;
  const char ** InPortsNow = Sender->getPorts(JackPortIsInput),
    ** OutPortsNow = Sender->getPorts(JackPortIsOutput);
  bool ret = PortsChanged(InPorts, InPortsNow) || PortsChanged(OutPorts, OutPortsNow);
  free(InPortsNow);
  free(OutPortsNow);
  if (ret) {
    free(InPorts);
    free(OutPorts);
  }
  return ret;
}
