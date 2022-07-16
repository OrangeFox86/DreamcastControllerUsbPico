#ifndef __UTILS_H__
#define __UTILS_H__

#define INT_DIVIDE_CEILING(x,y) (((x) + (y) - 1) / (y))
#define INT_DIVIDE_ROUND(x,y) (((x) + ((y) / 2)) / (y))
// Just for completeness...
#define INT_DIVIDE_FLOOR(x,y) ((x)/(y))

#endif // __UTILS_H__
