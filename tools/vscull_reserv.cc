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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <vscull_ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cout << "usage: " << argv[0] << " /dev/videoX {new_pid}" << std::endl;
        return 0;
    }    

    int fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        err(1, "error");
    }

    // read the reservation...

    pid_t pid;

    if( ioctl(fd, VSIOCGRES, &pid) < 0 ) 
        err(2, "ioctl");

    std::cout << argv[1] << " reserved to " << pid << std::endl;
 
    if (argc > 2 ) { 

        pid = atoi(argv[2]); 

        if( ioctl(fd, VSIOCSRES, &pid) < 0 ) 
            err(2, "ioctl");
    }

    return 0;
}
 
