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

#ifndef _VSCULL_IOCTL_H_
#define _VSCULL_IOCTL_H_ 

#include <linux/ioctl.h>

struct vscull_ioctl
{
    int width;
    int height;
    int depth;
    int palette;
    int fps;
};

#define VSCULL_IOC_MAGIC    'k'

#define VSIOCGPAR   _IOR(VSCULL_IOC_MAGIC, 1, struct vscull_ioctl)
#define VSIOCSPAR   _IOW(VSCULL_IOC_MAGIC, 2, struct vscull_ioctl) 
#define VSIOCGRES   _IOR(VSCULL_IOC_MAGIC, 3, pid_t) 
#define VSIOCSRES   _IOW(VSCULL_IOC_MAGIC, 4, pid_t) 


#endif /* _VSCULL_IOCTL_H_ */
