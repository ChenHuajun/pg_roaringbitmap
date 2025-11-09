#ifndef __ROARING64_BUFFER_READER_H__
#define __ROARING64_BUFFER_READER_H__

#include "roaring_buffer_reader.h"

typedef struct roaring64_buffer_s {
	const char *buf;
	size_t buf_len;
	int32_t size;           /* number of 64-bit buckets (each holds a 32-bit roaring bitmap) */
	const uint32_t *keys;   /* upper 32 bits per bucket */
	const roaring_buffer_t **rb_readers; /* buffer reader for 32-bit roaring bitmaps */
} roaring64_buffer_t;


/**
 * Creates a new 64-bit roaring buffer reader (from a portable serialized 64-bit roaringbitmap buffer).
 * The caller is responsible for freeing the result. 
 * Returns NULL if error occurred.
 */
roaring64_buffer_t *roaring64_buffer_create(const char *buf, size_t buf_len);

/**
 * free 64-bit roaring buffer reader
 */
void roaring64_buffer_free(const roaring64_buffer_t *rb);

/**
 * Get the cardinality of the bitmap (number of elements).
 */
uint64_t roaring64_buffer_get_cardinality(const roaring64_buffer_t *rb);

/**
 * Check if value x is present
 * Return false if error occurred.
 */
bool roaring64_buffer_contains(const roaring64_buffer_t *r,
							  uint64_t val,
							  bool *result);

/**
 * Check if all the elements of ra1 are also in ra2.
 * Return false if error occurred.
 */
bool roaring64_buffer_is_subset(const roaring64_buffer_t *ra1,
								  const roaring64_buffer_t *ra2,
								  bool *result);

/**
 * Computes the intersection between two bitmaps.
 * Return false if error occurred.
 */
bool roaring64_buffer_and_cardinality(const roaring64_buffer_t *x1,
										  const roaring64_buffer_t *x2,
										  uint64_t *result);

/**
 * Computes the size of the union between two bitmaps.
 * Return false if error occurred.
 */
bool roaring64_buffer_or_cardinality(const roaring64_buffer_t *x1,
										 const roaring64_buffer_t *x2,
										 uint64_t *result);

/**
 * Computes the size of the difference (andnot) between two bitmaps.
 * Return false if error occurred.
 */
bool roaring64_buffer_andnot_cardinality(const roaring64_buffer_t *x1,
											 const roaring64_buffer_t *x2,
											 uint64_t *result);

/**
 * Computes the size of the symmetric difference between two bitmaps.
 * Return false if error occurred.
 */
bool roaring64_buffer_xor_cardinality(const roaring64_buffer_t *x1,
										  const roaring64_buffer_t *x2,
										  uint64_t *result);

/**
 * Computes the Jaccard index between two bitmaps.
 * Return false if error occurred.
 */
bool roaring64_buffer_jaccard_index(const roaring64_buffer_t *x1,
										 const roaring64_buffer_t *x2,
										 double *result);

/**
 * Check whether two bitmaps intersect.
 * Return false if error occurred.
 */
bool roaring64_buffer_intersect(const roaring64_buffer_t *x1,
								   const roaring64_buffer_t *x2,
								   bool *result);

/**
* Returns true if the bitmap is empty (cardinality is zero).
*/
bool roaring64_buffer_is_empty(const roaring64_buffer_t *rb);

/**
 * Check if the two bitmaps contain the same elements.
 * Return false if error occurred.
 */
bool roaring64_buffer_equals(const roaring64_buffer_t *rb1,
							  const roaring64_buffer_t *rb2,
							  bool *result);

/**
 * Count the number of integers that are smaller or equal to x.
 * Return false if error occurred.
 */
bool roaring64_buffer_rank(const roaring64_buffer_t *rb,
							 uint64_t x,
							 uint64_t *result);

/**
 * Get the smallest value in the set, or UINT64_MAX if the set is empty.
 * Return false if error occurred.
 */
bool roaring64_buffer_minimum(const roaring64_buffer_t *rb,
							   uint64_t *result);

/**
 * Get the greatest value in the set, or 0 if the set is empty.
 * Return false if error occurred.
 */
bool roaring64_buffer_maximum(const roaring64_buffer_t *rb,
							   uint64_t *result);

#endif


