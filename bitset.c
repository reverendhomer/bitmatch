#include "bitset.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

int bitset_init(struct bitset *bs, uint64_t length)
{
	bs->cells = ceil(length * 1. / 64.);
	bs->data = calloc(bs->cells, sizeof(uint64_t));
	if (bs->data == NULL)
		return -1;
	bs->length = length;
	return 0;
}

void bitset_free(struct bitset *bs)
{
	free(bs->data);
}

void bitset_set_bits(struct bitset *bs, uint64_t lbit, uint64_t fbit, uint64_t val)
{
	const uint64_t lcell = (lbit-1) / 64; // cell containing last bit
	const uint64_t fcell = (fbit-1) / 64; // cell containing first bit

	assert(lbit >= fbit);

	if (lcell == fcell) {
		DBG("lbit=%lu, fbit=%lu, val=%lu, lcell=%lu\n", lbit - 64 * lcell, fbit - 64 * lcell, val, lcell);
		copy_range_bit(&bs->data[lcell], lbit - 64 * lcell, fbit - 64 * lcell, val);
	} else {
		// 5...11
		//       3 1   8  5
		// 1101 1111 | 1000 1100
		//       ___ | ____
		const uint8_t total_bits_to_send = lbit - fbit + 1; // 11 - 5 + 1 = 7
		// numbers of bits in their cells
		const uint8_t lbitcell = (lbit-1) % 64 + 1; // 3
		const uint8_t fbitcell = (fbit-1) % 64 + 1; // 5
		// lbit for first bit
		const uint8_t fpartmaxidx = 64 - fbitcell + 1; // 8 - 5 + 1 = 4

		const uint64_t fbitdata = get_range_bit(val, fpartmaxidx, 1);
		const uint64_t lbitdata = get_range_bit(val, total_bits_to_send, fpartmaxidx + 1);
		DBG("value = %lx\n", val);
		DBG("lbitcell=%u, fbitcell=%u, fpartmaxidx=%u, total_bits=%u\n",
			lbitcell, fbitcell, fpartmaxidx, total_bits_to_send);
		DBG("[%lu](%u..%u) = %lx, [%lu](%u..%u)  = %lx (total %u)\n",
				lcell, 1, lbitcell, lbitdata,
				fcell, fbitcell, 64, fbitdata, total_bits_to_send);
		copy_range_bit(&bs->data[lcell], lbitcell, 1, lbitdata);
		copy_range_bit(&bs->data[fcell], 64, fbitcell, fbitdata);
	}
}

void bitset_shift_with_bit(struct bitset *bs, uint8_t bit)
{
	size_t i;
	uint64_t lbit = bit;
	for (i = 0; i < bs->cells; ++i) {
		uint64_t t = get_bit(bs->data[i], 64);
		DBG("lbit=%lu\n", lbit);
		bs->data[i] = (bs->data[i] << 1) | lbit;
		lbit = t;
	}
	// now truncate the first word
	lbit = (uint64_t)ceil(bs->length / 64.) * 64;
	if (lbit > bs->length) {
		DBG("truncating %lu..%lu\n", bs->length + 1, lbit);
		bitset_set_bits(bs, lbit, bs->length + 1, 0);
	}
}

#ifdef DEBUG_PRINT
static inline void print_bin(uint64_t x)
{
	int i;
	for (i = 0; i < 64; i++)
		putchar(((x >> (63 - i)) & 1) ? '1' : '0');
	putchar('\n');
}

void bitset_print(struct bitset *bs, const char *pref)
{
	int64_t i;
	printf("%s: ", pref);
	for (i = bs->cells - 1; i >= 0; --i)
		print_bin(bs->data[i]);
	putchar('\n');
}
#endif
