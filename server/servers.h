// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://gnu.org/licenses/gpl-2.0.txt>
//
// Various servers we support, all competing for the same display.

#ifndef FT_SERVER_H
#define FT_SERVER_H

class FlaschenTaschen;

namespace ft {
class Mutex;
}

// Our main service that we always support.
bool udp_server_init(int port);
void udp_server_run_blocking(FlaschenTaschen *display, ft::Mutex *mutex);

// Optional services, chosen on command-line
bool opc_server_init(int port);
void opc_server_run_thread(FlaschenTaschen *display, ft::Mutex *mutex);

bool pixel_pusher_init(FlaschenTaschen *canvas);
void pixel_pusher_run_thread(FlaschenTaschen *display, ft::Mutex *mutex);

#endif // OPC_SERVER_H
