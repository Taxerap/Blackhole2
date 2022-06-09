#ifndef BH2_CONTAINER_VECTOR_H
#define BH2_CONTAINER_VECTOR_H

#include <pch.h>

// A very simple and straightforward dynamic vector implementation... Yoinked from another project of mine.

typedef void (*VectorFreeFunc)( void * );
typedef void (*VectorWalkFunc)( void * );
typedef bool (*VectorCmpFunc)( const void *, const void * );

typedef struct Vector
{
    size_t elem_size;
    size_t capacity;
    size_t length;
    VectorFreeFunc free_func;
    void *ptr;
} Vector;

///
/// \brief Create a vector
///
/// Create a vector.
///
/// \param elem_size Size of vector's elements
/// \param free_func Function to call when the vector frees an element
///
/// \return Created vector
///
Vector *
Vector_Create( size_t elem_size, VectorFreeFunc free_func );

///
/// \brief Create a vector
///
/// Create a vector by stack.<BR />
/// <B>The created vector must be freed by \a Vector_DestroyS().</B>
///
/// \param elem_size Size of vector's elements
/// \param free_func Function to call when the vector frees an element
///
/// \return Created vector
///
Vector
Vector_CreateS( size_t elem_size, VectorFreeFunc free_func );

///
/// \brief Get the first element
///
/// Get the first element of the vector.
///
/// \param vec Vector
///
/// \return Pointer to the first element
///
void *
Vector_First( const Vector *vec );

///
/// \brief Get the last element
///
/// Get the last element of the vector.
///
/// \param vec Vector
///
/// \return Pointer to the last element
///
void *
Vector_Last( const Vector *vec );

///
/// \brief Get the indexed element
///
/// Get the pointer to some place in the vector.<BR />
/// If the index is out of bound, it returns the last element of the vector, if any.
///
/// \param vec Vector
/// \param index Index to access
///
/// \return Pointer to the location after index
///
void *
Vector_PtrAt( const Vector *vec, size_t index );

///
/// \brief Find an element in the vector
///
/// Search for an element in the given vector.
///
/// \param vec Vector
/// \param data Data to search
/// \param cmp Comparision function
///
/// \return Pointer to the found element, NULL otherwise.
///
void *
Vector_Find( const Vector *vec, const void *data, VectorCmpFunc cmp );

///
/// \brief Expand the vector
///
/// Expand the vector, usually doubling its capacity unless the length is 0.
///
/// \param vec Vector to expand
///
void
Vector_Expand( Vector *vec );

///
/// \brief Expand the vector
///
/// Expand the vector until its capacity reaches the set limit.
///
/// \param vec Vector to expand
/// \param size Target size for expanding
///
void
Vector_ExpandUntil( Vector *vec, size_t size );

///
/// \brief Walkthrough the vector
///
/// Walk the vector by a function.
///
/// \param vec Vector to walk
/// \param func Function for walking
///
void
Vector_Walk( Vector *vec, VectorWalkFunc func );

///
/// \brief Clear out a vector
///
/// Clear the content of a vector. Its capacity is unchanged.
///
/// \param vec Vector to clear
///
void
Vector_Clear( Vector *vec );

///
/// \brief Shrink the vector
///
/// Shrink the vector to just enough to fit its contents.
///
/// \param vec Vector to shrink
///
void
Vector_ShrinkToFit( Vector *vec );

///
/// \brief Insert element to vector
///
/// Insert an element to vector.<BR />
/// If index is out of bound, it is understood to push the element to the end of vector.
///
/// \param vec Vector to insert
/// \param index Index to insert
/// \param elem Element to insert
///
void
Vector_Insert( Vector *vec, size_t index, const void *elem );

///
/// \brief Replace an element
///
/// Replace an element with a new one, the old one will be overriden.<BR />
/// If index is out of bound, it is understood to push the element to last of vector.
///
/// \param vec Vector to operate
/// \param index Index to replace
/// \param elem Element to replace
///
void
Vector_Replace( Vector *vec, size_t index, const void *elem );

///
/// \brief Delete an element
///
/// Delete an element in index.<BR />
/// If index is out of bound, it does nothing.
///
/// \param vec Vector to operate
/// \param index Index to delete
///
void
Vector_Delete( Vector *vec, size_t index );

///
/// \brief Take an element
///
/// Take an element out of the vector.<BR />
/// If index is out of bound, it is understood to pop the last element of the vector.
/// If ptr_retrieve is NULL the free function will be called.
///
/// \param vec Vector to operate
/// \param index Index to take out
/// \param ptr_retrieve Pointer to retrieve the result
///
void
Vector_Take( Vector *vec, size_t index, void *ptr_retrieve );

///
/// \brief Push an element
///
/// Push an element into the end of vector.
///
/// \param vec Vector to push into
/// \param elem Element to be pushed
///
void
Vector_Push( Vector *vec, const void *elem );

///
/// \brief Pop the last element of vector
///
/// Pop the last element of vector into the pointer.<BR />
/// If ptr_retrieve is NULL the free function will be called.
///
/// \param vec Vector to pop out
/// \param ptr_retrieve Pointer to retrieve the result
///
void
Vector_Pop( Vector *vec, void *ptr_retrieve );

///
/// \brief Push an element to front
///
/// Push an element into the front of vector.
///
/// \param vec Vector to push into
/// \param elem Element to be pushed
///
void
Vector_Push_Front( Vector *vec, const void *elem );

///
/// \brief Pop the first element of vector
///
/// Pop the first element of vector into the pointer.<BR />
/// Whether ptr_retrieve is NULL or not, the first element will be overriden.
///
/// \param vec Vector to pop out
/// \param ptr_retrieve Pointer to retrieve the result
///
void
Vector_Pop_Front( Vector *vec, void *ptr_retrieve );

///
/// \brief Destroy a vector
///
/// Destroy a vector, freeing all its elements.
///
/// \param vec Vector to destroy
///
void
Vector_Destroy( Vector *vec );

///
/// \brief Destroy a vector
///
/// Destroy a vector created by \a Vector_CreateS(), freeing all its elements.
///
/// \param vec Vector to destroy
///
void
Vector_DestroyS( Vector *vec );

#endif // !BH2_CONTAINER_VECTOR_H
