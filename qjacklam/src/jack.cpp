/*
    Copyright (C) 2001 Paul Davis
    Copyright (C) 2003 Jack O'Quin
    
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

    * 2002/08/23 - modify for libsndfile 1.0.0 <andy@alsaplayer.org>
    * 2003/05/26 - use ringbuffers - joq
    
    $Id: jack.cpp,v 1.1 2005/05/11 13:30:12 fzu Exp $
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/resource.h> 
#include <unistd.h>
#include <sndfile.h>
#include <pthread.h>
#include <getopt.h>
#include <jack/jack.h>
#include <signal.h>
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <list>
#include <qpopupmenu.h>
#include <qapplication.h>

#include "tools.h"
#include "jack.h"
#include "MainWindow.h"

extern MainWindow *PW;

using namespace std;

typedef struct _thread_info {
  pthread_t thread_id;
  //    SNDFILE *sf;
  jack_nframes_t duration;
  jack_client_t *client;
  int channels;
  int bitdepth;
  volatile int can_capture;
  volatile int can_process;
  volatile int status;
  volatile jack_nframes_t SampleRate;
} thread_info_t;

/* JACK data */
static jack_port_t **ports;
static jack_default_audio_sample_t **in, **out;
//jack_nframes_t nframes;
const size_t sample_size = sizeof(jack_default_audio_sample_t);

pthread_mutex_t disk_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  data_ready = PTHREAD_COND_INITIALIZER;
long overruns = 0;

static jack_client_t *client;

static int SampleRateCallback(jack_nframes_t nframes, void *arg) 
{
  ((thread_info_t *)arg)->SampleRate = nframes;
  return 0;
}

static int SyncCallback(jack_transport_state_t state, jack_position_t *pos, void *arg)
{
  DEBUG("%s@%i", TransportStateName[state], pos->frame);
  return 1;
}

void MeasureLatency(long numSampsToProcess, int sampleRate, float *indata, float *outdata);

	
static int ProcessCallback(jack_nframes_t nframes, void *arg)
{
	int chn;
	//	size_t i;
	thread_info_t *info = (thread_info_t *) arg;



	//	cout << __LINE__ << endl;
	/* Do nothing until we're ready to begin. */
	if (!info->can_process)
		return 0;

	for (chn = 0; chn < info->channels; chn++) {
		in[chn] = (jack_default_audio_sample_t *)jack_port_get_buffer (ports[chn], nframes);
		out[chn] = (jack_default_audio_sample_t *)jack_port_get_buffer (ports[chn+1], nframes);
		//		cout << " : " << in[chn];
	}
	MeasureLatency(nframes, info->SampleRate, in[0], out[0]);


	//cout << " # " << nframes << endl;

	/* Tell the disk thread there is work to do.  If it is already
	 * running, the lock will not be available.  We can't wait
	 * here in the process() thread, but we don't need to signal
	 * in that case, because the disk thread will read all the
	 * data queued before waiting again. */
// 	if (pthread_mutex_trylock (&disk_thread_lock) == 0) {
// 	    pthread_cond_signal (&data_ready);
// 	    pthread_mutex_unlock (&disk_thread_lock);
// 	}

	return 0;
}



static int GraphOrderCallback(void *arg)
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

static void jack_shutdown(void *arg)
{
	fprintf (stderr, "JACK shutdown\n");
	// exit (0);
	abort();
}


static void setup_ports(thread_info_t *info)
{
	int i;
	size_t in_size;

	/* Allocate data structures that depend on the number of ports. */
	ports = (jack_port_t **) malloc (sizeof (jack_port_t *) * 2 * info->channels);
	in_size =  info->channels * sizeof (jack_default_audio_sample_t *);
	in = (jack_default_audio_sample_t **) malloc (in_size);
	out = (jack_default_audio_sample_t **) malloc (in_size);

	/* When JACK is running realtime, jack_activate() will have
	 * called mlockall() to lock our pages into memory.  But, we
	 * still need to touch any newly allocated pages before
	 * process() starts using them.  Otherwise, a page fault could
	 * create a delay that would force JACK to shut us down. */
	memset(in, 0, in_size);
	memset(out, 0, in_size);

	for (i = 0; i < info->channels; i++) {
		char name[64];

		sprintf (name, "input%d", i+1);
		if ((ports[i] = jack_port_register (info->client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)) == 0) {
			fprintf (stderr, "cannot register input port \"%s\"!\n", name);
			jack_client_close (info->client);
			exit (1);
		}
		sprintf (name, "output%d", i+1);
		if ((ports[i + 1] = jack_port_register (info->client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) == 0) {
			fprintf (stderr, "cannot register output port \"%s\"!\n", name);
			jack_client_close (info->client);
			exit (1);
		}
	}

	info->can_process = 1;		/* process() can start, now */
}


static thread_info_t thread_info;

int jackUp()
{
  memset (&thread_info, 0, sizeof (thread_info));
  thread_info.channels = 1;
  opterr = 0;

  if ((client = jack_client_new ("lam")) == 0) {
    fprintf (stderr, "jack server not running?\n");
    exit (1);
  }
  jack_set_sample_rate_callback(client, SampleRateCallback, &thread_info);

  thread_info.client = client;
  thread_info.can_process = 0;

  {

    jack_on_shutdown (client, jack_shutdown, &thread_info);
    jack_set_sync_callback(client, SyncCallback, &thread_info);
    jack_set_process_callback(client, ProcessCallback, &thread_info);

    setup_ports (&thread_info);
    jack_set_graph_order_callback(client, GraphOrderCallback, NULL);
    if (jack_activate (client)) {
      fprintf (stderr, "cannot activate client\n");
      exit(1);
    }

  }
  return 0;
}

void jackDown()
{

  jack_client_close(client);

  DEBUG("Ende");

}


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



class Client
{
  typedef list<Client*> TClients;
  static TClients Clients;

  typedef list<Port*> TPorts;
  TPorts Ports;
  string Name;
  QPopupMenu *ChildM;
  void addPort(const char * name)
  {
    Port *P;
    Ports.push_back(P = new Port(name));
    ChildM->setItemParameter(ChildM->insertItem(P->name()), 0);
  }

  Client(const string &name, QPopupMenu &m):Name(name) {
    ChildM = new QPopupMenu;
    m.setItemParameter(m.insertItem(Name, ChildM), 1);
  }

  ~Client()
  {
    //cout << __FUNCTION__ << " " << Name << endl;
    TPorts::iterator p = Ports.begin();
    while (p != Ports.end()) {
      delete *p;
      ++p;
    }
  }
public:
  static void addClientPort(const char * name, QPopupMenu &M)
  {
    string ClientName = name;
    ClientName.resize(ClientName.find(':'));
    //cout << __FUNCTION__ << " " << name << " @" << (void*)name << endl;
    if (ClientName == "lam")
      return;
    TClients::iterator i = Clients.begin();
    Client * C = NULL;
    while (i != Clients.end()) {
      if ((*i)->Name == ClientName) {
	C = *i;
	break;
      }
      ++i;
    }
    if (!C) {
      C = new Client(ClientName, M);
      Clients.push_back(C);
    }
    C->addPort(name);
  }

  static void clearAll()
  {
    TClients::iterator i = Clients.begin();
    while (i != Clients.end()) {
      //cout << __FUNCTION__ << " " << (*i)->Name << endl;
      delete (*i);
      i++;
    }

    Clients.clear();
  }
};

Client::TClients Client::Clients;

void jackGetPorts(QPopupMenu &M, bool out)
{
  const char **Ports = jack_get_ports(client, NULL, NULL, out ? JackPortIsInput : JackPortIsOutput);
  int i = 0;
  if (Ports) {
    while (Ports[i]) {
      //      cout << Ports[i] << endl;
      Client::addClientPort(Ports[i], M);
      ++i;
    }
    free(Ports);
  }

  Client::clearAll();
}

void jackConnect(const char *Input, bool out)
{
  if (out) {
    jack_port_disconnect(client, ports[1]);
    jack_connect(client, jack_port_name(ports[1]), Input);
  } else {
    jack_port_disconnect(client, ports[0]);
    jack_connect(client, Input, jack_port_name(ports[0]));
  }
}

void jackConnections(const char **&O, const char **&I)
{
  O = jack_port_get_all_connections(client, ports[1]);
  I = jack_port_get_all_connections(client, ports[0]);
}
