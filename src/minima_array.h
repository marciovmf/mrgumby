/**
 * @file minima_array.h
 * @brief Dynamic array implementation for the Minima language
 *
 * @author marciovmf
 */
#ifndef MINIMA_ARRAY_H
#define MINIMA_ARRAY_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "minima_common.h"

typedef struct MiArray_t MiArray;
typedef struct MiArrayElement_t MiArrayElement;

struct MiArrayElement_t
{
  MiType type;
  union  
  {
    bool b;
    int i;
    double f;
    char* s;
    MiArray* arr;
  } data;
};

struct MiArray_t
{
  MiArrayElement* elements;
  size_t size;
  size_t capacity;
};

/**
 * @brief Creates a new dynamic array with the specified initial capacity.
 *
 * @param initial_capacity The initial number of elements the array can hold.
 * @return Pointer to the newly created dynamic array.
 */
MiArray* mi_array_create(size_t initial_capacity);

/**
 * @brief Resizes the dynamic array, doubling its capacity.
 *
 * @param array Pointer to the dynamic array to be resized.
 */
void mi_array_resize(MiArray* array);

/**
 * @brief Adds an integer element to the dynamic array.
 *
 * @param array Pointer to the dynamic array.
 * @param value Boolean value to add.
 */
void mi_array_add_bool(MiArray* array, bool value);

/**
 * @brief Adds an integer element to the dynamic array.
 *
 * @param array Pointer to the dynamic array.
 * @param value Integer value to add.
 */
void mi_array_add_int(MiArray* array, int value);

/**
 * @brief Adds a float element to the dynamic array.
 *
 * @param array Pointer to the dynamic array.
 * @param value Float value to add.
 */
void mi_array_add_float(MiArray* array, float value);

/**
 * @brief Adds a string element to the dynamic array.
 *
 * @param array Pointer to the dynamic array.
 * @param value String value to add (deep copied into the array).
 */
void mi_array_add_string(MiArray* array, const char* value);

/**
 * @brief Adds a sub-array element to the dynamic array (for multidimensional arrays).
 *
 * @param array Pointer to the dynamic array.
 * @param sub_array Pointer to the sub-array to add.
 */
void mi_array_add_array(MiArray* array, MiArray* sub_array);

/**
 * @brief Destroys the dynamic array and frees all associated memory.
 *
 * @param array Pointer to the dynamic array to destroy.
 */
void mi_array_destroy(MiArray* array);

/**
 * @brief Prints the contents of the dynamic array (for debugging purposes).
 *
 * @param array Pointer to the dynamic array to print.
 */
void mi_array_print(MiArray* array);

#ifdef __cplusplus
extern {
#endif

#endif //MINIMA_ARRAY_H

