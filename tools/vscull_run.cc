/*
    Copyright (c) 2009 Nicola Bonelli <n.bonelli@netresults.it>
 
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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <iostream>
#include <cstdlib>

#include <vscull_ioctl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>
#include <err.h>


int
main(int argc, char *argv[])
{
    int sta;

    if (argc < 4) {
        std::cout << "usage: " << argv[0] << " id_video id_audio -- argv[0] argv[1]..." << std::endl;
        exit(0);
    }

    int id_video = atoi(argv[1]);
    int id_audio = atoi(argv[2]);

    pid_t pid = fork();
    if (pid == -1) 
        err(1, "fork");

    if ( pid == 0 ) {    /* child */
            
        char dev[80]; sprintf(dev, "/dev/video%d", id_video);
       
        int fd = open(dev, O_RDWR);
        if (fd < 0) {
            err(2, "open");
        }

        /* confim reservation to me */
        pid = getpid();
        if( ioctl(fd, VSIOCSRES, &pid) < 0 )
            err(2, "ioctl");

        char alsa_card[80]; sprintf(alsa_card, "ALSA_CARD=%d", id_audio);
        putenv(alsa_card);

        /* exec command line... */
        std::cout << "running: " << alsa_card << " " << argv[3] << "...\n";
        if ( execv(argv[3], &argv[3]) < 0 ) {
            err(3, "execve");
        }
    }

    wait(&sta);
    return 0;
}
 
