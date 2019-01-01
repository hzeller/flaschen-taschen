// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
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

#include <unistd.h>
#include <chrono>
#include <sys/socket.h>
#include <string.h>
#include <cerrno>
#include <stdio.h>

#include "servers.h"
#include "composite-flaschen-taschen.h"
#include "ft-thread.h"
#include "image-reader.h"

Server::Server(){}

void Server::InterruptHandler(int) {
  interrupt_received = true;
}

bool Server::interrupt_received=false;

bool Server::use_constant_async_fps= fps != 0;

void* Server::periodically_send_to_display(void* ptr) {
    arg_struct* args = (arg_struct*) ptr;
    CompositeFlaschenTaschen *display = args->display;
    ft::Mutex *mutex = args->mutex;
    const auto timeWindow = std::chrono::milliseconds(int(1000/fps));
    while(true){
        auto start = std::chrono::steady_clock::now();

        if (Server::interrupt_received){
            break;
        }

        mutex->Lock();
        display->Send();
        mutex->Unlock();

        auto end = std::chrono::steady_clock::now();
        auto elapsed = end - start;
        std::chrono::duration<double> timeToWait = timeWindow - elapsed;
        if(timeToWait > std::chrono::milliseconds::zero())
        {
            usleep(timeToWait.count()*1000000);
        }
    }
    return NULL;
}

void* Server::receive_data_and_set_display_pixel(void* ptr) {
    arg_struct* args = (arg_struct*) ptr;

    int new_socket = args->socket;
    CompositeFlaschenTaschen *display = args->display;
    ft::Mutex *mutex = args->mutex;

    const int kBufferSize = 65535;  // maximum UDP or TCP has to offer.
    char *packet_buffer = new char[kBufferSize];

    for(;;){
        if (Server::interrupt_received){
            break;
        }

        bzero(packet_buffer, kBufferSize); // important to clear the buffer every loop!
        ssize_t received_bytes = recv(new_socket,
                                          packet_buffer, kBufferSize,
                                          0);
        if (received_bytes==0){
            // connection closed
            break;
        }

        if (received_bytes < 0 && errno == EINTR){
            // Other signals. Don't care.
            continue;
        }

        if (received_bytes < 0) {
            perror("Trouble receiving.");
            break;
        }

        ImageMetaInfo img_info = {0};
        img_info.width = display->width();  // defaults.
        img_info.height = display->height();

        // Create Image object and read in from pixel data above
        const char *pixel_pos = ReadImageData(packet_buffer, received_bytes,
                                           &img_info);

        char* start_pixel = (char*)pixel_pos; // todo: find other way

        mutex->Lock();
        display->SetLayer(img_info.layer);
        for (int y = 0; y < img_info.height; ++y) {
            for (int x = 0; x < img_info.width; ++x) {
                Color c;
                c.r = *pixel_pos++;
                c.g = *pixel_pos++;
                c.b = *pixel_pos++;
                display->SetPixel(x + img_info.offset_x,
                                 y + img_info.offset_y,
                                  c);
            }
        }

        if(!Server::use_constant_async_fps)
            display->Send();

        display->SetLayer(0);  // Back to sane default.
        mutex->Unlock();

        if(img_info.type==ImageType::PNG)
            free(start_pixel);// todo: find other way
    }
    delete [] packet_buffer;
    close (new_socket);
    return NULL;
}


Server::~Server()
{
  // Compulsory virtual destructor definition,
  // even if it's empty
}

