/*
    Copyright (C) 2005 Karsten Wiese <annabellesgarden@yahoo.de>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/resource.h> 
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <list>
#include <qpopupmenu.h>
#include <qapplication.h>

#include "tools.h"
#include "MainWindow.h"
#include "userevent.h"

extern MainWindow *PW;

using namespace std;



int jackClient::SyncCallback(jack_transport_state_t state __attribute__ ((unused)), jack_position_t *pos __attribute__ ((unused)), void *arg __attribute__ ((unused)))
{
  DEBUGp("%s@%i", TransportStateName[state], pos->frame);
  return 1;
}


// int jackClient::ProcessCallback(jack_nframes_t nframes)
// {
//   int chn;
//   //	size_t i;



//   //	cout << __LINE__ << endl;

//   for (chn = 0; chn < channels_i; chn++) {
//     in[chn] = (jack_default_audio_sample_t *)jack_port_get_buffer (ports_i[chn], nframes);
//     //		cout << " : " << in[chn];
//   }
//   for (chn = 0; chn < channels_o; chn++) {
//     out[chn] = (jack_default_audio_sample_t *)jack_port_get_buffer (ports_o[chn], nframes);
//     //		cout << " : " << in[chn];
//   }
//   MeasureLatency(nframes, SampleRate, in[0], out[0]);


//   //cout << " # " << nframes << endl;

//   /* Tell the disk thread there is work to do.  If it is already
//    * running, the lock will not be available.  We can't wait
//    * here in the process() thread, but we don't need to signal
//    * in that case, because the disk thread will read all the
//    * data queued before waiting again. */
//   // 	if (pthread_mutex_trylock (&disk_thread_lock) == 0) {
//   // 	    pthread_cond_signal (&data_ready);
//   // 	    pthread_mutex_unlock (&disk_thread_lock);
//   // 	}

//   return 0;
// }

void jackClient::SetupPorts(int n_i, int n_o)
{
  /* Allocate data structures that depend on the number of ports. */
  ports_i = (jack_port_t **) calloc (n_i, sizeof (jack_port_t *));
  ports_o = (jack_port_t **) calloc (n_o, sizeof (jack_port_t *));

  /* When JACK is running realtime, jack_activate() will have
   * called mlockall() to lock our pages into memory.  But, we
   * still need to touch any newly allocated pages before
   * process() starts using them.  Otherwise, a page fault could
   * create a delay that would force JACK to shut us down. */

  for (channels_i = 0; channels_i < n_i; channels_i++) {
    char name[64];

    sprintf (name, "input%d", channels_i+1);
    if ((ports_i[channels_i] = jack_port_register (client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)) == 0) {
      fprintf (stderr, "cannot register input port \"%s\"!\n", name);
      jack_client_close (client);
      exit (1);
    }
  }

  for (channels_o = 0; channels_o < n_o; channels_o++) {
    char name[64];

    sprintf (name, "output%d", channels_o+1);
    if ((ports_o[channels_o] = jack_port_register (client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) == 0) {
      fprintf (stderr, "cannot register output port \"%s\"!\n", name);
      jack_client_close (client);
      exit (1);
    }
  }

}


int jackClient::GraphOrderCallback(void *arg __attribute__ ((unused)))
{
  //  cout << __FUNCTION__ << endl;

  //const char ** C = jack_port_get_connections (ports[0]);

  QCustomEvent * E =  new QCustomEvent(QCustomEvent::User + eeGraphOrderChanged);
  //E->setData(C);
  QApplication::postEvent(PW, E);

  //   C = jack_port_get_connections (ports[1]);

  //   E =  new QCustomEvent(QCustomEvent::User + eeOutputConnections);
  //   E->setData(C);
  //   QApplication::postEvent(PW, E);
  return 0;
}


int jackClient::jackUp(const char * name, JackProcessCallback process_callback, void *instance, bool allcallbacks)
{
  opterr = 0;

  if ((client = jack_client_new (name)) == 0) {
    fprintf (stderr, "jack server not running?\n");
    exit (1);
  }
  jack_set_sample_rate_callback(client, SampleRateCallback, this);

  {
    jack_on_shutdown (client, jack_shutdown, this);
    jack_set_process_callback(client, process_callback, instance);
    if (allcallbacks) {
      jack_set_sync_callback(client, SyncCallback, this);
      jack_set_graph_order_callback(client, GraphOrderCallback, this);
    }
    if (jack_activate (client)) {
      fprintf (stderr, "cannot activate client\n");
      exit(1);
    }
  }
  return 0;
}

void jackClient::jackDown()
{
  jack_client_close(client);

  DEBUGp("Ende");
}









jackClient *Sender, *Receiver;




class Port
{
  const char* Name;
  string _Name;
public:
  Port(const char* name):Name(name), _Name(strchr(name, ':') + 1) {}
  ~Port() {
    //    cout << __FUNCTION__ << " " << Name  << " @" << (void*)Name << endl;
    //::free((void*)Name);
  }

  const string& name() {return _Name;}

};



// class Client
// {
//   typedef list<Client*> TClients;
//   static TClients Clients;

//   typedef list<Port*> TPorts;
//   TPorts Ports;
//   string Name;
//   QPopupMenu *ChildM;
//   void addPort(const char * name)
//   {
//     Port *P;
//     Ports.push_back(P = new Port(name));
//     ChildM->setItemParameter(ChildM->insertItem(P->name()), 0);
//   }

//   Client(const string &name, QPopupMenu &m):Name(name) {
//     ChildM = new QPopupMenu;
//     m.setItemParameter(m.insertItem(Name, ChildM), 1);
//   }

//   ~Client()
//   {
//     //cout << __FUNCTION__ << " " << Name << endl;
//     TPorts::iterator p = Ports.begin();
//     while (p != Ports.end()) {
//       delete *p;
//       ++p;
//     }
//   }
// public:
//   static void addClientPort(const char * name, QPopupMenu &M)
//   {
//     string ClientName = name;
//     ClientName.resize(ClientName.find(':'));
//     //cout << __FUNCTION__ << " " << name << " @" << (void*)name << endl;
//     if (ClientName == "lam")
//       return;
//     TClients::iterator i = Clients.begin();
//     Client * C = NULL;
//     while (i != Clients.end()) {
//       if ((*i)->Name == ClientName) {
// 	C = *i;
// 	break;
//       }
//       ++i;
//     }
//     if (!C) {
//       C = new Client(ClientName, M);
//       Clients.push_back(C);
//     }
//     C->addPort(name);
//   }

//   static void clearAll()
//   {
//     TClients::iterator i = Clients.begin();
//     while (i != Clients.end()) {
//       //cout << __FUNCTION__ << " " << (*i)->Name << endl;
//       delete (*i);
//       i++;
//     }

//     Clients.clear();
//   }
// };

// Client::TClients Client::Clients;

// void jackGetPorts(QPopupMenu &M, bool out)
// {
//   jackClient * jc = out ? Sender : Receiver;
//   const char **Ports = jc->getPorts(out ? JackPortIsInput : JackPortIsOutput);
//   int i = 0;
//   if (Ports) {
//     while (Ports[i]) {
//       //      cout << Ports[i] << endl;
//       Client::addClientPort(Ports[i], M);
//       ++i;
//     }
//     free(Ports);
//   }

//   Client::clearAll();
// }

// void jackConnect(const char *Input, bool out)
// {
//   jackClient * jc = out ? Sender : Receiver;
//   jc->Connect(Input, out);
// }

// void jackConnections(const char **&O, const char **&I)
// {
//   O = Sender->GetAllConnections(true);
//   I = Receiver->GetAllConnections(false);
// }


