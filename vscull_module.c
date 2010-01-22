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

#include <linux/init.h>
#include <linux/module.h>

#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/time.h>
#include <linux/videodev.h>
#include <media/v4l2-common.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#include <media/v4l2-ioctl.h>
#endif

#include "vscull_ioctl.h"
#include "vscull_palette.h"

/* module parameter */

#define NDEVS    8          /* max. number of video devices allowed */
#define MAXDEVS 256         /* max. minor for video devices */

#define VIDEOFRAME_SIZE(w,h,d)       ((w*h*(d>>3)))

static unsigned int ndevs       = 1;
static unsigned int fps         = 25; 
static unsigned int width       = 320;
static unsigned int height      = 240;
static unsigned int brightness  = 32768;
static unsigned int hue         = 32768;
static unsigned int colour      = 32768;
static unsigned int contrast    = 32768;
static unsigned int whiteness   = 32768;
static unsigned int depth       = 32;
static unsigned int palette     = 15;   // default: yuv 4:2:0 planar 
static unsigned int debug       = 0;
static unsigned int framebuf    = 1;

/* v4l palettes available are defined in include/linux/videodev.h:

VIDEO_PALETTE_GREY  1           Linear greyscale 
VIDEO_PALETTE_HI240 2           High 240 cube (BT848) 
VIDEO_PALETTE_RGB565    3       565 16 bit RGB 
VIDEO_PALETTE_RGB24 4           24bit RGB 
VIDEO_PALETTE_RGB32 5           32bit RGB 
VIDEO_PALETTE_RGB555    6       555 15bit RGB 
VIDEO_PALETTE_YUV422    7       YUV422 capture 
VIDEO_PALETTE_YUYV  8
VIDEO_PALETTE_UYVY  9           The great thing about standards is ... 
VIDEO_PALETTE_YUV420    10
VIDEO_PALETTE_YUV411    11      YUV411 capture 
VIDEO_PALETTE_RAW   12          RAW capture (BT848) 
VIDEO_PALETTE_YUV422P   13      YUV 4:2:2 Planar 
VIDEO_PALETTE_YUV411P   14      YUV 4:1:1 Planar 
VIDEO_PALETTE_YUV420P   15      YUV 4:2:0 Planar 
VIDEO_PALETTE_YUV410P   16      YUV 4:1:0 Planar 
VIDEO_PALETTE_PLANAR    13      start of planar entries 
VIDEO_PALETTE_COMPONENT 7       start of component entries 

*/

MODULE_DESCRIPTION("vscull - a dummy video device, scull based");
MODULE_VERSION("0.1");
MODULE_AUTHOR("Nicola Bonelli");
MODULE_LICENSE("Dual BSD/GPL");

module_param(ndevs, uint, 0);
MODULE_PARM_DESC(ndevs,"number of devices");

module_param(width, uint, 0);
MODULE_PARM_DESC(width,"video width");

module_param(height,uint,0);
MODULE_PARM_DESC(height,"video height");

module_param(fps,uint,0);
MODULE_PARM_DESC(fps,"framerate (fps)");

module_param(palette,uint,0);
MODULE_PARM_DESC(palette,"v4l palette");

module_param(depth,uint,0);
MODULE_PARM_DESC(depth,"bitdepth");

module_param(debug,uint,0);
MODULE_PARM_DESC(debug, "debug level (0-4)");

module_param(framebuf,uint,0);
MODULE_PARM_DESC(debug, "framebuf (0-32)");


#define dprintk(num, format, args...) \
    do { \
        if (debug >= num) \
            printk(format, ##args); \
    } while (0)


struct vscull_device 
{
    struct timeval timer_read;
    struct timeval timer_write;

    struct video_device *vd;    // video_device
    char * frame;  
    int    frame_size;

    struct semaphore    sem;

    struct completion   comp;

    // int users;               // number of processes enabled to open the device concurrently (disabled)
    pid_t pid;                  // pid of the process booking this device (leak reservation: the device could be already opened)

    int fps;
    int width;  
    int height; 
    int depth;  
    
    int brightness;
    int hue; 
    int colour; 
    int contrast;
    int whiteness;

    int palette;

} * vscull_dev[MAXDEVS];            


static char * vscull_alloc_video_frame(struct vscull_device *dev, int w, int h, int d)
{
    dev->width  = w;
    dev->height = h;
    dev->depth  = d;

    vfree(dev->frame);
    dev->frame_size = 0;

    dev->frame = vmalloc( (VIDEOFRAME_SIZE(dev->width, dev->height, dev->depth )/PAGE_SIZE + 1)*PAGE_SIZE );

    if (dev->frame)
        dev->frame_size = (VIDEOFRAME_SIZE(dev->width, dev->height, dev->depth)/PAGE_SIZE + 1)*PAGE_SIZE; // mmap() maps multiple of PAGE_SIZE
    
    printk(KERN_INFO "vscull: alloc_video_frame(%p): w=%d, h=%d, d=%d (size=%d bytes)\n", dev->frame, w, h, d, dev->frame_size); 
    return dev->frame;
}


static void vscull_sleep(int fps, struct timeval * timer)
{
    if (fps > 0 ) {

        struct timeval now;
        int wait_time;
        int msec;

        do_gettimeofday(&now);

        msec = (now.tv_sec-timer->tv_sec)*1000 + (now.tv_usec - timer->tv_usec)/1000;

        wait_time = HZ/fps - msecs_to_jiffies(msec);

        if (wait_time>0) {

            set_current_state(TASK_INTERRUPTIBLE);
            schedule_timeout(wait_time);   

            timer->tv_usec += 1000000/fps;
            if ( timer->tv_usec > 1000000 ) {
                timer->tv_sec  += 1;
                timer->tv_usec -= 1000000;
            }
            return;
        }

        /* we lost the temporal reference */
        do_gettimeofday(timer);
       
    }
}


static int vscull_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg) 
{   
    struct vscull_device * sd = (struct vscull_device *)file->private_data;

    switch(cmd) {

    case VSIOCGPAR: /* specific vscull ioctl */
        {   
            struct vscull_ioctl par;

            if ( down_interruptible(&sd->sem) )
                     return -ERESTARTSYS;

            par.width   = sd->width;
            par.height  = sd->height;
            par.depth   = sd->depth;
            par.palette = sd->palette;
            par.fps     = sd->fps;

            up(&sd->sem);

            if ( copy_to_user((void __user *)arg, &par, sizeof(par)) )
                return -EFAULT;

            dprintk(1, KERN_INFO "vscull: VSIOCGPAR successfully called\n"); 
            return 0;
        }
    case VSIOCSPAR: /* specific vscull ioctl */
        {            
            struct vscull_ioctl par;

            if (copy_from_user(&par, (void __user *)arg, sizeof(par))) 
                return -EFAULT;
            
            if (par.width != sd->width || par.height != sd->height || par.depth != sd->depth) {             
                 
                 if ( down_interruptible(&sd->sem) )
                     return -ERESTARTSYS;

                 if ( !vscull_alloc_video_frame(sd, par.width, par.height, par.depth) ) {
                     printk (KERN_INFO "vscull: Couldn't allocate video frame.\n");
                     up(&sd->sem);
                     return -EFAULT;
                 }
                
                 sd->width = par.width;
                 sd->height = par.height;
                 sd->depth = par.depth;

                 up(&sd->sem);
            }

            sd->palette = par.palette;
            sd->fps = par.fps;

            dprintk(1, KERN_INFO "vscull: VSIOCSPAR successfully called\n"); 
            return 0;
        }  
    case VSIOCGRES: /* vscull specific ioctl */
        {            
            if ( down_interruptible(&sd->sem) )
                     return -ERESTARTSYS;
            
            if (put_user(sd->pid, (pid_t __user *)arg) < 0) {
                up(&sd->sem);
                return -EFAULT;
            }

            up(&sd->sem);
            dprintk(1, KERN_INFO "vscull: VSIOCGRES successfully called\n");
            return 0; 
        }
    case VSIOCSRES: /* vscull specific ioctl */
        {            
            pid_t val;
            if ( down_interruptible(&sd->sem) )
                     return -ERESTARTSYS;

            if ( get_user(val, (pid_t __user *)arg) < 0 ) {
                up(&sd->sem);
                return -EFAULT;
            }
            sd->pid = val; 
            up(&sd->sem);
            dprintk(1, KERN_INFO "vscull: VSIOCSRES successfully called\n");
            return 0; 
        } 
    case VIDIOCGCAP: /* get video capability */
        {
            struct video_capability cap = {
                .type = VID_TYPE_CAPTURE,
                .channels = 1,              /* Num channels */
                .audios = 0,                /* Num audio devices */
                .maxwidth = sd->width,      /* Supported width */
                .maxheight = sd->height,    /* And height */
                .minwidth = sd->width,      /* Supported width */
                .minheight = sd->height     /* And height */
            };
           
            strcpy(cap.name, sd->vd->name);

            if ( copy_to_user((void __user *)arg, &cap, sizeof(cap)) )
                return -EFAULT;
        
            dprintk(1, KERN_INFO "vscull: VIDIOCGCAP successfully called\n"); 
            return 0;
        }
    case VIDIOCGCHAN: /* get video channel informations */
        {
            struct video_channel chan;
            if (copy_from_user(&chan, (void __user *)arg, sizeof(chan)))
                return -EFAULT;

            if (chan.channel != 0)
                return -EINVAL;

            strcpy(chan.name, "vscull_camera");
            chan.tuners = 0;
            chan.flags = 0;
            chan.type = VIDEO_TYPE_CAMERA;
            chan.norm = VIDEO_MODE_AUTO;

            if (copy_to_user((void __user *)arg, &chan, sizeof(chan)))
                return -EFAULT;

            dprintk(1, KERN_INFO "vscull: VIDIOCGCHAN successfully called\n");
            return 0;
        }
    case VIDIOCSCHAN: /* set active channel */
        {
            struct video_channel chan;

            if (copy_from_user(&chan, (void __user *)arg, sizeof(chan)))
                return -EFAULT;

            if (chan.channel != 0)
                return -EINVAL;

            dprintk(1, KERN_INFO "vscull: VIDIOCSCHAN successfully called\n");
            return 0;
        }
    case VIDIOCGPICT: /* get image properties of the picture */
        {
            struct video_picture pict;

            pict.brightness = sd->brightness;
            pict.hue = sd->hue;
            pict.colour = sd->colour;
            pict.contrast = sd->contrast;
            pict.whiteness = sd->whiteness;
            pict.depth = sd->depth;
            pict.palette = sd->palette;

            if (copy_to_user((void __user *)arg, &pict, sizeof(pict)))
                return -EFAULT;

            dprintk(1, KERN_INFO "vscull: VIDIOCGPICT successfully called\n");
            return 0;
        }
    case VIDIOCSPICT: /* change picture settings */
        {
            struct video_picture pict;
            if (copy_from_user(&pict, (void __user *)arg, sizeof(pict)))
                return -EFAULT;

            if (sd->depth < pict.depth) { /* FIXME != is ideal, < allows the application to set a minor depth (fix the mplayer bug) */
                printk(KERN_INFO "vscull: VIDIOCSPICT: setting depth from=%d to=%d (error)\n",sd->depth, pict.depth);
                return -EINVAL;
            }

            if (sd->palette != pict.palette) {
                printk(KERN_INFO "vscull: VIDIOCSPICT: setting palette from=%d[%s] to=%d[%s] (error)\n",sd->palette, 
                                                                                                        PALETTE(sd->palette),
                                                                                                        pict.palette,
                                                                                                        PALETTE(pict.palette));
                return -EINVAL;
            }

            // if(sd->depth != pict.depth) {

            //     if ( down_interruptible(&sd->sem) )
            //         return -ERESTARTSYS;

            //     if ( !vscull_alloc_video_frame(sd, sd->width, sd->height, pict.depth) ) {
            //         printk (KERN_INFO "vscull: Couldn't allocate video frame.\n");
            //         up(&sd->sem);
            //         return -EFAULT;
            //     }

            //     up(&sd->sem);
            // }

            sd->brightness = pict.brightness;
            sd->hue = pict.hue;
            sd->colour = pict.colour;
            sd->contrast = pict.contrast;
            sd->whiteness = pict.whiteness;

            dprintk(1, KERN_INFO "vscull: VIDIOCSPICT successfully called\n");
            return 0;
        }
   case VIDIOCGWIN: /* get current window properties */
        {
            struct video_window vid = {
                .x = 0,
                .y = 0,                 /* Position of window */
                .width = sd->width,
                .height = sd->height,       /* Its size */
                .chromakey = 0,
                .flags = 0,
                .clips = NULL,
                .clipcount = 0
            };

            if (copy_to_user((void __user *)arg,&vid,sizeof(struct video_window)))
                return -EFAULT;

            dprintk(1, KERN_INFO "vscull: VIDIOCGWIN successfully called\n");
            return 0;
        }
    case VIDIOCSWIN: /* set capture area */
        {
            struct video_window win;

            if (copy_from_user(&win, (void __user *)arg, sizeof(win)))
                return -EFAULT;

            // if ((win.clipcount) || (win.clips)) {
            //     dprintk(1, KERN_INFO "vscull: VIDIOCSWIN: setting clipcount(%d) or clips(0x%p) (error)\n", win.clipcount, win.clips);
            //     return -EINVAL;
            // }

            if (win.flags) {
                printk(KERN_INFO "vscull: VIDIOCSWIN: setting flags(%d) (error)\n",win.flags);
                return -EINVAL;
            }

            if (win.width != sd->width || win.height != sd->height ) {
                printk(KERN_INFO "vscull: VIDIOCSWIN: setting width(%d) heigth(%d) (error)\n",win.width, win.height);
                return -EINVAL;
            }

            dprintk(1, KERN_INFO "vscull: VIDIOCSWIN successfully called\n");
            return 0;
        }
    case VIDIOCGMBUF: /* request for memory (mapped) buffer */
        {
            struct video_mbuf mbuf;
            int n;
           
            mbuf.frames = framebuf;    // XXX: we lie here (frames are overlapped)
            mbuf.size = sd->frame_size;
            for(n=0; n < 31; n++)
                mbuf.offsets[n] = 0;

            if (copy_to_user((void __user *)arg, &mbuf, sizeof(mbuf)))
                return -EFAULT;
 
            dprintk(1, KERN_INFO "vscull: VIDIOCGMBUF successfully called\n");
            return 0;
        }
    case VIDIOCCAPTURE:  /* Start, end capture */
        {  
           struct video_mmap *vmap = (struct video_mmap *)arg;

           dprintk(1, KERN_INFO "vscull: VIDIOCCAPTURE {frame=%d geom=%dx%d fmt=%d}\n",
                                vmap->frame, vmap->width, vmap->height,vmap->format);
           return 0;
        }
    case VIDIOCSYNC: /* Sync with mmap grabbing */
        {
            int *frame = (int *)arg;

            wait_for_completion_timeout(&sd->comp, msecs_to_jiffies(1000/sd->fps));

            // vscull_sleep(sd->fps, &sd->timer_read);

            dprintk(2, KERN_INFO "vscull: VIDIOCSYNC successfully called [frame=%d]\n",*frame);
            return 0;
        }
    case VIDIOCMCAPTURE: /* start the capture to a frame */
        {
            struct video_mmap vmap;

            if (copy_from_user(&vmap, (void __user *)arg, sizeof(vmap)))
                return -EFAULT;

            if (vmap.width != sd->width ||
                vmap.height != sd->height) {
                printk(KERN_INFO "vscull: VIDIOCMCAPTURE: pict capture-size incongruent (%d %d)\n", vmap.width, vmap.height);
                return -EINVAL;
            }

            dprintk(2, KERN_INFO "vscull: VIDIOCCAPTURE {frame=%d geom=%dx%d fmt=%d}\n",
                                vmap.frame, vmap.width, vmap.height,vmap.format);
            return 0;
        }
    case  VIDIOCSFBUF:  /* set frame buffer */
        {
            dprintk(1, KERN_INFO "vscull: VIDIOCSFBUF successfully called\n");
            return 0;
        }

    case  VIDIOCGFBUF: /* get frame buffer */
        {
            struct video_buffer vid = {
                .base = 0,
                .height = sd->height,
                .width = sd->width,
                .depth = sd->depth,
                .bytesperline = sd->width*(sd->depth>>3)
            };

            if (copy_to_user((void __user *)arg, &vid, sizeof(vid)))
                return -EFAULT;

            dprintk(1, KERN_INFO "vscull: VIDIOCGFBUF successfully called\n");
            return 0;
        }
    default:
        printk(KERN_INFO "vscull: ioctl 0x%x not implemented for this device\n",cmd); 
        return -ENOTTY;
    }

    return 0; 
}

static int has_reservation(pid_t p)
{
    int i;
    for(i=0; i < MAXDEVS; i++) {
        if (vscull_dev[i] && vscull_dev[i]->pid == p)
            return 1;
    }
    return 0;
}

static inline
int is_reserved(pid_t p, int minor)
{ return vscull_dev[minor]->pid == p; }

/* open the device, in accordance to a leak reservation policy */
static int vscull_open(struct inode *inode, struct file *file) 
{
    int minor = iminor(inode);

    if ( !has_reservation(current->pid) )
        goto avail;

    dprintk(1, KERN_INFO "vscull: pid %d has a reservation...\n", current->pid);

    if ( !is_reserved(current->pid, minor) ) {
        dprintk(1, KERN_INFO "vscull: pid %d has reserved another device (EBUSY)\n", current->pid);
        return -EBUSY;
    }
   
    avail:

    // avail:
    // if ( --vscull_dev[minor]->users < 0) {
    //     vscull_dev[minor]->users++;
    //     return -EBUSY;
    // }

    file->private_data = vscull_dev[minor]; 

    dprintk(1, KERN_INFO "vscull: /dev/video%d successfully opened (pid=%d)\n", minor,current->pid);
    return 0;    
}

/* release the device on the last close() */
static int vscull_release(struct inode *inode, struct file *file)
{
    int minor = iminor(inode);

    dprintk(1, KERN_INFO "vscull: /dev/video%d released.\n", minor);
    return 0;
}


/* clean up reservation on close() is not really required for a leak-reservation strategy */

// static int vscull_flush(struct file *file, fl_owner_t null)
// {
//     struct inode * inode = file->f_dentry->d_inode;
//     int minor = iminor(inode);
// 
//     if ( is_reserved(current->pid, minor) ) {
//         vscull_dev[minor]->pid = 0;
//         dprintk(1, KERN_INFO "vscull: /dev/video%d reservation canceled.\n", minor);
//     }
//     dprintk(1, KERN_INFO "vscull: /dev/video%d flushed.\n", minor);
//     return 0;
// }

static int vscull_mmap(struct file *f, struct vm_area_struct *vma) 
{
    struct vscull_device * sd = (struct vscull_device *)f->private_data;
    struct page *page = NULL;

    unsigned long pos;
    unsigned long start = (unsigned long)(vma->vm_start);
    unsigned long size  = (unsigned long)(vma->vm_end-vma->vm_start);

    if ( size > sd->frame_size ) {
        printk(KERN_INFO "vscull: mmap buffer overrun (memorymap size exceedes the frame size: %lu/%u)\n", size, sd->frame_size);
        return -EINVAL;
    }

    pos = (unsigned long) sd->frame;

    while (size > 0) {
        page = (void *)vmalloc_to_pfn((void *)pos);
        
        if ( remap_pfn_range(vma, start, (unsigned long)page, PAGE_SIZE, PAGE_SHARED))
           return -EAGAIN; 

        start += PAGE_SIZE;
        pos   += PAGE_SIZE;
        size  -= PAGE_SIZE;
    }
    
    dprintk(1, KERN_INFO "vscull: /dev/video%d mmaped (%p)\n", sd->vd->minor, sd->frame);
    return 0;
}


static ssize_t vscull_read(struct file *f, char __user *buf, size_t count, loff_t *ppos)
{
    struct vscull_device * sd = (struct vscull_device *)f->private_data;
        
    if (down_interruptible(&sd->sem))
        return -ERESTARTSYS;

    if (count > sd->frame_size) {
        up(&sd->sem);
        printk(KERN_INFO "vscull: buffer overrun. Can't read %u/%u bytes.\n",(unsigned int)count,sd->frame_size);
        return -EINVAL;
    }

    if (copy_to_user((void __user *)buf, sd->frame, count*sizeof(char))) {
        up(&sd->sem);
        return -EFAULT;
    }
    
    up(&sd->sem);

    /* blocking I/O */

    if (wait_for_completion_interruptible(&sd->comp)) {
        return -EFAULT;
    }

    return count; 
}


static ssize_t vscull_write(struct file *f, const char __user *buf, size_t count, loff_t *ppos)
{
    struct vscull_device * sd = (struct vscull_device *)f->private_data;

    if (down_interruptible(&sd->sem))
        return -ERESTARTSYS;

    if (count > sd->frame_size) {
        up(&sd->sem);
        printk(KERN_INFO "vscull: buffer overrun. Can't write %u/%u bytes.\n",(unsigned int)count, sd->frame_size);
        return -EINVAL;
    }

    /* copy the frame from user */

    if (copy_from_user(sd->frame, (void __user*)buf, count)) {
        up(&sd->sem);
        printk (KERN_INFO "vscull: copy_from_user() error\n");
        return -EFAULT;
    }

    /* uplock the device */
    up(&sd->sem);

    /* signaling the complete condition */
    complete(&sd->comp);

    /* blocking I/O */
    vscull_sleep(sd->fps, &sd->timer_write);

    return count;
}


static struct file_operations vscull_fops = {
            owner:      THIS_MODULE,
            open:       vscull_open,
            release:    vscull_release,
            // flush:      vscull_flush,
            read:       vscull_read,
            mmap:       vscull_mmap,
            write:      vscull_write,
            ioctl:      vscull_ioctl,
            compat_ioctl: v4l_compat_ioctl32,
            llseek:     no_llseek,
};

/* initialize the video_device structure */

static void vscull_video_device_init(struct video_device *vd)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
    vd->type = VID_TYPE_CAPTURE;
#else
    // vd->type = VID_TYPE_CAPTURE;
#endif
    vd->fops = &vscull_fops;
    vd->release = video_device_release;
    vd->minor = -1;
}



static int vscull_dev_release(void)
{
    int i;
    for(i = 0 ; i < MAXDEVS; i++) {
        if (!vscull_dev[i])
            continue;
        if ( vscull_dev[i]->vd->minor != -1)
            video_unregister_device(vscull_dev[i]->vd);
        vfree(vscull_dev[i]->frame);
        kfree(vscull_dev[i]);
    } 

    return 0; 
}

/* init/exit module */

static int __init vscull_init(void)
{
    /* vscull initialization */

    int ret = -ENOMEM, i,n;

    if (ndevs > NDEVS)  
        ndevs = NDEVS; 

    for(i=0, n = 0; i < ndevs; i++) {

        struct vscull_device * dev = 
        dev = kzalloc(sizeof(struct vscull_device), GFP_KERNEL);
        if (dev == NULL)
            goto error;

        /* initialize timer */

        do_gettimeofday(&dev->timer_read);
        do_gettimeofday(&dev->timer_write);

        /* initialize semaphore */
        init_MUTEX(&dev->sem);

        init_completion(&dev->comp);

        // dev->users = 2;       /* 2 players: writer and reader */ 
        dev->pid = 0;         /* not reserved */

        /* global settings */

        dev->fps = fps;           
        dev->width = width;
        dev->height = height;
        dev->depth = depth;
        dev->palette = palette;

        dev->brightness = brightness;
        dev->hue = hue;
        dev->colour = colour;
        dev->contrast = contrast;
        dev->whiteness = whiteness;

        /* alloc frame */
        if ( !vscull_alloc_video_frame(dev, width, height, depth) ) { 
            printk (KERN_INFO "vscull: couldn't allocate video frame.\n");
            goto error;
        }

        /* alloc video_device */
        dev->vd = video_device_alloc();
        if (dev->vd == NULL) {
            printk (KERN_INFO "vscull: couldn't allocate video device.\n");
            goto error; 
        }

        /* initialize video_device */
        vscull_video_device_init(dev->vd);        

        ret = video_register_device(dev->vd, VFL_TYPE_GRABBER, -1);
        if (ret < 0) {
            printk (KERN_INFO "vscull: couldn't register video device.\n");
            goto error;
        }

        snprintf(dev->vd->name,sizeof(dev->vd->name), "vscull_device_%d",dev->vd->minor); // up to 32 chars
       
        printk(KERN_INFO "vscull: '%s' registered.\n", dev->vd->name); 

        if ( dev->vd->minor >= MAXDEVS ) {
            printk(KERN_INFO "vscull: minor descriptor exceeds MAXDEVS\n");
            video_unregister_device(dev->vd);
            vfree(dev->frame);
            kfree(dev);
            continue;
        }

        vscull_dev[dev->vd->minor] = dev; 
        n++;
    }

    printk(KERN_INFO "vscull: %d video device(s) created.\n", n);
    return 0;

error:

    vscull_dev_release();
    printk(KERN_INFO "vscull: error %d while loading vscull driver.\n", ret);    
    return -ENOMEM;
}


static void __exit vscull_exit(void)
{
    vscull_dev_release();
    printk(KERN_INFO "vscull unloaded.\n");
}

module_init(vscull_init);
module_exit(vscull_exit);
