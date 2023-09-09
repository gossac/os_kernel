/*
 * these functions should be thread safe.
 * It is up to you to rewrite them
 * to make them thread safe.
 *
 */

#include <stdlib.h>
#include <types.h>
#include <stddef.h>
#include <thr_internals.h>
#include <malloc.h>

void *malloc(size_t __size)
{
  mutex_lock(&alloc_mutex);
  void *allocated_space = _malloc(__size);
  mutex_unlock(&alloc_mutex);
  return allocated_space;
}

void *calloc(size_t __nelt, size_t __eltsize)
{
  mutex_lock(&alloc_mutex);
  void *allocated_space = _calloc(__nelt, __eltsize);
  mutex_unlock(&alloc_mutex);
  return allocated_space;
}

void *realloc(void *__buf, size_t __new_size)
{
  mutex_lock(&alloc_mutex);
  void *allocated_space = _realloc(__buf, __new_size);
  mutex_unlock(&alloc_mutex);
  return allocated_space;
}

void free(void *__buf)
{
  mutex_lock(&alloc_mutex);
  _free(__buf);
  mutex_unlock(&alloc_mutex);
}
