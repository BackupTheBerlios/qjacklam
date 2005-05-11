/***************************************************************************
 *   Copyright (C) 2004 by Karsten Wiese                                   *
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
/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/
#include "jack.h"

#include <iostream>






using namespace std;







void MainWindow::helpAbout()
{
    
}

void MainWindow::customEvent(QCustomEvent *E)
{
    switch (E->type() - QCustomEvent::User) {
    case eeLatencyValueUnavailable: {
      textLabel1->setText("?");
	}break;
	
    case eeLatencyValue: {
	    int frames = (int)E->data();
	    textLabel1->setText(QString("%1 frames").arg(frames));
	}break;

    case eeGraphOrderChanged: {
      const char ** O, **I;
	    jackConnections(O, I);

	    int i;
	    QString CSO;	    	    
	    if (O) {
		for (i=0; O[i]; i++) {
		    if (!CSO.isEmpty())
			CSO += " + ";
		    CSO += O[i];
		}
		free(O);
	    }
	    QString CSI;	    	    
	    if (I) {
		for (i=0; I[i]; i++) {
		    if (!CSI.isEmpty())
			CSI += " + ";
		    CSI += I[i];
		}
		free(I);
	    }
	    QString Cap("qjacklam");
	    if (!(CSO.isEmpty() && CSI.isEmpty())) {
		Cap += " @";
		if (!CSO.isEmpty())
		  (Cap += " Output ") += CSO;
		if (!CSI.isEmpty())
		  (Cap += " Input ") += CSI;
	    }
	    setCaption(Cap);
	}break;

    }
}





void MainWindow::menubar_activated(int i)
{
    QString w = PortBase;
    w += ":";
    w += IO->text(i);
    cout  << __FUNCTION__ << " "<< w << endl;
    jackConnect(w, outputActive());
    //cout << name() << endl;
}


void MainWindow::menubar_highlighted( int i)
{
    cout << __FUNCTION__ << i << endl;
    if (emOutput == i)
	fillDropdown(Output);
    else
    if (emInput == i)
	fillDropdown(Input);
    else
	if (IO->itemParameter(i))
	    PortBase = IO->text(i);
}


void MainWindow::fillDropdown(QPopupMenu *io)
{
    IO = io;
    IO->clear();
    jackGetPorts(*IO, outputActive());
}




bool MainWindow::outputActive()
{
    return IO == Output;
}
