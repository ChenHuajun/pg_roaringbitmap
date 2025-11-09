/**
 * 64-bit roaring buffer reader, leveraging 32-bit roaring buffer reader for leaf bitmaps.
 */

#include "roaring64_buffer_reader.h"


static inline uint32_t rb64_get_key_at_index(const roaring64_buffer_t *rb, uint32_t i) {
    return rb->keys[i];
}

static inline const roaring_buffer_t *rb64_get_reader_at_index(const roaring64_buffer_t *rb, uint32_t i) {
    return rb->rb_readers[i];
}

/**
 *  Good old binary search.
 *  Assumes that array is sorted, has logarithmic complexity.
 *  if the result is x, then:
 *     if ( x>0 )  you have array[x] = ikey
 *     if ( x<0 ) then inserting ikey at position -x-1 in array (insuring that array[-x-1]=ikey)
 *                   keeps the array sorted.
 */
static inline int32_t rb64_keys_binary_search(const uint32_t *array, int32_t size, uint32_t key) {
    int32_t low = 0;
    int32_t high = size - 1;
    while (low <= high) {
        int32_t middleIndex = (low + high) >> 1;
        uint32_t middleValue = array[middleIndex];
        if (middleValue < key) {
            low = middleIndex + 1;
        } else if (middleValue > key) {
            high = middleIndex - 1;
        } else {
            return middleIndex;
        }
    }
    return -(low + 1);
}

/**
 * Galloping search
 * Assumes that array is sorted, has logarithmic complexity.
 * if the result is x, then if x = length, you have that all values in array between pos and length
 *    are smaller than min.
 * otherwise returns the first index x such that array[x] >= min.
 */
static inline int32_t rb64_keys_advance_until(const uint32_t *array, int32_t pos, int32_t length, uint32_t min) {
    int32_t lower = pos + 1;
    if ((lower >= length) || (array[lower] >= min)) {
        return lower;
    }
    int32_t spansize = 1;
    while ((lower + spansize < length) && (array[lower + spansize] < min)) {
        spansize <<= 1;
    }
    int32_t upper = (lower + spansize < length) ? lower + spansize : length - 1;
    if (array[upper] == min) {
        return upper;
    }
    if (array[upper] < min) {
        return length;
    }
    lower += (spansize >> 1);
    int32_t mid = 0;
    while (lower + 1 != upper) {
        mid = (lower + upper) >> 1;
        if (array[mid] == min) {
            return mid;
        } else if (array[mid] < min) {
            lower = mid;
        } else {
            upper = mid;
        }
    }
    return upper;
}

/**
 * Creates a new 64-bit roaring buffer reader (from a portable serialized 64-bit roaringbitmap buffer).
 * The caller is responsible for freeing the result. 
 * Returns NULL if error occurred.
 */
roaring64_buffer_t *roaring64_buffer_create(const char *buf, size_t buf_len){
    // https://github.com/RoaringBitmap/RoaringFormatSpec#extension-for-64-bit-implementations
    if (buf == NULL) {
        return NULL;
    }
    size_t read_bytes = 0;
    const char *cur = buf;

    // Read as uint64 the distinct number of "buckets", where a bucket is
    // defined as the most significant 32 bits of an element.
    uint64_t num_buckets;
    if (read_bytes + sizeof(num_buckets) > buf_len) {
        return NULL;
    }
    memcpy(&num_buckets, cur, sizeof(num_buckets));
    cur += sizeof(num_buckets);
    read_bytes += sizeof(num_buckets);

    // Buckets should be 32 bits with 4 bits of zero padding.
    if (num_buckets > UINT32_MAX) {
        return NULL;
    }

    if(num_buckets == 0) {
        roaring64_buffer_t *ans = (roaring64_buffer_t *)roaring_malloc(sizeof(roaring64_buffer_t));
        if(ans == NULL)
            return NULL;
        ans->buf = buf;
        ans->buf_len = read_bytes;
        ans->size = 0;
        ans->keys = NULL;
        ans->rb_readers = NULL;
        return ans;
    }

    uint32_t *keys = (uint32_t *)roaring_malloc(sizeof(uint32_t) * num_buckets);
    const roaring_buffer_t **rb_readers = (const roaring_buffer_t **)roaring_malloc(sizeof(roaring_buffer_t *) * num_buckets);
    if(keys == NULL || rb_readers == NULL){
        if(keys) roaring_free(keys);
        if(rb_readers) roaring_free((void *)rb_readers);
        return NULL;
    }

    // Iterate through buckets ordered by increasing keys.
    int64_t previous_key = -1;
    for(uint64_t i = 0; i < num_buckets; ++i){
        // Read as uint32 the most significant 32 bits of the bucket.
        if(read_bytes + sizeof(uint32_t) > buf_len){
            roaring_free(keys);
            roaring_free((void *)rb_readers);
            return NULL;
        }
        uint32_t key;
        memcpy(&key, cur, sizeof(uint32_t));
        cur += sizeof(uint32_t);
        read_bytes += sizeof(uint32_t);

        // High 32 bits must be strictly increasing.
        if (key <= previous_key) {
            roaring_free(keys);
            roaring_free((void *)rb_readers);
            return NULL;
        }
        previous_key = key;

        // Read the 32-bit Roaring bitmaps representing the least
        // significant bits of a set of elements.
        size_t remain = (size_t)(buf_len - read_bytes);
        roaring_buffer_t *rb_reader = roaring_buffer_create(cur, remain);
        if(rb_reader == NULL){
            // Free previously created rb_readers before returning
            for(uint64_t j = 0; j < i; ++j){
                if(rb_readers[j]) roaring_buffer_free(rb_readers[j]);
            }
            roaring_free(keys);
            roaring_free((void *)rb_readers);
            return NULL;
        }
        size_t rb_size = rb_reader->buf_len;
        // Check if the buffer size is valid
        if(rb_size > remain){
            roaring_buffer_free(rb_reader);
            // Free previously created rb_readers before returning
            for(uint64_t j = 0; j < i; ++j){
                if(rb_readers[j]) roaring_buffer_free(rb_readers[j]);
            }
            roaring_free(keys);
            roaring_free((void *)rb_readers);
            return NULL;
        }
        cur += rb_size;
        read_bytes += rb_size;

        keys[i] = key;
        rb_readers[i] = rb_reader;

    }

    roaring64_buffer_t *ans = (roaring64_buffer_t *)roaring_malloc(sizeof(roaring64_buffer_t));
    if(ans == NULL){
        // Free all created rb_readers before returning
        for(uint64_t i = 0; i < num_buckets; ++i){
            if(rb_readers[i]) roaring_buffer_free(rb_readers[i]);
        }
        roaring_free(keys);
        roaring_free((void *)rb_readers);
        return NULL;
    }
    ans->buf = buf;
    ans->buf_len = read_bytes;
    ans->size = num_buckets;
    ans->keys = keys;
    ans->rb_readers = rb_readers;
    return ans;
}

/**
 * free 64-bit roaring buffer reader
 */
void roaring64_buffer_free(const roaring64_buffer_t *rb) {
    if(!rb) return;
    if(rb->rb_readers){
        for(int i = 0; i < rb->size; ++i){
            if(rb->rb_readers[i]) roaring_buffer_free(rb->rb_readers[i]);
        }
        roaring_free((void *)rb->rb_readers);
    }
    if(rb->keys) roaring_free((void *)rb->keys);
    roaring_free((void *)rb);
}

/**
 * Get the cardinality of the bitmap (number of elements).
 */
uint64_t roaring64_buffer_get_cardinality(const roaring64_buffer_t *rb) {
    uint64_t total = 0;
    for (int i = 0; i < rb->size; ++i) {
        total += roaring_buffer_get_cardinality(rb->rb_readers[i]);
    }
    return total;
}

/**
 * Check if value x is present
 * Return false if error occurred.
 */
bool roaring64_buffer_contains(const roaring64_buffer_t *rb,
                              uint64_t val,
                              bool *result) {
    uint32_t high = (uint32_t)(val >> 32);
    int32_t idx = rb64_keys_binary_search(rb->keys, rb->size, high);
    if(idx < 0){
        *result = false;
        return true;
    }
    bool ans = false;
    bool ok = roaring_buffer_contains(rb64_get_reader_at_index(rb, idx), (uint32_t)(val & 0xFFFFFFFFu), &ans);
    if(!ok)
        return false;
    *result = ans;
    return true;
}

/**
 * Check if all the elements of ra1 are also in ra2.
 * Return false if error occurred.
 */
bool roaring64_buffer_is_subset(const roaring64_buffer_t *ra1,
                               const roaring64_buffer_t *ra2,
                               bool *result) {
    const int length1 = ra1->size,
              length2 = ra2->size;
    int pos1 = 0, pos2 = 0;
    while (pos1 < length1 && pos2 < length2) {
        const uint32_t s1 = rb64_get_key_at_index(ra1, pos1);
        const uint32_t s2 = rb64_get_key_at_index(ra2, pos2);
        if (s1 == s2) {
            const roaring_buffer_t *c1 = rb64_get_reader_at_index(ra1, pos1);
            if(c1 == NULL)
                return false;
            const roaring_buffer_t *c2 = rb64_get_reader_at_index(ra2, pos2);
            if(c2 == NULL)
                return false;
            bool subset = false;
            bool ok = roaring_buffer_is_subset(c1, c2, &subset);
            if(!ok)
                return false;
            if(!subset){
                *result = false;
                return true;
            }
            ++pos1; ++pos2;
        } else if (s1 < s2) {
            *result = false;
            return true;
        } else {
            pos2 = rb64_keys_advance_until(ra2->keys, pos2, length2, s1);
        }
    }
    *result = (pos1 == length1);
    return true;
}

/**
 * Computes the intersection between two bitmaps.
 * Return false if error occurred.
 */
bool roaring64_buffer_and_cardinality(const roaring64_buffer_t *x1,
                                     const roaring64_buffer_t *x2,
                                     uint64_t *result) {
    const int length1 = x1->size,
              length2 = x2->size;
    uint64_t cardinality = 0;
    int pos1 = 0, pos2 = 0;
    while (pos1 < length1 && pos2 < length2) {
        const uint32_t s1 = rb64_get_key_at_index(x1, pos1);
        const uint32_t s2 = rb64_get_key_at_index(x2, pos2);
        if (s1 == s2) {
            const roaring_buffer_t *c1 = rb64_get_reader_at_index(x1, pos1);
            if(c1 == NULL)
                return false;
            const roaring_buffer_t *c2 = rb64_get_reader_at_index(x2, pos2);
            if(c2 == NULL)
                return false;
            uint64_t card = 0;
            bool ok = roaring_buffer_and_cardinality(c1, c2, &card);
            if(!ok)
                return false;
            cardinality += card;
            ++pos1; ++pos2;
        } else if (s1 < s2) {
            pos1 = rb64_keys_advance_until(x1->keys, pos1, length1, s2);
        } else {
            pos2 = rb64_keys_advance_until(x2->keys, pos2, length2, s1);
        }
    }
    *result = cardinality;
    return true;
}

/**
 * Computes the size of the union between two bitmaps.
 * Return false if error occurred.
 */
bool roaring64_buffer_or_cardinality(const roaring64_buffer_t *x1,
                                    const roaring64_buffer_t *x2,
                                    uint64_t *result) {
    bool ok;
    uint64_t inter;
    const uint64_t c1 = roaring64_buffer_get_cardinality(x1);
    const uint64_t c2 = roaring64_buffer_get_cardinality(x2);
    ok = roaring64_buffer_and_cardinality(x1, x2, &inter);
    if(!ok)
        return false;
    *result = c1 + c2 - inter;
    return true;
}

/**
 * Computes the size of the difference (andnot) between two bitmaps.
 * Return false if error occurred.
 */
bool roaring64_buffer_andnot_cardinality(const roaring64_buffer_t *x1,
                                        const roaring64_buffer_t *x2,
                                        uint64_t *result) {
    bool ok;
    uint64_t inter;
    const uint64_t c1 = roaring64_buffer_get_cardinality(x1);
    ok = roaring64_buffer_and_cardinality(x1, x2, &inter);
    if(!ok)
        return false;
    *result = c1 - inter;
    return true;
}

/**
 * Computes the size of the symmetric difference between two bitmaps.
 * Return false if error occurred.
 */
bool roaring64_buffer_xor_cardinality(const roaring64_buffer_t *x1,
                                     const roaring64_buffer_t *x2,
                                     uint64_t *result) {
    bool ok;
    uint64_t inter;
    const uint64_t c1 = roaring64_buffer_get_cardinality(x1);
    const uint64_t c2 = roaring64_buffer_get_cardinality(x2);
    ok = roaring64_buffer_and_cardinality(x1, x2, &inter);
    if(!ok)
        return false;
    *result = c1 + c2 - 2 * inter;
    return true;
}

/**
 * Computes the Jaccard index between two bitmaps.
 * Return false if error occurred.
 */
bool roaring64_buffer_jaccard_index(const roaring64_buffer_t *x1,
                                   const roaring64_buffer_t *x2,
                                   double *result) {
    bool ok;
    uint64_t inter;
    const uint64_t c1 = roaring64_buffer_get_cardinality(x1);
    const uint64_t c2 = roaring64_buffer_get_cardinality(x2);
    ok = roaring64_buffer_and_cardinality(x1, x2, &inter);
    if(!ok)
        return false;
    *result = (double)inter / (double)(c1 + c2 - inter);
    return true;
}

/**
 * Check whether two bitmaps intersect.
 * Return false if error occurred.
 */
bool roaring64_buffer_intersect(const roaring64_buffer_t *x1,
                               const roaring64_buffer_t *x2,
                               bool *result) {
    const int length1 = x1->size,
              length2 = x2->size;
    int pos1 = 0, pos2 = 0;
    while (pos1 < length1 && pos2 < length2) {
        const uint32_t s1 = rb64_get_key_at_index(x1, pos1);
        const uint32_t s2 = rb64_get_key_at_index(x2, pos2);
        if (s1 == s2) {
            const roaring_buffer_t *c1 = rb64_get_reader_at_index(x1, pos1);
            if(c1 == NULL)
                return false;
            const roaring_buffer_t *c2 = rb64_get_reader_at_index(x2, pos2);
            if(c2 == NULL)
                return false;
            bool intersect = false;
            bool ok = roaring_buffer_intersect(c1, c2, &intersect);
            if(!ok)
                return false;
            if(intersect){
                *result = true;
                return true;
            }
            ++pos1; ++pos2;
        } else if (s1 < s2) {
            pos1 = rb64_keys_advance_until(x1->keys, pos1, length1, s2);
        } else {
            pos2 = rb64_keys_advance_until(x2->keys, pos2, length2, s1);
        }
    }
    *result = false;
    return true;
}

/**
* Returns true if the bitmap is empty (cardinality is zero).
*/
bool roaring64_buffer_is_empty(const roaring64_buffer_t *rb) {
    return rb->size == 0;
}

/**
 * Check if the two bitmaps contain the same elements.
 * Return false if error occurred.
 */
bool roaring64_buffer_equals(const roaring64_buffer_t *rb1,
                            const roaring64_buffer_t *rb2,
                            bool *result) {
    if (rb1->size != rb2->size) { *result = false; return true; }
    for (int i = 0; i < rb1->size; ++i) {
        if (rb64_get_key_at_index(rb1, i) != rb64_get_key_at_index(rb2, i)) {
            *result = false;
            return true; 
        }
    }
    for (int i = 0; i < rb1->size; ++i) {
        const roaring_buffer_t *c1 = rb64_get_reader_at_index(rb1, i);
        if(c1 == NULL)
            return false;
        const roaring_buffer_t *c2 = rb64_get_reader_at_index(rb2, i);
        if(c2 == NULL)
            return false;
        bool areequal = false;
        bool ok = roaring_buffer_equals(c1, c2, &areequal);
        if(!ok)
            return false;
        if (!areequal) {
            *result = false;
            return true;
        }
    }
    *result = true;
    return true;
}

/**
 * Count the number of integers that are smaller or equal to x.
 * Return false if error occurred.
 */
bool roaring64_buffer_rank(const roaring64_buffer_t *rb,
                          uint64_t x,
                          uint64_t *result) {
    uint32_t xhigh = (uint32_t)(x >> 32);
    *result = 0;
    for (int i = 0; i < rb->size; i++) {
        uint32_t key = rb64_get_key_at_index(rb, i);
        if (xhigh < key)
            return true;
        const roaring_buffer_t *c = rb64_get_reader_at_index(rb, (uint32_t)i);
        if(c == NULL)
            return false;
        if (xhigh == key) {
            uint64_t r = 0;
            bool ok = roaring_buffer_rank(c, (uint32_t)(x & 0xFFFFFFFFu), &r);
            if(!ok)
                return false;
            *result += r;
            return true;
        } else {
            *result += roaring_buffer_get_cardinality(c);
        }
    }
    return true;
}

/**
 * Get the smallest value in the set, or UINT64_MAX if the set is empty.
 * Return false if error occurred.
 */
bool roaring64_buffer_minimum(const roaring64_buffer_t *rb,
                             uint64_t *result) {
    if (rb->size > 0) {
        const roaring_buffer_t *c = rb64_get_reader_at_index(rb, 0);
        if(c == NULL)
            return false;
        uint32_t low = 0;
        bool ok = roaring_buffer_minimum(c, &low);
        if(!ok)
            return false;
        *result = (((uint64_t)rb64_get_key_at_index(rb, 0)) << 32) | (uint64_t)low;
    } else {
        *result = UINT64_MAX;
    }
    return true;
}

/**
 * Get the greatest value in the set, or 0 if the set is empty.
 * Return false if error occurred.
 */
bool roaring64_buffer_maximum(const roaring64_buffer_t *rb,
                             uint64_t *result) {
    if (rb->size > 0) {
        int i = rb->size - 1;
        const roaring_buffer_t *c = rb64_get_reader_at_index(rb, (uint32_t)i);
        if(c == NULL)
            return false;
        uint32_t low = 0;
        bool ok = roaring_buffer_maximum(c, &low);
        if(!ok)
            return false;
        *result = (((uint64_t)rb64_get_key_at_index(rb, i)) << 32) | (uint64_t)low;
    } else {
        *result = 0;
    }
    return true;
}


