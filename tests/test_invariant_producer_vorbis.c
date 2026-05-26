#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/*
 * Security invariant:
 * When formatting a metadata attribute name using a pattern like
 * "meta.attr.%s.markup", the resulting string MUST NOT exceed the
 * bounds of the destination buffer. The formatted output length must
 * always be within safe bounds regardless of the input string length.
 *
 * This test simulates the vulnerable sprintf pattern and verifies that
 * a safe implementation (using snprintf with bounds checking) correctly
 * handles adversarial inputs without buffer overflow.
 */

/* Simulate the fixed-size buffer used in the vulnerable code.
 * In the real code, meta->name is a fixed-size buffer. We use 256 bytes
 * as a representative size (common for such buffers). */
#define META_NAME_BUFFER_SIZE 256

/* The format string prefix and suffix from the vulnerable code */
#define FORMAT_PREFIX "meta.attr."
#define FORMAT_SUFFIX ".markup"
#define FORMAT_OVERHEAD (sizeof(FORMAT_PREFIX) - 1 + sizeof(FORMAT_SUFFIX) - 1)

/*
 * Safe formatting function that enforces bounds checking.
 * Returns 0 on success, -1 if the input would overflow the buffer.
 */
static int safe_format_meta_name(char *dest, size_t dest_size, const char *str)
{
    if (!dest || !str || dest_size == 0) {
        return -1;
    }

    /* Check if the formatted string would fit */
    size_t str_len = strlen(str);
    size_t required = FORMAT_OVERHEAD + str_len + 1; /* +1 for null terminator */

    if (required > dest_size) {
        return -1; /* Would overflow */
    }

    int written = snprintf(dest, dest_size, "meta.attr.%s.markup", str);

    /* snprintf returns negative on error, or >= dest_size if truncated */
    if (written < 0 || (size_t)written >= dest_size) {
        return -1;
    }

    return 0;
}

START_TEST(test_meta_name_buffer_overflow_prevention)
{
    /* Invariant: Formatting metadata attribute names must never overflow
     * the destination buffer, regardless of the input string content or length. */
    const char *payloads[] = {
        /* Normal inputs */
        "",
        "a",
        "normal_key",
        "ARTIST",
        "TITLE",
        /* Boundary inputs */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", /* 64 chars */
        /* Inputs that would overflow a 256-byte buffer with the format string */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", /* 256 chars */
        /* Very long inputs */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", /* 512 chars */
        /* Attack payloads with special characters */
        "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
        "%n%n%n%n%n%n%n%n",
        "../../../../etc/passwd",
        "\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41",
        /* Null-byte adjacent */
        "key\x00injection",
        /* Unicode-like long sequences */
        "\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf\xc0\xaf",
        /* Exactly at boundary: META_NAME_BUFFER_SIZE - FORMAT_OVERHEAD - 1 chars */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", /* 242 chars: 256 - 17 (overhead+null) + 3 */
        /* One over boundary */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", /* 243 chars */
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        /* Allocate buffer with guard pages (canary pattern) */
        char meta_name[META_NAME_BUFFER_SIZE];
        char canary_before[16];
        char canary_after[16];

        /* Initialize canaries */
        memset(canary_before, 0xAB, sizeof(canary_before));
        memset(canary_after, 0xCD, sizeof(canary_after));
        memset(meta_name, 0, sizeof(meta_name));

        const char *str = payloads[i];
        size_t str_len = strlen(str);
        size_t required_len = FORMAT_OVERHEAD + str_len + 1;

        int result = safe_format_meta_name(meta_name, META_NAME_BUFFER_SIZE, str);

        if (result == 0) {
            /* Success case: verify the output is properly null-terminated
             * and within bounds */
            size_t actual_len = strlen(meta_name);

            /* The formatted string must fit within the buffer */
            ck_assert_msg(actual_len < META_NAME_BUFFER_SIZE,
                "Formatted meta name length %zu must be less than buffer size %d for input length %zu",
                actual_len, META_NAME_BUFFER_SIZE, str_len);

            /* Verify the format is correct */
            ck_assert_msg(strncmp(meta_name, "meta.attr.", 10) == 0,
                "Formatted meta name must start with 'meta.attr.'");

            /* Verify null termination */
            ck_assert_msg(meta_name[META_NAME_BUFFER_SIZE - 1] == '\0' ||
                          actual_len < META_NAME_BUFFER_SIZE - 1,
                "Buffer must be properly null-terminated");

        } else {
            /* Rejection case: input was too long, verify buffer is safe */
            ck_assert_msg(required_len > META_NAME_BUFFER_SIZE,
                "Safe function should only reject inputs that would overflow: "
                "required=%zu, buffer=%d, input_len=%zu",
                required_len, META_NAME_BUFFER_SIZE, str_len);
        }

        /* Verify canaries are intact (no buffer overflow occurred) */
        /* Note: In a real scenario these would be adjacent in memory,
         * but here we verify the logic invariant */
        for (int j = 0; j < 16; j++) {
            ck_assert_msg(canary_before[j] == (char)0xAB,
                "Pre-buffer canary corrupted at index %d for payload %d", j, i);
            ck_assert_msg(canary_after[j] == (char)0xCD,
                "Post-buffer canary corrupted at index %d for payload %d", j, i);
        }

        /* Core invariant: if the input would cause overflow, it must be rejected */
        if (str_len + FORMAT_OVERHEAD + 1 > META_NAME_BUFFER_SIZE) {
            ck_assert_msg(result != 0,
                "Payload %d (length %zu) should have been rejected to prevent overflow",
                i, str_len);
        }

        /* Core invariant: if accepted, the result must fit in the buffer */
        if (result == 0) {
            ck_assert_msg(strlen(meta_name) + 1 <= META_NAME_BUFFER_SIZE,
                "Accepted payload %d produced output that exceeds buffer bounds", i);
        }
    }
}
END_TEST

START_TEST(test_meta_name_format_string_safety)
{
    /* Invariant: Format string attack payloads must not cause undefined behavior
     * when processed through the safe formatting function. */
    const char *payloads[] = {
        "%s",
        "%d",
        "%n",
        "%x%x%x%x",
        "%%",
        "%p%p%p%p",
        "%99999999d",
        "%s%s%s%s%s%s%s%s%s%s",
        "%1$s%2$s%3$s",
        "%.999999999f",
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        char meta_name[META_NAME_BUFFER_SIZE];
        memset(meta_name, 0, sizeof(meta_name));

        const char *str = payloads[i];
        int result = safe_format_meta_name(meta_name, META_NAME_BUFFER_SIZE, str);

        if (result == 0) {
            /* The output must be a literal string containing the payload,
             * not an interpreted format string */
            size_t actual_len = strlen(meta_name);
            ck_assert_msg(actual_len < META_NAME_BUFFER_SIZE,
                "Format string payload %d produced output exceeding buffer", i);

            /* Verify the payload appears literally in the output */
            ck_assert_msg(strstr(meta_name, str) != NULL,
                "Format string payload %d should appear literally in output", i);
        }
        /* If rejected, that's also acceptable behavior */
    }
}
END_TEST

START_TEST(test_meta_name_exact_boundary)
{
    /* Invariant: Inputs at exactly the boundary must be handled correctly -
     * either accepted (if they fit) or rejected (if they don't). */

    /* Calculate exact boundary sizes */
    /* Format: "meta.attr." + str + ".markup" + '\0'
     * = 10 + str_len + 7 + 1 = str_len + 18 */
    const size_t format_overhead_with_null = FORMAT_OVERHEAD + 1; /* 18 */
    const size_t max_str_len = META_NAME_BUFFER_SIZE - format_overhead_with_null;

    /* Create a string of exactly max_str_len characters */
    char *exact_fit = malloc(max_str_len + 1);
    ck_assert_msg(exact_fit != NULL, "Memory allocation failed");
    memset(exact_fit, 'A', max_str_len);
    exact_fit[max_str_len] = '\0';

    /* Create a string of max_str_len + 1 characters (one too many) */
    char *one_over = malloc(max_str_len + 2);
    ck_assert_msg(one_over != NULL, "Memory allocation failed");
    memset(one_over, 'A', max_str_len + 1);
    one_over[max_str_len + 1] = '\0';

    char meta_name[META_NAME_BUFFER_SIZE];

    /* Exact fit should succeed */
    memset(meta_name, 0, sizeof(meta_name));
    int result_exact = safe_format_meta_name(meta_name, META_NAME_BUFFER_SIZE, exact_fit);
    ck_assert_msg(result_exact == 0,
        "String of exactly max length (%zu) should be accepted", max_str_len);
    ck_assert_msg(strlen(meta_name) == META_NAME_BUFFER_SIZE - 1,
        "Exact fit string should produce output of length %d", META_NAME_BUFFER_SIZE - 1);

    /* One over should be rejected */
    memset(meta_name, 0, sizeof(meta_name));
    int result_over = safe_format_meta_name(meta_name, META_NAME_BUFFER_SIZE, one_over);
    ck_assert_msg(result_over != 0,
        "String one character over max length (%zu) must be rejected to prevent overflow",
        max_str_len + 1);

    free(exact_fit);
    free(one_over);
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_set_timeout(tc_core, 30);
    tcase_add_test(tc_core, test_meta_name_buffer_overflow_prevention);
    tcase_add_test(tc_core, test_meta_name_format_string_safety);
    tcase_add_test(tc_core, test_meta_name_exact_boundary);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}