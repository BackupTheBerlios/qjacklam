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
int jackUp();
void jackDown();
void jackGetPorts(QPopupMenu &M, bool out=false);
void jackConnect(const char *Input, bool out=false);
void jackConnections(const char **&O, const char **&I);

#include <jack/jack.h>


class jackClient {
  jack_client_t *client;
  int channels_i, channels_o;
  jack_port_t **ports_i, **ports_o;
  volatile int status;
  volatile jack_nframes_t SampleRate;


  /*extern "C"*/
  static int SampleRateCallback(jack_nframes_t nframes, void *arg) {
    ((jackClient *)arg)->SampleRate = nframes;
    return 0;
  }

  static int SyncCallback(jack_transport_state_t state, jack_position_t *pos, void *arg);

  static void jack_shutdown(void *args __attribute__ ((unused))) {
    fprintf (stderr, "JACK shutdown\n");
    // exit (0);
    abort();
  }

  static int GraphOrderCallback(void *arg);

public:
/*   jackClient(int n_i, int n_o) */
/*     :client(NULL) */
/*     ,channels_i(n_i) */
/*     ,channels_o(n_o) */
/*   { */
/*   } */
  jackClient()
    :client(NULL)
    ,channels_i(0)
    ,channels_o(0)
    ,ports_i(NULL)
    ,ports_o(NULL)
  {
  }

  int Channels(bool out)
    {
      return out ? channels_o : channels_i;
    }

  void SetupPorts(int n_i, int n_o);

  int jackUp(const char * name, JackProcessCallback process_callback, void *instance, bool allcallbacks = false);

  void jackDown();

  const char ** getPorts(unsigned long flags) {
    return jack_get_ports(client, NULL, NULL, flags);
  }

  jack_default_audio_sample_t *getBuffer(jack_nframes_t nframes, bool out, int ch) {
    if ((out ? channels_o : channels_i) <= ch)
      return NULL;
    return (jack_default_audio_sample_t *)jack_port_get_buffer(out ? ports_o[ch] : ports_i[ch], nframes);
  }

  void Connect(const char *put, bool out, int ch = 0) {
    if (out) {
      jack_port_disconnect(client, ports_o[ch]);
      jack_connect(client, jack_port_name(ports_o[ch]), put);
    } else {
      jack_port_disconnect(client, ports_i[ch]);
      jack_connect(client, put, jack_port_name(ports_i[ch]));
    }
  }

  const char ** GetAllConnections(bool out) {
    return jack_port_get_all_connections(client, out ? ports_o[0] : ports_i[0]);
  }
  jack_nframes_t GetSampleRate() {
    return SampleRate;
  }
  jack_nframes_t LastFrameTime() {
    return jack_last_frame_time(client);
  }
  int Deactivate() {
    return jack_deactivate(client);
  }
  int UnregisterPorts() {
    int error;
    while (channels_i)
      if (error = jack_port_unregister(client, ports_i[--channels_i]))
	return error;
    free(ports_i);
    ports_i = NULL;
    while (channels_o)
      if (error = jack_port_unregister(client, ports_o[--channels_o]))
	return error;
    free(ports_o);
    ports_o = NULL;
    return 0;
  }


};

extern jackClient *Sender, *Receiver;

