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
#include <vscull_palette.h>
#include <vscull_dev.h>

#include <getopt.h>

extern char *__progname;

const char usage[]=
      "%s [options]\n"
      "   -m minor            vscull video device (/dev/video0 is default)\n"
      "   -W width            \n"
      "   -H height           \n"
      "   -p palette          [1-16] see include/linux/videodev.h\n"
      "   -d depth            32/24 bit per pixel\n"
      "   -f fps              frame per second\n"
      "   -h                  print this help\n";

int
main(int argc, char *argv[])
{
    int i;
    int minor = 0;

    int w = -1;
    int h = -1;
    int p = -1;
    int d = -1;
    int f = -1;

    while(( i = getopt(argc, argv, "m:W:H:p:d:f:h")) != EOF)
        switch(i) {
        case 'm': minor = atoi(optarg);
                  break;
        case 'W': w = atoi(optarg);
                  break;
        case 'H': h = atoi(optarg);
                  break;
        case 'p': p = atoi(optarg);
                  break;
        case 'd': d = atoi(optarg); 
                  break;
        case 'f': f = atoi(optarg);
                  break;
        case 'h': fprintf(stderr,usage,__progname); exit(0);
        case '?': fprintf(stderr,"unknown option!\n"); exit(1);
        }

    vscull::Dev dev(minor);

    if (w > 0) {
        dev.width(w);
    }
    if (h > 0) {
        dev.height(h);
    } 
    if (p > 0) {
        dev.palette(p);
    }
    if (d > -1) {
        dev.depth(d);
    }
    if (f > -1) {
        dev.fps(f);
    }

    if( dev.commit() ) {
        std::cout << "committing changes...\n"; 
    }

    dev.update();

    std::cout << dev.name() << " vscull settings: \n"; 
    std::cout << "   width  : " << dev.width() << std::endl;    
    std::cout << "   height : " << dev.height() << std::endl;    
    std::cout << "   palette: " << dev.palette() << "[" << PALETTE(dev.palette()) << "]" << std::endl;    
    std::cout << "   depth  : " << dev.depth() << std::endl;    
    std::cout << "   fps    : " << dev.fps() << std::endl;    

    return 0;
}
 
