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
