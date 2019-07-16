#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include "endian.h"

#define min(a, b) ((a) > (b) ? (b) : (a))


/* references:
 * - https://www.infradead.org/~mchehab/kernel_docs/unsorted/lzo.html
 * - https://github.com/synopse/mORMot/blob/master/SynLZO.pas
 */

static inline unsigned lzo_parse_length(const uint8_t **buf, unsigned bits)
{
    unsigned mask = (1 << bits) - 1;

    const uint8_t *p = *buf;
    unsigned len = *p++ & mask;
    if (!len) {
        for (; !*p; p++)
            len += 0xFF;
        len += *p++ + mask;
    }

    *buf = p;
    return len;
}

static inline void lzo_copy(uint8_t **out, const uint8_t **in, size_t n)
{
    memcpy(*out, *in, n);
    *out += n;
    *in += n;
}

static inline void lzo_copy_distance(uint8_t **out, ptrdiff_t dist, size_t n)
{
    /* interestingly, memmove() does *not* work here, at least on maCOS */
    uint8_t *p = *out;
    const uint8_t *in = p - dist;
    while (n--)
        *p++ = *in++;
    *out = p;
}

int lzo_decompress(const uint8_t *buf, size_t len, uint8_t *out, size_t outlen)
{
    const uint8_t *p = buf, *end = buf + len, *oend = out + outlen;
    uint8_t *op = out;
    uint8_t state = 0;

    while (p < end) {
        uint8_t instr = *p;
        /* first command is special */
        if (p == buf) {
            if (instr > 17) {
                lzo_copy(&op, &p, instr - 17);
                state = 4;
                continue;
            }
        }

        uint32_t count = 0;
        ptrdiff_t distance = 0;
        if (instr >= 64) {
            p++;
            count = (instr >> 5) + 1;
            distance = 1 + ((instr >> 2) & 7) + (*p++ << 3);
            state = instr & 3;
        } else if (instr >= 32) {
            /* back up and parse length properly */
            count = lzo_parse_length(&p, 5) + 2;
            distance = 1 + (le16toh(*(const uint16_t *)p) >> 2);
            state = le16toh(*(const uint16_t *)p) & 3;
            p += 2;
        } else if (instr >= 16) {
            /* back up and parse length properly */
            count = lzo_parse_length(&p, 3) + 2;
            distance = (instr & 8 << 11) + 0x4000 + (le16toh(*(const uint16_t *)p) >> 2);
            state = le16toh(*(const uint16_t *)p) & 3;
            p += 2;
        } else if (!state) {
            /* back up and parse length properly */
            count = lzo_parse_length(&p, 4) + 3;
            lzo_copy(&op, &p, count);
            state = 4;
            continue;
        } else {
            /* this shouldn't happen */
            return -1;
        }

        count = min(count, oend - op);
        if (count)
            lzo_copy_distance(&op, distance, count);
        if (state > 0 && state < 4)
            lzo_copy(&op, &p, state);
    }
    return 0;
}
