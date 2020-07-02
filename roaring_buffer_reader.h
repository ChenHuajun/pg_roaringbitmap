#ifndef __ROARING_BUFFER_READER_H__
#define __ROARING_BUFFER_READER_H__

#include "roaring.h"

typedef struct roaring_buffer_s {
	const char *buf;
	size_t buf_len;
    int32_t size;      /* number of containers */
    const uint16_t *keyscards;
    const uint32_t *offsets;
    const char *bitmapOfRunContainers;
	bool hasrun;
	bool keyscards_need_free;
	bool offsets_need_free;
} roaring_buffer_t;



/**
 * Creates a new roaring buffer (from a partable serialized roaringbitmap buffer).
 * Returns NULL if error occurred.
 */
roaring_buffer_t *roaring_buffer_create(const char *buf, size_t buf_len);

/**
 * free roaring buffer
 */
void roaring_buffer_free(const roaring_buffer_t *rb);

/**
 * Get the cardinality of the bitmap (number of elements).
 */
uint64_t roaring_buffer_get_cardinality(const roaring_buffer_t *ra);

/**
 * Check if value x is present
 * Return false if error occurred.
 */
bool roaring_buffer_contains(const roaring_buffer_t *r,
                             uint32_t val,
                              bool *result);

/**
 * Check if all the elements of ra1 are also in ra2.
 * Return false if error occurred.
 */
bool roaring_buffer_is_subset(const roaring_buffer_t *ra1,
                              const roaring_buffer_t *ra2,
                              bool *result);

/**
 * Computes the intersection between two bitmaps and returns new bitmap. The
 * caller is responsible for memory management.
 * Return NULL if error occurred.
 */
roaring_bitmap_t *roaring_buffer_and(const roaring_buffer_t *ra1,
                                     const roaring_buffer_t *ra2);

/**
 * Computes the size of the difference (andnot) between two bitmaps.
 * Return NULL if error occurred.
 */
roaring_bitmap_t *roaring_buffer_andnot(const roaring_buffer_t *x1,
                                        const roaring_buffer_t *x2);

/**
 * Computes the size of the intersection between two bitmaps.
 * Return false if error occurred.
 */
bool roaring_buffer_and_cardinality(const roaring_buffer_t *x1,
                                    const roaring_buffer_t *x2,
                                    uint64_t *result);

/**
 * Computes the size of the union between two bitmaps.
 * Return false if error occurred.
 */
bool roaring_buffer_or_cardinality(const roaring_buffer_t *x1,
                                   const roaring_buffer_t *x2,
                                   uint64_t *result);

/**
 * Computes the size of the difference (andnot) between two bitmaps.
 * Return false if error occurred.
 */
bool roaring_buffer_andnot_cardinality(const roaring_buffer_t *x1,
                                       const roaring_buffer_t *x2,
                                       uint64_t *result);

/**
 * Computes the size of the difference (andnot) between two bitmaps.
 * Return false if error occurred.
 */
bool roaring_buffer_xor_cardinality(const roaring_buffer_t *x1,
                                    const roaring_buffer_t *x2,
                                    uint64_t *result);

/**
 * Computes the Jaccard index between two bitmaps. (Also known as the Tanimoto
 * distance, or the Jaccard similarity coefficient)
 *
 * The Jaccard index is undefined if both bitmaps are empty.
 * Return false if error occurred.
 */
bool roaring_buffer_jaccard_index(const roaring_buffer_t *x1,
                                  const roaring_buffer_t *x2,
                                  double *result);

/**
 * Check whether two bitmaps intersect.
 * Return false if error occurred.
 */
bool roaring_buffer_intersect(const roaring_buffer_t *x1,
                              const roaring_buffer_t *x2,
                              bool *result);

/**
* Returns true if the bitmap is empty (cardinality is zero).
*/
bool roaring_buffer_is_empty(const roaring_buffer_t *rb);


/**
 * Check if the two bitmaps contain the same elements.
 * Return false if error occurred.
 */
bool roaring_buffer_equals(const roaring_buffer_t *rb1,
                           const roaring_buffer_t *rb2,
                           bool *result);

/**
* Count the number of integers that are smaller or equal to x.
* Return false if error occurred.
*/
bool roaring_buffer_rank(const roaring_buffer_t *rb,
                         uint32_t x,
                         uint64_t *reuslt);

/**
* Get the smallest value in the set, or UINT32_MAX if the set is empty.
* Return false if error occurred.
*/
bool roaring_buffer_minimum(const roaring_buffer_t *rb,
                            uint32_t *result);

/**
* Get the greatest value in the set, or 0 if the set is empty.
* Return false if error occurred.
*/
bool roaring_buffer_maximum(const roaring_buffer_t *rb,
                            uint32_t *result);

#endif
