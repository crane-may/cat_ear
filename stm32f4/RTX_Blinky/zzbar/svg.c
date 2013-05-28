/*------------------------------------------------------------------------
 *  Copyright 2009 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the ZBar Bar Code Reader; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zbar
 *------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "svg.h"

static const char svg_head[] = "";

static FILE *svg = NULL;

void svg_open (const char *name, double x, double y, double w, double h)
{
    //svg = fopen(name, "w");
    if(!svg) return;

    /* FIXME calc scaled size */
    fprintf(svg, svg_head, x, y, w, h);
}

void svg_close ()
{
    if(!svg) return;
    fprintf(svg, "</svg>\n");
    fclose(svg);
    svg = NULL;
}

void svg_commentf (const char *format, ...)
{
    va_list args;
	  if(!svg) return;
    fprintf(svg, "<!-- ");
    
    va_start(args, format);
    vfprintf(svg, format, args);
    va_end(args);
    fprintf(svg, " -->\n");
}

void svg_image (const char *name, double width, double height)
{
    if(!svg) return;
    fprintf(svg, "<image width='%g' height='%g' xlink:href='%s'/>\n",
            width, height, name);
}

void svg_group_start (const char *cls,
                      double deg,
                      double sx,
                      double sy,
                      double x,
                      double y)
{
    if(!svg) return;
    fprintf(svg, "<g class='%s'", cls);
    if(sx != 1. || sy != 1 || deg || x || y) {
        fprintf(svg, " transform='");
        if(deg)
            fprintf(svg, "rotate(%g)", deg);
        if(x || y)
            fprintf(svg, "translate(%g,%g)", x, y);
        if(sx != 1. || sy != 1.) {
            if(!sy)
                fprintf(svg, "scale(%g)", sx);
            else
                fprintf(svg, "scale(%g,%g)", sx, sy);
        }
        fprintf(svg, "'");
    }
    fprintf(svg, ">\n");
}

void svg_group_end ()
{
	  if(!svg) return;
    fprintf(svg, "</g>\n");
}

void svg_path_start (const char *cls,
                     double scale,
                     double x,
                     double y)
{
    if(!svg) return;
    fprintf(svg, "<path class='%s'", cls);
    if(scale != 1. || x || y) {
        fprintf(svg, " transform='");
        if(x || y)
            fprintf(svg, "translate(%g,%g)", x, y);
        if(scale != 1.)
            fprintf(svg, "scale(%g)", scale);
        fprintf(svg, "'");
    }
    fprintf(svg, " d='");
}

void svg_path_end ()
{
    if(!svg) return;
    fprintf(svg, "'/>\n");
}

void svg_path_close ()
{
    if(!svg) return;
    fprintf(svg, "z");
}

void svg_path_moveto (svg_absrel_t abs, double x, double y)
{
    if(!svg) return;
    fprintf(svg, " %c%g,%g", (abs) ? 'M' : 'm', x, y);
}

void svg_path_lineto(svg_absrel_t abs, double x, double y)
{
    if(!svg) return;
    if(x && y)
        fprintf(svg, "%c%g,%g", (abs) ? 'L' : 'l', x, y);
    else if(x)
        fprintf(svg, "%c%g", (abs) ? 'H' : 'h', x);
    else if(y)
        fprintf(svg, "%c%g", (abs) ? 'V' : 'v', y);
}
