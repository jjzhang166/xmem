#include <stdio.h>
#include "xmem.h"

int main(int argc, char *argv[])
{
    char * a,*b,*c,*d,*e,*f,*g,*h,*i;

    //xMemInit();
    a=(char *)xmalloc(1);
    b=(char *)xmalloc(5);
    c=(char *)xmalloc(9);
    d=(char *)xmalloc(17);
    xfree(b);
    xMemInfoDump();
    e=(char *)xmalloc(17);
    f=(char *)xmalloc(80);
    g=(char *)xmalloc(20);
    xfree(f);
    xMemInfoDump();
    h=(char *)xmalloc(30);
    i=(char *)xmalloc(50);
    xfree(a);
    xfree(c);
    xMemInfoDump();
    xfree(d);
    xfree(i);
    xfree(h);
    xfree(g);
    xfree(e);
    xMemInfoDump();

    return 0;
}
