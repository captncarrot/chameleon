//====================================================================================
// mostly taken from ccache ccache.h
//====================================================================================

#ifndef CHAMELEON_H
#define CHAMELEON_H

#ifdef __GNUC__
#define ATTR_FORMAT(x, y, z) __attribute__((format (x, y, z)))
#else
#define ATTR_FORMAT(x, y, z)
#endif

#ifndef MYNAME
#define MYNAME "chameleon"
#endif

typedef int bool;
#define true 1;
#define false 0;

#endif // CHAMELEON_H
