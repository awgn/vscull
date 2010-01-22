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


#ifndef _VSCULL_DEV_H_
#define _VSCULL_DEV_H_ 

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

// #include <vscull_dev.h>

#include <string>
#include <stdexcept>

namespace vscull {

    class Dev
    {
        int         _M_fd;
        int         _M_minor; 
        std::string _M_dev;

        struct vscull_ioctl _M_par;

        bool        _M_changes;

    public:
        Dev(int min = 0)
        : _M_fd(0),
        _M_minor(min),
        _M_dev(),
        _M_par(),
        _M_changes(false)
        {
            char dev[80];
            sprintf(dev, "/dev/video%d", _M_minor);
            _M_dev.append(dev);

            _M_fd = open(_M_dev.c_str(), O_RDWR);
            if (_M_fd < 0) {
                throw std::runtime_error(std::string("open: couldn't open ").append(_M_dev)); 
            }

            update();
        }

        ~Dev()
        {
            close(_M_fd);
        }

        bool commit() 
        {
            if (!_M_changes) {
                return false; 
            }

            if ( ioctl(_M_fd, VSIOCSPAR, &_M_par) < 0 ) {
                std::clog << "ioctl: VSIOCSPAR error" << std::endl;
                return false;
            }
            return true;
        }

        bool update() const
        {
            if ( ioctl(_M_fd, VSIOCGPAR, &_M_par) < 0 ) {
                std::clog << "ioctl: VSIOCGPAR error" << std::endl;
                return false;
            }
            return true;
        }

        const std::string
        name() const
        { return _M_dev; }

        const int
        minor() const
        { return _M_minor; }

        const int 
        width() const 
        { return _M_par.width; }

        const int 
        height() const 
        { return _M_par.height; }

        const int
        depth() const
        { return _M_par.depth; }

        const int
        palette() const
        { return _M_par.palette; }

        const int
        fps() const
        { return _M_par.fps; }

        void width(int w)
        { set(_M_par.width,w); }

        void height(int h)
        { set(_M_par.height,h); }

        void depth(int d)
        { set(_M_par.depth,d); }

        void palette(int p)
        { set(_M_par.palette,p); }

        void fps(int f)
        { set(_M_par.fps,f); } 

    private:
        // non-copyable idiom
        Dev(const Dev &);
        Dev & operator=(const Dev &);

        template <typename T>
        void set(T & r, T val)
        {
            if (r != val) {
                r = val;
                _M_changes = true;
            }
        }
    };
}

#endif /* _VSCULL_DEV_H_ */
