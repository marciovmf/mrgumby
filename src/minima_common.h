#ifndef MINIMA_COMMON_H
#define MINIMA_COMMON_H


#define MINIMA_VERSION_MAJOR 0
#define MINIMA_VERSION_PATCH 0
#define MINIMA_VERSION_MINOR 1
  
typedef enum MiType_e
{
  MI_TYPE_INT,
  MI_TYPE_FLOAT,
  MI_TYPE_STRING,
  MI_TYPE_ARRAY,
  MI_TYPE_BOOL,
  MI_TYPE_VOID,
  MI_TYPE_ANY // used of function parameters
} MiType;

#endif //MINIMA_COMMON_H
