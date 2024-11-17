#include "minima_array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#define strdup _strdup
#endif

MiArray* mi_array_create(size_t initial_capacity)
{
  MiArray* array = (MiArray*)malloc(sizeof(MiArray));
  array->elements = (MiArrayElement*)malloc(sizeof(MiArrayElement) * initial_capacity);
  array->size = 0;
  array->capacity = initial_capacity;
  return array;
}


void mi_array_resize(MiArray* array)
{
  array->capacity *= 2;
  array->elements = (MiArrayElement*)realloc(array->elements, sizeof(MiArrayElement) * array->capacity);
}


void mi_array_add_bool(MiArray* array, bool value)
{
  if (array->size == array->capacity)
  {
    mi_array_resize(array);
  }
  array->elements[array->size].type = MI_TYPE_BOOL;
  array->elements[array->size].data.b = value;
  array->size++;
}

void mi_array_add_int(MiArray* array, int value)
{
  if (array->size == array->capacity)
  {
    mi_array_resize(array);
  }
  array->elements[array->size].type = MI_TYPE_INT;
  array->elements[array->size].data.i = value;
  array->size++;
}


void mi_array_add_float(MiArray* array, float value)
{
  if (array->size == array->capacity)
  {
    mi_array_resize(array);
  }
  array->elements[array->size].type = MI_TYPE_FLOAT;
  array->elements[array->size].data.f = value;
  array->size++;
}


void mi_array_add_string(MiArray* array, const char* value)
{
  if (array->size == array->capacity)
  {
    mi_array_resize(array);
  }
  array->elements[array->size].type = MI_TYPE_STRING;
  array->elements[array->size].data.s = strdup(value); //TODO: Should I use a reference here ?
  array->size++;
}


void mi_array_add_array(MiArray* array, MiArray* sub_array)
{
  if (array->size == array->capacity)
  {
    mi_array_resize(array);
  }
  array->elements[array->size].type = MI_TYPE_ARRAY;
  array->elements[array->size].data.arr = sub_array;
  array->size++;
}


void mi_array_destroy(MiArray* array)
{
  for (size_t i = 0; i < array->size; i++)
  {
    if (array->elements[i].type == MI_TYPE_STRING)
    {
      free(array->elements[i].data.s);
    }
    else if (array->elements[i].type == MI_TYPE_ARRAY)
    {
      mi_array_destroy(array->elements[i].data.arr);
    }
  }
  free(array->elements);
  free(array);
}


void mi_array_print(MiArray* array)
{
  printf("[ ");
  for (size_t i = 0; i < array->size; i++)
  {
    char* comma =  (i < array->size - 1) ? ", " : "";
    switch (array->elements[i].type)
    {
      case MI_TYPE_BOOL:
        printf("%s%s", array->elements[i].data.b ? "true" : "false", comma);
        break;
      case MI_TYPE_INT:
        printf("%d%s", array->elements[i].data.i, comma);
        break;
      case MI_TYPE_FLOAT:
        printf("%.4f%s", array->elements[i].data.f, comma);
        break;
      case MI_TYPE_STRING:
        printf("'%s'%s", array->elements[i].data.s, comma);
        break;
      case MI_TYPE_ARRAY:
        mi_array_print(array->elements[i].data.arr);
        printf("%s",comma);
        break;
      default:
        printf("<UNKNOWN>: %d%s", array->elements[i].type, comma);
    }
  }
  printf(" ]");
}
