#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../roaring.h"
#include "../roaring_buffer_reader.h"

#include "../roaring.c"
#include "../roaring_buffer_reader.c"

#include <inttypes.h>
#include <stdio.h>

void CHECK_TRUE(bool b, const char *reason) {
    if (!b) {
        fprintf(stderr, "CHECK_TRUE failed: %s\n", reason);
        abort();
    }
}

void CHECK_FALSE(bool b, const char *reason) {
    if (b) {
        fprintf(stderr, "CHECK_FALSE failed: %s\n", reason);
        abort();
    }
}

void CHECK_EQ(uint64_t a, uint64_t b, const char *reason) {
    if (a != b) {
        fprintf(stderr, "CHECK_EQ failed: %s (%" PRIu64 " != %" PRIu64 ")\n", reason, a, b);
        abort();
    }
}

void CHECK_DOUBLE_EQ(double a, double b, const char *reason) {
    if (a != b) {
        fprintf(stderr, "CHECK_DOUBLE_EQ failed: %s (%f != %f)\n", reason, a, b);
        abort();
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    roaring_bitmap_t *bitmap1 = roaring_bitmap_portable_deserialize_safe((const char *)data, size);
    if (bitmap1 == NULL) {
        return -1;
    }
    
    roaring_buffer_t *buffer1 = roaring_buffer_create((const char *)data, size);
    CHECK_TRUE(buffer1 != NULL, "buffer1 creation");

    CHECK_EQ(roaring_bitmap_get_cardinality(bitmap1), roaring_buffer_get_cardinality(buffer1), "cardinality");
    CHECK_EQ(roaring_bitmap_is_empty(bitmap1), roaring_buffer_is_empty(buffer1), "is_empty");

    if (!roaring_bitmap_is_empty(bitmap1)) {
        uint32_t expected_min, actual_min;
        bool success1 = roaring_buffer_minimum(buffer1, &actual_min);
        CHECK_TRUE(success1, "buffer_minimum success");
        expected_min = roaring_bitmap_minimum(bitmap1);
        CHECK_EQ(expected_min, actual_min, "minimum value");

        uint32_t expected_max, actual_max;
        bool success2 = roaring_buffer_maximum(buffer1, &actual_max);
        CHECK_TRUE(success2, "buffer_maximum success");
        expected_max = roaring_bitmap_maximum(bitmap1);
        CHECK_EQ(expected_max, actual_max, "maximum value");

        for (uint32_t val = expected_min; val <= expected_max; val += (expected_max - expected_min) / 10) {
            bool expected_contains = roaring_bitmap_contains(bitmap1, val);
            bool actual_contains;
            bool success = roaring_buffer_contains(buffer1, val, &actual_contains);
            CHECK_TRUE(success, "buffer_contains success");
            CHECK_EQ(expected_contains, actual_contains, "contains value");

            uint64_t expected_rank = roaring_bitmap_rank(bitmap1, val);
            uint64_t actual_rank;
            success = roaring_buffer_rank(buffer1, val, &actual_rank);
            CHECK_TRUE(success, "buffer_rank success");
            CHECK_EQ(expected_rank, actual_rank, "rank value");
        }
    }


    uint64_t card = roaring_bitmap_get_cardinality(bitmap1);

    if (card > 1) {
        uint32_t *values = (uint32_t *)malloc(card * sizeof(uint32_t));
        roaring_bitmap_to_uint32_array(bitmap1, values);

        size_t subset_size = card / 2;
        uint32_t *subset_values = (uint32_t *)malloc(subset_size * sizeof(uint32_t));
        for (size_t i = 0; i < subset_size; ++i) {
            subset_values[i] = values[i * 2];
        }

        roaring_bitmap_t *bitmap2 = roaring_bitmap_of_ptr(subset_size, subset_values);
        char *serialized_buffer = (char *)malloc(roaring_bitmap_portable_size_in_bytes(bitmap2));
        size_t serialized_size = roaring_bitmap_portable_serialize(bitmap2, serialized_buffer);
        roaring_buffer_t *buffer2 = roaring_buffer_create(serialized_buffer, serialized_size);
        CHECK_TRUE(buffer2 != NULL, "buffer2 creation");

        bool expected_equals = roaring_bitmap_equals(bitmap1, bitmap2);
        bool actual_equals;
        bool success = roaring_buffer_equals(buffer1, buffer2, &actual_equals);
        CHECK_TRUE(success, "buffer_equals success");
        CHECK_EQ(expected_equals, actual_equals, "equals");


        bool expected_is_subset = roaring_bitmap_is_subset(bitmap1, bitmap2);
        bool actual_is_subset;
        success = roaring_buffer_is_subset(buffer1, buffer2, &actual_is_subset);
        CHECK_TRUE(success, "buffer_is_subset success");
        CHECK_EQ(expected_is_subset, actual_is_subset, "is_subset");

        uint64_t and_card;
        success = roaring_buffer_and_cardinality(buffer1, buffer2, &and_card);
        CHECK_TRUE(success, "buffer_and_cardinality success");
        CHECK_EQ(roaring_bitmap_and_cardinality(bitmap1, bitmap2), and_card, "and_cardinality");

        uint64_t or_card;
        success = roaring_buffer_or_cardinality(buffer1, buffer2, &or_card);
        CHECK_TRUE(success, "buffer_or_cardinality success");
        CHECK_EQ(roaring_bitmap_or_cardinality(bitmap1, bitmap2), or_card, "or_cardinality");

        uint64_t xor_card;
        success = roaring_buffer_xor_cardinality(buffer1, buffer2, &xor_card);
        CHECK_TRUE(success, "buffer_xor_cardinality success");
        CHECK_EQ(roaring_bitmap_xor_cardinality(bitmap1, bitmap2), xor_card, "xor_cardinality");

        uint64_t andnot_card;
        success = roaring_buffer_andnot_cardinality(buffer1, buffer2, &andnot_card);
        CHECK_TRUE(success, "buffer_andnot_cardinality success");
        CHECK_EQ(roaring_bitmap_andnot_cardinality(bitmap1, bitmap2), andnot_card, "andnot_cardinality");

        double jaccard;
        success = roaring_buffer_jaccard_index(buffer1, buffer2, &jaccard);
        CHECK_TRUE(success, "buffer_jaccard_index success");
        CHECK_DOUBLE_EQ(roaring_bitmap_jaccard_index(bitmap1, bitmap2), jaccard, "jaccard_index");

        bool intersect;
        success = roaring_buffer_intersect(buffer1, buffer2, &intersect);
        CHECK_TRUE(success, "buffer_intersect success");
        CHECK_EQ(roaring_bitmap_intersect(bitmap1, bitmap2), intersect, "intersect");

        roaring_bitmap_free(bitmap2);
        roaring_buffer_free(buffer2);
        free(serialized_buffer);
        free(values);
        free(subset_values);
    }

    roaring_bitmap_free(bitmap1);
    roaring_buffer_free(buffer1);
    return 0;
}
