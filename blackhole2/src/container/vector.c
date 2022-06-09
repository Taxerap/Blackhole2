#include "vector.h"

Vector *
Vector_Create( size_t elem_size, VectorFreeFunc free_func )
{
    Vector *vec = malloc(sizeof(Vector));
    vec->length = 0ULL;
    vec->capacity = 0ULL;
    vec->elem_size = elem_size;
    vec->free_func = free_func;
    vec->ptr = NULL;
    return vec;
}

Vector
Vector_CreateS( size_t elem_size, VectorFreeFunc free_func )
{
    return (Vector)
    {
        .length = 0ULL,
        .capacity = 0ULL,
        .elem_size = elem_size,
        .free_func = free_func,
        .ptr = NULL  
    };
}

inline
void *
Vector_First( const Vector *vec )
{
    return vec->ptr;
}

inline
void *
Vector_Last( const Vector *vec )
{
    if (!vec->length)
        return NULL;

    return (void *)((char *)(vec->ptr) + vec->elem_size * (vec->length - 1ULL));
}

inline
void *
Vector_PtrAt( const Vector *vec, size_t index )
{
    return (void *)((char *)(vec->ptr) + vec->elem_size * index);
}

void *
Vector_Find( const Vector *vec, const void *elem, VectorCmpFunc cmp )
{
    if (!vec->length)
        return NULL;

    char *ptr = vec->ptr;
    for (size_t i = 0ULL; i < vec->length; i++, ptr += vec->elem_size)
        if (cmp((void *)ptr, elem))
            return ptr;

    return NULL;
}

void
Vector_Expand( Vector *vec )
{
    if (!vec->capacity)
    {
        vec->capacity = 1ULL;
        vec->ptr = malloc(vec->elem_size);
    }
    else
    {
        vec->capacity *= 2ULL;
        vec->ptr = realloc(vec->ptr, vec->elem_size * vec->capacity);
    }
}

inline
void
Vector_ExpandUntil( Vector *vec, size_t size )
{
    while (vec->capacity < size)
        Vector_Expand(vec);
}

inline
void
Vector_ShrinkToFit( Vector *vec )
{
    if (vec->capacity > vec->length && vec->length)
    {
        vec->ptr = realloc(vec->ptr, vec->length * vec->elem_size);
        vec->capacity = vec->length;
    }
    else if (vec->capacity && !vec->length)
    {
        free(vec->ptr);
        vec->capacity = 0ULL;
    }
}

void
Vector_Walk( Vector *vec, VectorWalkFunc func )
{
    if (!vec->length)
        return;

    char *ptr = vec->ptr;
    for (size_t i = 0ULL; i < vec->length; i++, ptr += vec->elem_size)
        func((void *)ptr);
}

void
Vector_Clear( Vector *vec )
{
    if (vec->free_func)
        Vector_Walk(vec, vec->free_func);
    vec->length = 0ULL;
}

void
Vector_Insert( Vector *vec, size_t index, const void *elem )
{
    if (index >= vec->length)
        Vector_Push(vec, elem);
    else
    {
        Vector_ExpandUntil(vec, vec->length + 1ULL);
        void *ptr_inserting = Vector_PtrAt(vec, index);
        void *ptr_shifting = Vector_PtrAt(vec, index + 1ULL);
        size_t bytes_to_shift = vec->elem_size * (vec->length - index);
        memmove(ptr_shifting, ptr_inserting, bytes_to_shift);
        memcpy(ptr_inserting, elem, vec->elem_size);
        vec->length++;
    }
}

void
Vector_Replace( Vector *vec, size_t index, const void *elem )
{
    if (index >= vec->length)
        Vector_Push(vec, elem);
    else
    {
        void *ptr = Vector_PtrAt(vec, index);
        if (vec->free_func)
            vec->free_func(ptr);
        memcpy(ptr, elem, vec->elem_size);
    }
}

void
Vector_Delete( Vector *vec, size_t index )
{
    if (index >= vec->length)
        return;

    void *ptr_shifting = Vector_PtrAt(vec, index);
    if (vec->free_func)
        vec->free_func(ptr_shifting);
    void *ptr_data = Vector_PtrAt(vec, index + 1ULL);
    size_t bytes_to_shift = vec->elem_size * (vec->length - index - 1ULL);
    memmove(ptr_shifting, ptr_data, bytes_to_shift);
    vec->length--;
}

void
Vector_Take( Vector *vec, size_t index, void *ptr_retrieve )
{
    if (!vec->length)
        return;
    if (index >= vec->length)
        Vector_Pop(vec, ptr_retrieve);
    else
    {
        void *ptr_shifting = Vector_PtrAt(vec, index);
        void *ptr_data = Vector_PtrAt(vec, index + 1ULL);
        size_t bytes_to_shift = vec->elem_size * (vec->length - index - 1ULL);
        if (ptr_retrieve)
            memcpy(ptr_retrieve, ptr_shifting, vec->elem_size);
        else if (vec->free_func)
            vec->free_func(ptr_shifting);
        memmove(ptr_shifting, ptr_data, bytes_to_shift);
        vec->length--;
    }
}

void
Vector_Push( Vector *vec, const void *elem )
{
    Vector_ExpandUntil(vec, vec->length + 1ULL);
    memcpy(Vector_PtrAt(vec, vec->length), elem, vec->elem_size);
    vec->length++;
}

void
Vector_Pop( Vector *vec, void *ptr_retrieve )
{
    if (!vec->length)
        return;
    if (ptr_retrieve)
        memcpy(ptr_retrieve, Vector_PtrAt(vec, vec->length - 1ULL), vec->elem_size);
    else if (vec->free_func)
        vec->free_func(Vector_PtrAt(vec, vec->length));
    vec->length--;
}

void
Vector_Push_Front( Vector *vec, const void *elem )
{
    Vector_ExpandUntil(vec, vec->length + 1ULL);
    memmove(Vector_PtrAt(vec, 1ULL), vec->ptr, vec->elem_size * vec->length);
    memcpy(vec->ptr, elem, vec->elem_size);
    vec->length++;
}

void
Vector_Pop_Front( Vector *vec, void *ptr_retrieve )
{
    if (!vec->length)
        return;
    if (ptr_retrieve)
        memcpy(ptr_retrieve, vec->ptr, vec->elem_size);
    else if (vec->free_func)
        vec->free_func(vec->ptr);
    vec->length--;
    memmove(vec->ptr, Vector_PtrAt(vec, 1ULL), vec->elem_size * vec->length);
}

void
Vector_Destroy( Vector *vec )
{
    Vector_Clear(vec);
    if (vec->capacity)
        free(vec->ptr);
    free(vec);
}

void
Vector_DestroyS( Vector *vec )
{
    Vector_Clear(vec);
    if (vec->capacity)
        free(vec->ptr);
}
