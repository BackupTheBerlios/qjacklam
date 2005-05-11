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
void _Perror(const char * File, int Line);
#define Perror() _Perror(__FILE__, __LINE__)

#ifdef DEBUG_ENABLED

#define rdtscll(val)     __asm__ __volatile__("rdtsc" : "=A" (val))

static inline unsigned get_cycles (void)
{
	unsigned long long ret;

	rdtscll(ret);
	return ret;
}

#define DEBUG(format,args...) \
	fprintf (stderr, PACKAGE ":%5d:%08x %s:%s:%d: " format "\n", getpid(), get_cycles(), __FILE__, __FUNCTION__, __LINE__ , ## args)
#define DEBUGP(format,args...) \
	fprintf (stderr, PACKAGE ":%5d:%08x %s:%s:%d: " format "\n", getpid(), get_cycles(), __FILE__, __PRETTY_FUNCTION__, __LINE__ , ## args)
#else
    #define DEBUG(format,args...)
    #define DEBUGP(format,args...)
#endif
