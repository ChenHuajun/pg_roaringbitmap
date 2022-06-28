/**
 * For some roaringbitmap operation, when the input is serialized binary data,
 * maybe only deserializing part of the data is enough. This file provides some
 * functions that support direct reading of serialized binary data to improve
 * performance in certain scenarios.
 */

#include "roaring_buffer_reader.h"

static inline int32_t keyscardsBinarySearch(const uint16_t *array, int32_t size, uint16_t ikey);
static inline int32_t keyscardsAdvanceUntil(const uint16_t *array, int32_t pos, int32_t length, uint16_t min);
static inline int32_t rb_get_size(const roaring_buffer_t *rb);
static inline uint16_t rb_get_key_at_index(const roaring_buffer_t *rb, uint16_t i);
static void *rb_get_container_at_index(const roaring_buffer_t *rb, uint16_t i, uint8_t *typecode);
static inline int32_t rb_get_index(const roaring_buffer_t *rb, uint16_t x);
static inline int32_t rb_advance_until(const roaring_buffer_t *rb, uint16_t x, int32_t pos);
static bool rb_append_copy_range(roaring_array_t *ra, const roaring_buffer_t *sa,
                                 int32_t start_index, int32_t end_index);

/**
 *  Good old binary search.
 *  Assumes that array is sorted, has logarithmic complexity.
 *  if the result is x, then:
 *     if ( x>0 )  you have array[x] = ikey
 *     if ( x<0 ) then inserting ikey at position -x-1 in array (insuring that array[-x-1]=ikey)
 *                   keys the array sorted.
 */
static inline int32_t keyscardsBinarySearch(const uint16_t *array, int32_t size,
                            uint16_t ikey) {
    int32_t low = 0;
    int32_t high = size - 1;
    while (low <= high) {
        int32_t middleIndex = (low + high) >> 1;
        uint16_t middleValue = array[middleIndex << 1];
        if (middleValue < ikey) {
            low = middleIndex + 1;
        } else if (middleValue > ikey) {
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
static inline int32_t keyscardsAdvanceUntil(const uint16_t *array, int32_t pos,
                                   int32_t length, uint16_t min) {
    int32_t lower = pos + 1;

    if ((lower >= length) || (array[lower << 1] >= min)) {
        return lower;
    }

    int32_t spansize = 1;

    while ((lower + spansize < length) && (array[(lower + spansize) << 1] < min)) {
        spansize <<= 1;
    }
    int32_t upper = (lower + spansize < length) ? lower + spansize : length - 1;

    if (array[upper << 1] == min) {
        return upper;
    }
    if (array[upper << 1] < min) {
        // means
        // array
        // has no
        // item
        // >= min
        // pos = array.length;
        return length;
    }

    // we know that the next-smallest span was too small
    lower += (spansize >> 1);

    int32_t mid = 0;
    while (lower + 1 != upper) {
        mid = (lower + upper) >> 1;
        if (array[mid << 1] == min) {
            return mid;
        } else if (array[mid << 1] < min) {
            lower = mid;
        } else {
            upper = mid;
        }
    }
    return upper;
}

/**
 * Get the number of containers
 */
static inline int32_t rb_get_size(const roaring_buffer_t *rb) { return rb->size; }

/**
 * Retrieves the key at index i
 */
static inline uint16_t rb_get_key_at_index(const roaring_buffer_t *rb, uint16_t i) {
    return rb->keyscards[i * 2];
}

/**
 * Retrieves the container at index i, filling in the typecode
 * Return NULL if error occurred.
 */
static void *rb_get_container_at_index(const roaring_buffer_t *rb, uint16_t i,
                                       uint8_t *typecode)
{
	if(i < 0 || i >= rb->size) {
	  fprintf(stderr, "i out of the range.\n");
	  return NULL;
	}

	size_t readbytes = rb->offsets[i];
	void *answer = NULL;
	const char *buf = rb->buf + rb->offsets[i];
	uint32_t thiscard = rb->keyscards[2*i+1] + 1;
	bool isbitmap = (thiscard > DEFAULT_MAX_SIZE);
	bool isrun = false;
	if(rb->hasrun) {
	  if((rb->bitmapOfRunContainers[i / 8] & (1 << (i % 8))) != 0) {
		isbitmap = false;
		isrun = true;
	  }
	}
	if (isbitmap) {
		// we check that the read is allowed
		size_t containersize = BITSET_CONTAINER_SIZE_IN_WORDS * sizeof(uint64_t);
		readbytes += containersize;
		if(readbytes > rb->buf_len) {
		  fprintf(stderr, "Running out of bytes while reading a bitset container.\n");
		  return NULL;
		}

		// it is now safe to read
		bitset_container_t *c = bitset_container_create();
		if(c == NULL) {// memory allocation failure
		  fprintf(stderr, "Failed to allocate memory for a bitset container.\n");
		  return NULL;
		}

		bitset_container_read(thiscard, c, buf);
		answer = c;
		*typecode = BITSET_CONTAINER_TYPE_CODE;
	} else if (isrun) {
		// we check that the read is allowed
		readbytes += sizeof(uint16_t);
		if(readbytes > rb->buf_len) {
		  fprintf(stderr, "Running out of bytes while reading a run container (header).\n");
		  return NULL;
		}
		uint16_t n_runs;
		memcpy(&n_runs, buf, sizeof(uint16_t));
		size_t containersize = n_runs * sizeof(rle16_t);
		readbytes += containersize;
		if(readbytes > rb->buf_len) {// data is corrupted?
		  fprintf(stderr, "Running out of bytes while reading a run container.\n");
		  return NULL;
		}
		// it is now safe to read

		run_container_t *c = run_container_create();
		if(c == NULL) {// memory allocation failure
		  fprintf(stderr, "Failed to allocate memory for a run container.\n");
		  return NULL;
		}
		run_container_read(thiscard, c, buf);
		answer = c;
		*typecode = RUN_CONTAINER_TYPE_CODE;
	} else {
		// we check that the read is allowed
		size_t containersize = thiscard * sizeof(uint16_t);
		readbytes += containersize;
		if(readbytes > rb->buf_len) {// data is corrupted?
		  fprintf(stderr, "Running out of bytes while reading an array container.\n");
		  return NULL;
		}
		// it is now safe to read
		array_container_t *c =
			array_container_create_given_capacity(thiscard);
		if(c == NULL) {// memory allocation failure
		  fprintf(stderr, "Failed to allocate memory for an array container.\n");
		  return NULL;
		}
		array_container_read(thiscard, c, buf);
		answer = c;
		*typecode = ARRAY_CONTAINER_TYPE_CODE;
	}

	return answer;
}

/**
 * Get the index corresponding to a 16-bit key
 */
static inline int32_t rb_get_index(const roaring_buffer_t *rb, uint16_t x){
    if ((rb->size == 0) || rb->keyscards[(rb->size - 1) * 2] == x) return rb->size - 1;
    return keyscardsBinarySearch(rb->keyscards, rb->size, x);
}

static inline int32_t rb_advance_until(const roaring_buffer_t *rb, uint16_t x,
                                       int32_t pos) {
    return keyscardsAdvanceUntil(rb->keyscards, pos, rb->size, x);
}

/**
 * Append new key-value pairs to ra, cloning  values from rb at indexes
 * [start_index, end_index)
 * Return false if error occurred.
 *
**/
static bool rb_append_copy_range(roaring_array_t *ra, const roaring_buffer_t *rb,
                          int32_t start_index, int32_t end_index) {
    bool ret;
    ret = extend_array(ra, end_index - start_index);
    if(!ret)
        return false;
    for (int32_t i = start_index; i < end_index; ++i) {
        const int32_t pos = ra->size;
        uint8_t container_type = 0;
        void *c = rb_get_container_at_index(rb, i, &container_type);
        if(c == NULL)
            return false;
        ra->keys[pos] = rb->keyscards[i * 2];
        ra->containers[pos] = c;
        ra->typecodes[pos] = container_type;
        ra->size++;
    }
    return true;
}


/**
 * Creates a new roaring buffer (from a partable serialized roaringbitmap buffer).
 * Returns NULL if error occurred.
 */
roaring_buffer_t *roaring_buffer_create(const char *buf, size_t buf_len){
    size_t readbytes;
    const char * initbuf = buf;

    readbytes = sizeof(int32_t);// for cookie
    if(readbytes > buf_len) {
      fprintf(stderr, "Ran out of bytes while reading first 4 bytes.\n");
      return NULL;
    }
    uint32_t cookie;
    memcpy(&cookie, buf, sizeof(int32_t));
    buf += sizeof(uint32_t);
    if ((cookie & 0xFFFF) != SERIAL_COOKIE &&
        cookie != SERIAL_COOKIE_NO_RUNCONTAINER) {
        fprintf(stderr, "I failed to find one of the right cookies. Found %" PRIu32 "\n",
                cookie);
        return NULL;
    }
    int32_t size;

    if ((cookie & 0xFFFF) == SERIAL_COOKIE)
        size = (cookie >> 16) + 1;
    else {
        readbytes += sizeof(int32_t);
        if(readbytes > buf_len) {
          fprintf(stderr, "Ran out of bytes while reading second part of the cookie.\n");
          return NULL;
        }
        memcpy(&size, buf, sizeof(int32_t));
        buf += sizeof(uint32_t);
    }
    if (size > (1<<16)) {
       fprintf(stderr, "You cannot have so many containers, the data must be corrupted: %" PRId32 "\n",
                size);
       return NULL; // logically impossible
    }
    const char *bitmapOfRunContainers = NULL;
    bool hasrun = (cookie & 0xFFFF) == SERIAL_COOKIE;
    if (hasrun) {
        int32_t s = (size + 7) / 8;
        readbytes += s;
        if(readbytes > buf_len) {// data is corrupted?
          fprintf(stderr, "Ran out of bytes while reading run bitmap.\n");
          return NULL;
        }
        bitmapOfRunContainers = buf;
        buf += s;
    }
    uint16_t *keyscards = (uint16_t *)buf;

    readbytes += size * 2 * sizeof(uint16_t);
    if(readbytes > buf_len) {
      fprintf(stderr, "Ran out of bytes while reading key-cardinality array.\n");
      return NULL;
    }
    buf += size * 2 * sizeof(uint16_t);

    /* make sure keyscards is 2 bytes aligned */
    bool keyscards_need_free = false;
    if ((uintptr_t)keyscards % sizeof(uint16_t) != 0) {
    	uint16_t * tmpbuf = malloc(size * 2 * sizeof(uint16_t));
    	if (tmpbuf == NULL) {
            fprintf(stderr, "Failed to allocate memory for keyscards. Bailing out.\n");
    		return NULL;
    	}
    	memcpy(tmpbuf, keyscards, size * 2 * sizeof(uint16_t));
    	keyscards_need_free = true;
    	keyscards = tmpbuf;
    }

    uint32_t *offsets = NULL;
    bool offsets_need_free = false;
    if ((!hasrun) || (size >= NO_OFFSET_THRESHOLD)) {
        readbytes += size * 4;
        if(readbytes > buf_len) {// data is corrupted?
          fprintf(stderr, "Ran out of bytes while reading offsets.\n");
          if(keyscards_need_free)
        	  free(keyscards);
          return NULL;
        }

        offsets = (uint32_t *)buf;
        if ((uintptr_t)offsets % 4 != 0) {
        	uint32_t * tmpbuf = malloc(size * 4);
        	if (tmpbuf == NULL) {
                fprintf(stderr, "Failed to allocate memory for offsets. Bailing out.\n");
                if(keyscards_need_free)
              	  free(keyscards);
        		return NULL;
        	}
        	memcpy(tmpbuf, offsets, size * 4);
        	offsets_need_free = true;
        	offsets = tmpbuf;
        }
        // skipping the offsets
        buf += size * 4;
    }
    else {
    	offsets = malloc(size * 4);
    	if (offsets == NULL) {
            fprintf(stderr, "Failed to allocate memory for offsets. Bailing out.\n");
            if(keyscards_need_free)
          	  free(keyscards);
    		return NULL;
    	}
    	offsets_need_free = true;

        // Reading the containers to fill offsets
        for (int32_t k = 0; k < size; ++k) {
        	uint32_t thiscard = keyscards[2*k+1] + 1;
        	bool isbitmap = (thiscard > DEFAULT_MAX_SIZE);
        	bool isrun = false;
        	if((bitmapOfRunContainers[k / 8] & (1 << (k % 8))) != 0) {
				isbitmap = false;
				isrun = true;
        	}

        	offsets[k] = readbytes;

            if (isbitmap) {
                size_t containersize = BITSET_CONTAINER_SIZE_IN_WORDS * sizeof(uint64_t);
                readbytes += containersize;
                buf += containersize;
            } else if (isrun) {
                // we check that the read is allowed
                readbytes += sizeof(uint16_t);
                if(readbytes > buf_len) {
                  fprintf(stderr, "Running out of bytes while reading a run container (header).\n");
                  if(keyscards_need_free)
                	  free(keyscards);
                  free(offsets);
          		  return NULL;
                }
                uint16_t n_runs;
                memcpy(&n_runs, buf, sizeof(uint16_t));
                size_t containersize = n_runs * sizeof(rle16_t);
                readbytes += containersize;
                buf += containersize;
            } else {
                // we check that the read is allowed
                size_t containersize = thiscard * sizeof(uint16_t);
                readbytes += containersize;
                buf += containersize;
            }
        }
    }

    roaring_buffer_t *ans = (roaring_buffer_t *)malloc(sizeof(roaring_buffer_t));
	if (ans == NULL) {
        fprintf(stderr, "Failed to allocate memory for roaring buffer. Bailing out.\n");
        if(keyscards_need_free)
      	  free(keyscards);
        if(offsets_need_free)
      	  free(offsets);
		return NULL;
	}

	ans->buf = initbuf;
	ans->buf_len = buf_len;
	ans->size = size;
	ans->keyscards = keyscards;
	ans->offsets = offsets;
	ans->bitmapOfRunContainers = bitmapOfRunContainers;
	ans->hasrun = hasrun;
	ans->keyscards_need_free = keyscards_need_free;
	ans->offsets_need_free = offsets_need_free;

    return ans;
}


/**
 * free roaring buffer
 */
void roaring_buffer_free(const roaring_buffer_t *rb) {
    if(rb->keyscards_need_free)
  	  free((void *)rb->keyscards);
    if(rb->offsets_need_free)
  	  free((void *)rb->offsets);

    free((void *)rb);
}


/**
 * Get the cardinality of the bitmap (number of elements).
 */
uint64_t roaring_buffer_get_cardinality(const roaring_buffer_t *ra) {
    uint64_t card = 0;
    for (int i = 0; i < ra->size; ++i)
    {
        card += ra->keyscards[2*i+1] + 1;
    }
    return card;
}


/**
 * Check if value x is present
 * Return false if error occurred.
 */
bool roaring_buffer_contains(const roaring_buffer_t *r,
                             uint32_t val,
                             bool *result) {
    bool answer;
    const uint16_t hb = val >> 16;
    /*
     * the next function call involves a binary search and lots of branching.
     */
    int32_t i = rb_get_index(r, hb);
    if (i < 0){
        *result = false;
        return true;
    }

    uint8_t typecode;
    // next call ought to be cheap
    void *container =
        rb_get_container_at_index(r, i, &typecode);
    if(container == NULL)
    {
        return false;
    }
    
    // rest might be a tad expensive, possibly involving another round of binary search
    answer = container_contains(container, val & 0xFFFF, typecode);
    container_free(container, typecode);

    *result = answer;
    return true;
}


/**
 * Check if all the elements of ra1 are also in ra2.
 * Return false if error occurred.
 */
bool roaring_buffer_is_subset(const roaring_buffer_t *ra1,
                              const roaring_buffer_t *ra2,
                              bool *result) {
    const int length1 = ra1->size,
              length2 = ra2->size;

    int pos1 = 0, pos2 = 0;

    while (pos1 < length1 && pos2 < length2) {
        const uint16_t s1 = rb_get_key_at_index(ra1, pos1);
        const uint16_t s2 = rb_get_key_at_index(ra2, pos2);

        if (s1 == s2) {
            uint8_t container_type_1, container_type_2;
            void *c1 = rb_get_container_at_index(ra1, pos1,
                                                 &container_type_1);
            if(c1 == NULL)
            {
                return false;
            }
            void *c2 = rb_get_container_at_index(ra2, pos2,
                                                 &container_type_2);
            if(c2 == NULL)
            {
                container_free(c1, container_type_1);
                return false;
            }
            bool subset =
                container_is_subset(c1, container_type_1, c2, container_type_2);
            container_free(c1, container_type_1);
            container_free(c2, container_type_2);
            if (!subset){
                *result = false;
                return true;
            } 
            ++pos1;
            ++pos2;
        } else if (s1 < s2) {  // s1 < s2
            *result = false;
            return true;
        } else {  // s1 > s2
            pos2 = rb_advance_until(ra2, s1, pos2);
        }
    }
    if (pos1 == length1)
        *result = true;
    else
        *result = false;
    return true;
}


/**
 * Computes the intersection between two bitmaps and returns new bitmap. The
 * caller is responsible for memory management.
 * Return NULL if error occurred.
 */
roaring_bitmap_t *roaring_buffer_and(const roaring_buffer_t *ra1,
                              const roaring_buffer_t *ra2) {
    uint8_t container_result_type = 0;
    const int length1 = ra1->size,
              length2 = ra2->size;
    uint32_t neededcap = length1 > length2 ? length2 : length1;
    roaring_bitmap_t *answer = roaring_bitmap_create_with_capacity(neededcap);
    if(answer == NULL)
        return NULL;

    int pos1 = 0, pos2 = 0;

    while (pos1 < length1 && pos2 < length2) {
        const uint16_t s1 = rb_get_key_at_index(ra1, pos1);
        const uint16_t s2 = rb_get_key_at_index(ra2, pos2);

        if (s1 == s2) {
            uint8_t container_type_1, container_type_2;
            void *c1 = rb_get_container_at_index(ra1, pos1,
                                                 &container_type_1);
            if(c1 == NULL)
            {
                roaring_bitmap_free(answer);
                return NULL;
            }
            void *c2 = rb_get_container_at_index(ra2, pos2,
                                                 &container_type_2);
            if(c2 == NULL)
            {
                container_free(c1, container_type_1);
                roaring_bitmap_free(answer);
                return NULL;
            }
            void *c = container_and(c1, container_type_1, c2, container_type_2,
                                    &container_result_type);
            container_free(c1, container_type_1);
            container_free(c2, container_type_2);
            if(c == NULL)
            {
                roaring_bitmap_free(answer);
                return NULL;
            }
            if (container_nonzero_cardinality(c, container_result_type)) {
                ra_append(&answer->high_low_container, s1, c,
                          container_result_type);
            } else {
                container_free(
                    c, container_result_type);  // otherwise:memory leak!
            }
            ++pos1;
            ++pos2;
        } else if (s1 < s2) {  // s1 < s2
            pos1 = rb_advance_until(ra1, s2, pos1);
        } else {  // s1 > s2
            pos2 = rb_advance_until(ra2, s1, pos2);
        }
    }
    return answer;
}


/**
 * Computes the size of the difference (andnot) between two bitmaps.
 * Return NULL if error occurred.
 */
roaring_bitmap_t *roaring_buffer_andnot(const roaring_buffer_t *x1,
                                        const roaring_buffer_t *x2) {
    bool ret;
    uint8_t container_result_type = 0;
    const int length1 = x1->size,
              length2 = x2->size;
    if (0 == length1) {
        roaring_bitmap_t *empty_bitmap = roaring_bitmap_create();
        return empty_bitmap;
    }
    if (0 == length2) {
        return roaring_bitmap_portable_deserialize(x1->buf);
    }
    roaring_bitmap_t *answer = roaring_bitmap_create_with_capacity(length1);
    if(answer == NULL)
        return NULL;

    int pos1 = 0, pos2 = 0;
    uint8_t container_type_1, container_type_2;
    uint16_t s1 = 0;
    uint16_t s2 = 0;
    while (true) {
        s1 = rb_get_key_at_index(x1, pos1);
        s2 = rb_get_key_at_index(x2, pos2);

        if (s1 == s2) {
            void *c1 = rb_get_container_at_index(x1, pos1,
                                                 &container_type_1);
            if(c1 == NULL)
            {
                roaring_bitmap_free(answer);
                return NULL;
            }
            void *c2 = rb_get_container_at_index(x2, pos2,
                                                 &container_type_2);
            if(c2 == NULL)
            {
                container_free(c1, container_type_1);
                roaring_bitmap_free(answer);
                return NULL;
            }
            void *c =
                container_andnot(c1, container_type_1, c2, container_type_2,
                                 &container_result_type);
            container_free(c1, container_type_1);
            container_free(c2, container_type_2);
            if(c == NULL)
            {
                roaring_bitmap_free(answer);
                return NULL;
            }
            if (container_nonzero_cardinality(c, container_result_type)) {
                ra_append(&answer->high_low_container, s1, c,
                          container_result_type);
            } else {
                container_free(c, container_result_type);
            }
            ++pos1;
            ++pos2;
            if (pos1 == length1) break;
            if (pos2 == length2) break;
        } else if (s1 < s2) {  // s1 < s2
            const int next_pos1 = rb_advance_until(x1, s2, pos1);
            ret = rb_append_copy_range(&answer->high_low_container,
                                 x1, pos1, next_pos1);
            if(!ret)
            {
                roaring_bitmap_free(answer);
                return NULL;
            }
            // TODO : perhaps some of the copy_on_write should be based on
            // answer rather than x1 (more stringent?).  Many similar cases
            pos1 = next_pos1;
            if (pos1 == length1) break;
        } else {  // s1 > s2
            pos2 = rb_advance_until(x2, s1, pos2);
            if (pos2 == length2) break;
        }
    }
    if (pos2 == length2) {
        ret = rb_append_copy_range(&answer->high_low_container,
                             x1, pos1, length1);
        if(!ret)
        {
            roaring_bitmap_free(answer);
            return NULL;
        }
    }
    return answer;
}


/**
 * Computes the size of the intersection between two bitmaps.
 * Return false if error occurred.
 */
bool roaring_buffer_and_cardinality(const roaring_buffer_t *x1,
                                        const roaring_buffer_t *x2,
                                        uint64_t *result) {
    const int length1 = x1->size,
              length2 = x2->size;
    uint64_t cardinality = 0;
    int pos1 = 0, pos2 = 0;

    while (pos1 < length1 && pos2 < length2) {
        const uint16_t s1 = rb_get_key_at_index(x1, pos1);
        const uint16_t s2 = rb_get_key_at_index(x2, pos2);

        if (s1 == s2) {
            uint8_t container_type_1, container_type_2;
            void *c1 = rb_get_container_at_index(x1, pos1,
                                                 &container_type_1);
            if(c1 == NULL)
                return false;

            void *c2 = rb_get_container_at_index(x2, pos2,
                                                 &container_type_2);
            if(c2 == NULL)
            {
                container_free(c1, container_type_1);
                return false;
            }
            cardinality += container_and_cardinality(c1, container_type_1, c2,
                                                container_type_2);
            container_free(c1, container_type_1);
            container_free(c2, container_type_2);
            ++pos1;
            ++pos2;
        } else if (s1 < s2) {  // s1 < s2
            pos1 = rb_advance_until(x1, s2, pos1);
        } else {  // s1 > s2
            pos2 = rb_advance_until(x2, s1, pos2);
        }
    }
    *result = cardinality;
    return true;
}


/**
 * Computes the size of the union between two bitmaps.
 * Return false if error occurred.
 */
bool roaring_buffer_or_cardinality(const roaring_buffer_t *x1,
                                   const roaring_buffer_t *x2,
                                   uint64_t *result) {
    bool ret;
    uint64_t inter;
    const uint64_t c1 = roaring_buffer_get_cardinality(x1);
    const uint64_t c2 = roaring_buffer_get_cardinality(x2);
    ret = roaring_buffer_and_cardinality(x1, x2, &inter);
    if(!ret)
        return false;
    *result = c1 + c2 - inter;
    return true;
}


/**
 * Computes the size of the difference (andnot) between two bitmaps.
 * Return false if error occurred.
 */
bool roaring_buffer_andnot_cardinality(const roaring_buffer_t *x1,
                                       const roaring_buffer_t *x2,
                                       uint64_t *result) {
    bool ret;
    uint64_t inter;
    const uint64_t c1 = roaring_buffer_get_cardinality(x1);
    ret = roaring_buffer_and_cardinality(x1, x2, &inter);
    if(!ret)
        return false;

    *result = c1 - inter;
    return true;
}


/**
 * Computes the size of the symmetric difference (andnot) between two bitmaps.
 * Return false if error occurred.
 */
bool roaring_buffer_xor_cardinality(const roaring_buffer_t *x1,
                                    const roaring_buffer_t *x2,
                                    uint64_t *result) {
    bool ret;
    uint64_t inter;
    const uint64_t c1 = roaring_buffer_get_cardinality(x1);
    const uint64_t c2 = roaring_buffer_get_cardinality(x2);
    ret = roaring_buffer_and_cardinality(x1, x2, &inter);
    if(!ret)
        return false;
    *result = c1 + c2 - 2 * inter;
    return true;
}


/**
 * Computes the Jaccard index between two bitmaps. (Also known as the Tanimoto
 * distance, or the Jaccard similarity coefficient)
 *
 * The Jaccard index is undefined if both bitmaps are empty.
 * Return false if error occurred.
 */
bool roaring_buffer_jaccard_index(const roaring_buffer_t *x1,
                                  const roaring_buffer_t *x2,
                                  double *result) {
    bool ret;
    uint64_t inter;
    const uint64_t c1 = roaring_buffer_get_cardinality(x1);
    const uint64_t c2 = roaring_buffer_get_cardinality(x2);
    ret = roaring_buffer_and_cardinality(x1, x2, &inter);
    if(!ret)
        return false;
    *result = (double)inter / (double)(c1 + c2 - inter);
    return true;
}


/**
 * Check whether two bitmaps intersect.
 * Return false if error occurred.
 */
bool roaring_buffer_intersect(const roaring_buffer_t *x1,
                              const roaring_buffer_t *x2,
                              bool *result) {
    const int length1 = x1->size,
              length2 = x2->size;
    int pos1 = 0, pos2 = 0;

    while (pos1 < length1 && pos2 < length2) {
        const uint16_t s1 = rb_get_key_at_index(x1, pos1);
        const uint16_t s2 = rb_get_key_at_index(x2, pos2);

        if (s1 == s2) {
            uint8_t container_type_1, container_type_2;
            void *c1 = rb_get_container_at_index(x1, pos1,
                                                 &container_type_1);
            if(c1 == NULL)
                return false;

            void *c2 = rb_get_container_at_index(x2, pos2,
                                                 &container_type_2);
            if(c2 == NULL)
            {
                container_free(c1, container_type_1);
                return false;
            }

            bool intersect = container_intersect(c1, container_type_1, c2, container_type_2);
            container_free(c1, container_type_1);
            container_free(c2, container_type_2);

            if(intersect){
                *result = true;
                return true;
            }
            ++pos1;
            ++pos2;
        } else if (s1 < s2) {  // s1 < s2
            pos1 = rb_advance_until(x1, s2, pos1);
        } else {  // s1 > s2
            pos2 = rb_advance_until(x2, s1, pos2);
        }
    }
    *result = false;
    return true;
}


/**
* Returns true if the bitmap is empty (cardinality is zero).
*/
bool roaring_buffer_is_empty(const roaring_buffer_t *rb) {
    return rb->size == 0;
}


/**
 * Check if the two bitmaps contain the same elements.
 * Return false if error occurred.
 */
bool roaring_buffer_equals(const roaring_buffer_t *rb1,
                           const roaring_buffer_t *rb2,
                           bool *result) {
    if (rb1->size != rb2->size) {
        *result = false;
        return true;
    }
    for (int i = 0; i < rb1->size; ++i) {
        if (rb1->keyscards[i * 2] !=
            rb2->keyscards[i * 2]) {
            *result = false;
            return true;
        }
    }
    for (int i = 0; i < rb1->size; ++i) {
        uint8_t container_type_1, container_type_2;
        void *c1 = rb_get_container_at_index(rb1, i,
                                                &container_type_1);
        if(c1 == NULL)
            return false;

        void *c2 = rb_get_container_at_index(rb2, i,
                                                &container_type_2);
        if(c2 == NULL)
        {
            container_free(c1, container_type_1);
            return false;
        }
                                     
        bool areequal = container_equals(c1,container_type_1,
                                         c2,container_type_2);
        container_free(c1, container_type_1);
        container_free(c2, container_type_2);

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
bool roaring_buffer_rank(const roaring_buffer_t *rb,
                         uint32_t x,
                         uint64_t *result) {
    uint32_t xhigh = x >> 16;
    *result = 0;
    for (int i = 0; i < rb->size; i++) {
        uint32_t key = rb->keyscards[i * 2];
        if (xhigh < key)
        {
            return true;
        }
        else
        {
            uint8_t container_type;
            void *c = rb_get_container_at_index(rb, i,
                                                &container_type);
            if(c == NULL)
                return false;

            if (xhigh == key) {
                *result += container_rank(c, container_type, x & 0xFFFF);
                container_free(c, container_type);
                return true;
            } else{
                *result += container_get_cardinality(c, container_type);
                container_free(c, container_type);
            }
        }
    }
    return true;
}


/**
* Get the smallest value in the set, or UINT32_MAX if the set is empty.
* Return false if error occurred.
*/
bool roaring_buffer_minimum(const roaring_buffer_t *rb,
                            uint32_t *result) {
    if (rb->size > 0) {
        uint8_t typecode;
        int i = 0;
        uint32_t key = rb->keyscards[i * 2];
        void *container = rb_get_container_at_index(rb, i, &typecode);
        if(container == NULL)
            return false;

        uint32_t lowvalue = container_minimum(container, typecode);
        *result = lowvalue | (key << 16);
    }else {
        *result = UINT32_MAX;
    }
    return true;
}


/**
* Get the greatest value in the set, or 0 if the set is empty.
* Return false if error occurred.
*/
bool roaring_buffer_maximum(const roaring_buffer_t *rb,
                            uint32_t *result) {
    if (rb->size > 0) {
        uint8_t typecode;
        int i = rb->size - 1;
        uint32_t key = rb->keyscards[i * 2];
        void *container = rb_get_container_at_index(rb, i, &typecode);
        if(container == NULL)
            return false;

        uint32_t lowvalue = container_maximum(container, typecode);
        *result =  lowvalue | (key << 16);
    }else {
        *result = 0;
    }
    return true;
}