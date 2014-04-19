/*
 * Fast bit array copy algorithm.
 * Source: http://stackoverflow.com/questions/3534535/whats-a-time-efficient-algorithm-to-copy-unaligned-bit-arrays
 */

#include <climits>
#include <cstring>
#include <cstddef>

#define PREPARE_FIRST_COPY()                                       \
    do {                                                           \
    if (src_len >= (CHAR_BIT - dst_offset_modulo)) {               \
        *dst &= reverse_mask[dst_offset_modulo];                   \
        src_len -= CHAR_BIT - dst_offset_modulo;                   \
    } else {                                                       \
        *dst &= reverse_mask[dst_offset_modulo] |                  \
                reverse_mask_xor[dst_offset_modulo + src_len + 1]; \
        c &= reverse_mask[dst_offset_modulo + src_len];            \
        src_len = 0;                                               \
    } } while (0)

namespace uavcan
{

void bitarrayCopy(const unsigned char* src_org, int src_offset, int src_len, unsigned char* dst_org, int dst_offset)
{
    static const unsigned char reverse_mask[]     = { 0x55U, 0x80U, 0xC0U, 0xE0U, 0xF0U, 0xF8U, 0xFCU, 0xFEU, 0xFFU };
    static const unsigned char reverse_mask_xor[] = { 0xFFU, 0x7FU, 0x3FU, 0x1FU, 0x0FU, 0x07U, 0x03U, 0x01U, 0x00U };

    if (src_len > 0)
    {
        const unsigned char* src;
        unsigned char* dst;
        int src_offset_modulo;
        int dst_offset_modulo;

        src = src_org + (src_offset / CHAR_BIT);
        dst = dst_org + (dst_offset / CHAR_BIT);

        src_offset_modulo = src_offset % CHAR_BIT;
        dst_offset_modulo = dst_offset % CHAR_BIT;

        if (src_offset_modulo == dst_offset_modulo)
        {
            int byte_len;
            int src_len_modulo;
            if (src_offset_modulo)
            {
                unsigned char c = reverse_mask_xor[dst_offset_modulo] & *src++;
                PREPARE_FIRST_COPY();
                *dst++ |= c;
            }

            byte_len = src_len / CHAR_BIT;
            src_len_modulo = src_len % CHAR_BIT;

            if (byte_len)
            {
                (void)std::memcpy(dst, src, byte_len);
                src += byte_len;
                dst += byte_len;
            }
            if (src_len_modulo)
            {
                *dst &= reverse_mask_xor[src_len_modulo];
                *dst |= reverse_mask[src_len_modulo] & *src;
            }
        }
        else
        {
            int bit_diff_ls;
            int bit_diff_rs;
            int byte_len;
            int src_len_modulo;
            unsigned char c;
            /*
             * Begin: Line things up on destination.
             */
            if (src_offset_modulo > dst_offset_modulo)
            {
                bit_diff_ls = src_offset_modulo - dst_offset_modulo;
                bit_diff_rs = CHAR_BIT - bit_diff_ls;

                c = *src++ << bit_diff_ls;
                c |= *src >> bit_diff_rs;
                c &= reverse_mask_xor[dst_offset_modulo];
            }
            else
            {
                bit_diff_rs = dst_offset_modulo - src_offset_modulo;
                bit_diff_ls = CHAR_BIT - bit_diff_rs;

                c = *src >> bit_diff_rs & reverse_mask_xor[dst_offset_modulo];
            }
            PREPARE_FIRST_COPY();
            *dst++ |= c;

            /*
             * Middle: copy with only shifting the source.
             */
            byte_len = src_len / CHAR_BIT;

            while (--byte_len >= 0)
            {
                c = *src++ << bit_diff_ls;
                c |= *src >> bit_diff_rs;
                *dst++ = c;
            }

            /*
             * End: copy the remaing bits;
             */
            src_len_modulo = src_len % CHAR_BIT;
            if (src_len_modulo)
            {
                c = *src++ << bit_diff_ls;
                c |= *src >> bit_diff_rs;
                c &= reverse_mask[src_len_modulo];

                *dst &= reverse_mask_xor[src_len_modulo];
                *dst |= c;
            }
        }
    }
}

}
