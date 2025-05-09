/* Generic streaming support for various data types.

   Copyright (C) 2011-2025 Free Software Foundation, Inc.
   Contributed by Diego Novillo <dnovillo@google.com>

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_DATA_STREAMER_H
#define GCC_DATA_STREAMER_H

#include "lto-streamer.h"

/* Data structures used to pack values and bitflags into a vector of
   words.  Used to stream values of a fixed number of bits in a space
   efficient way.  */
static unsigned const BITS_PER_BITPACK_WORD = HOST_BITS_PER_WIDE_INT;

typedef unsigned HOST_WIDE_INT bitpack_word_t;

struct bitpack_d
{
  /* The position of the first unused or unconsumed bit in the word.  */
  unsigned pos;

  /* The current word we are (un)packing.  */
  bitpack_word_t word;

  /* The lto_output_stream or the lto_input_block we are streaming to/from.  */
  void *stream;
};

/* In data-streamer.cc  */
void bp_pack_var_len_unsigned (struct bitpack_d *, unsigned HOST_WIDE_INT);
void bp_pack_var_len_int (struct bitpack_d *, HOST_WIDE_INT);
void bp_pack_real_value (struct bitpack_d *, const REAL_VALUE_TYPE *);
void bp_unpack_real_value (struct bitpack_d *, REAL_VALUE_TYPE *);
unsigned HOST_WIDE_INT bp_unpack_var_len_unsigned (struct bitpack_d *);
HOST_WIDE_INT bp_unpack_var_len_int (struct bitpack_d *);
extern unsigned host_num_poly_int_coeffs;

/* In data-streamer-out.cc  */
void streamer_write_zero (struct output_block *);
void streamer_write_uhwi (struct output_block *, unsigned HOST_WIDE_INT);
void streamer_write_hwi (struct output_block *, HOST_WIDE_INT);
void streamer_write_poly_uint64 (struct output_block *, poly_uint64);
void streamer_write_poly_int64 (struct output_block *, poly_int64);
void streamer_write_gcov_count (struct output_block *, gcov_type);
void streamer_write_string (struct output_block *, struct lto_output_stream *,
			    const char *, bool);
void streamer_write_string_with_length (struct output_block *,
					struct lto_output_stream *,
					const char *, unsigned int, bool);
void bp_pack_string_with_length (struct output_block *, struct bitpack_d *,
				 const char *, unsigned int, bool);
void bp_pack_string (struct output_block *, struct bitpack_d *,
		     const char *, bool);
void streamer_write_uhwi_stream (struct lto_output_stream *,
				 unsigned HOST_WIDE_INT);
void streamer_write_hwi_stream (struct lto_output_stream *, HOST_WIDE_INT);
void streamer_write_gcov_count_stream (struct lto_output_stream *, gcov_type);
void streamer_write_data_stream (struct lto_output_stream *, const void *,
				 size_t);
void streamer_write_wide_int (struct output_block *, const wide_int &);
void streamer_write_widest_int (struct output_block *, const widest_int &);
void streamer_write_vrange (struct output_block *, const class vrange &);

/* In data-streamer-in.cc  */
const char *streamer_read_string (class data_in *, class lto_input_block *);
const char *streamer_read_indexed_string (class data_in *,
					  class lto_input_block *,
					  unsigned int *);
const char *bp_unpack_indexed_string (class data_in *, struct bitpack_d *,
				      unsigned int *);
const char *bp_unpack_string (class data_in *, struct bitpack_d *);
unsigned HOST_WIDE_INT streamer_read_uhwi (class lto_input_block *);
HOST_WIDE_INT streamer_read_hwi (class lto_input_block *);
poly_uint64 streamer_read_poly_uint64 (class lto_input_block *);
poly_int64 streamer_read_poly_int64 (class lto_input_block *);
gcov_type streamer_read_gcov_count (class lto_input_block *);
wide_int streamer_read_wide_int (class lto_input_block *);
widest_int streamer_read_widest_int (class lto_input_block *);
void streamer_read_value_range (class lto_input_block *, class data_in *,
				class value_range &);

/* Returns a new bit-packing context for bit-packing into S.  */
inline struct bitpack_d
bitpack_create (struct lto_output_stream *s)
{
  struct bitpack_d bp;
  bp.pos = 0;
  bp.word = 0;
  bp.stream = (void *)s;
  return bp;
}

/* Pack the NBITS bit sized value VAL into the bit-packing context BP.  */
inline void
bp_pack_value (struct bitpack_d *bp, bitpack_word_t val, unsigned nbits)
{
  bitpack_word_t word = bp->word;
  int pos = bp->pos;

  /* Verify that VAL fits in the NBITS.  */
  gcc_checking_assert (nbits == BITS_PER_BITPACK_WORD
		       || !(val & ~(((bitpack_word_t)1<<nbits)-1)));

  /* If val does not fit into the current bitpack word switch to the
     next one.  */
  if (pos + nbits > BITS_PER_BITPACK_WORD)
    {
      streamer_write_uhwi_stream ((struct lto_output_stream *) bp->stream,
				  word);
      word = val;
      pos = nbits;
    }
  else
    {
      word |= val << pos;
      pos += nbits;
    }
  bp->word = word;
  bp->pos = pos;
}

/* Pack VAL into the bit-packing context BP, using NBITS for each
   coefficient.  */
inline void
bp_pack_poly_value (struct bitpack_d *bp,
		    const poly_int<NUM_POLY_INT_COEFFS, bitpack_word_t> &val,
		    unsigned nbits)
{
  for (int i = 0; i < NUM_POLY_INT_COEFFS; ++i)
    bp_pack_value (bp, val.coeffs[i], nbits);
}

/* Finishes bit-packing of BP.  */
inline void
streamer_write_bitpack (struct bitpack_d *bp)
{
  streamer_write_uhwi_stream ((struct lto_output_stream *) bp->stream,
			      bp->word);
  bp->word = 0;
  bp->pos = 0;
}

/* Returns a new bit-packing context for bit-unpacking from IB.  */
inline struct bitpack_d
streamer_read_bitpack (class lto_input_block *ib)
{
  struct bitpack_d bp;
  bp.word = streamer_read_uhwi (ib);
  bp.pos = 0;
  bp.stream = (void *)ib;
  return bp;
}

/* Unpacks NBITS bits from the bit-packing context BP and returns them.  */
inline bitpack_word_t
bp_unpack_value (struct bitpack_d *bp, unsigned nbits)
{
  bitpack_word_t mask, val;
  int pos = bp->pos;

  mask = (nbits == BITS_PER_BITPACK_WORD
	  ? (bitpack_word_t) -1
	  : ((bitpack_word_t) 1 << nbits) - 1);

  /* If there are not continuous nbits in the current bitpack word
     switch to the next one.  */
  if (pos + nbits > BITS_PER_BITPACK_WORD)
    {
      bp->word = val
	= streamer_read_uhwi ((class lto_input_block *)bp->stream);
      bp->pos = nbits;
      return val & mask;
    }
  val = bp->word;
  val >>= pos;
  bp->pos = pos + nbits;

  return val & mask;
}

/* Common code for reading poly_int.  */

template<typename C, typename F, typename ...Args>
poly_int<NUM_POLY_INT_COEFFS, C>
poly_int_read_common (F read_coeff, Args ...args)
{
  poly_int<NUM_POLY_INT_COEFFS, C> x;
  unsigned i;

#ifdef ACCEL_COMPILER
  /* Ensure that we have streamed-in host_num_poly_int_coeffs.  */
  const unsigned num_poly_int_coeffs = host_num_poly_int_coeffs;
  gcc_assert (host_num_poly_int_coeffs > 0);
#else
  const unsigned num_poly_int_coeffs = NUM_POLY_INT_COEFFS;
#endif

  if (num_poly_int_coeffs <= NUM_POLY_INT_COEFFS)
    {
      for (i = 0; i < num_poly_int_coeffs; i++)
	x.coeffs[i] = read_coeff (args...);
      for (; i < NUM_POLY_INT_COEFFS; i++)
	x.coeffs[i] = 0;
    }
  else
    {
      for (i = 0; i < NUM_POLY_INT_COEFFS; i++)
	x.coeffs[i] = read_coeff (args...);

      /* Ensure that degree of poly_int <= accel NUM_POLY_INT_COEFFS.  */
      for (; i < num_poly_int_coeffs; i++)
	{
	  C val = read_coeff (args...);
	  if (val != 0)
	    fatal_error (input_location,
			 "degree of %<poly_int%> exceeds "
			 "%<NUM_POLY_INT_COEFFS%> (%d)",
			 NUM_POLY_INT_COEFFS);
	}
    }
  return x;
}

/* Unpacks a polynomial value from the bit-packing context BP in which each
   coefficient has NBITS bits.  */
inline poly_int<NUM_POLY_INT_COEFFS, bitpack_word_t>
bp_unpack_poly_value (struct bitpack_d *bp, unsigned nbits)
{
  return poly_int_read_common<bitpack_word_t> (bp_unpack_value, bp, nbits);
}


/* Write a character to the output block.  */

inline void
streamer_write_char_stream (struct lto_output_stream *obs, char c)
{
  /* No space left.  */
  if (obs->left_in_block == 0)
    lto_append_block (obs);

  /* Write the actual character.  */
  char *current_pointer = obs->current_pointer;
  *(current_pointer++) = c;
  obs->current_pointer = current_pointer;
  obs->total_size++;
  obs->left_in_block--;
}


/* Read byte from the input block.  */

inline unsigned char
streamer_read_uchar (class lto_input_block *ib)
{
  if (ib->p >= ib->len)
    lto_section_overrun (ib);
  return (ib->data[ib->p++]);
}

/* Output VAL into OBS and verify it is in range MIN...MAX that is supposed
   to be compile time constant.
   Be host independent, limit range to 31bits.  */

inline void
streamer_write_hwi_in_range (struct lto_output_stream *obs,
				  HOST_WIDE_INT min,
				  HOST_WIDE_INT max,
				  HOST_WIDE_INT val)
{
  HOST_WIDE_INT range = max - min;

  gcc_checking_assert (val >= min && val <= max && range > 0
		       && range < 0x7fffffff);

  val -= min;
  streamer_write_uhwi_stream (obs, (unsigned HOST_WIDE_INT) val);
}

/* Input VAL into OBS and verify it is in range MIN...MAX that is supposed
   to be compile time constant.  PURPOSE is used for error reporting.  */

inline HOST_WIDE_INT
streamer_read_hwi_in_range (class lto_input_block *ib,
				 const char *purpose,
				 HOST_WIDE_INT min,
				 HOST_WIDE_INT max)
{
  HOST_WIDE_INT range = max - min;
  unsigned HOST_WIDE_INT uval = streamer_read_uhwi (ib);

  gcc_checking_assert (range > 0 && range < 0x7fffffff);

  HOST_WIDE_INT val = (HOST_WIDE_INT) (uval + (unsigned HOST_WIDE_INT) min);
  if (val < min || val > max)
    lto_value_range_error (purpose, val, min, max);
  return val;
}

/* Output VAL into BP and verify it is in range MIN...MAX that is supposed
   to be compile time constant.
   Be host independent, limit range to 31bits.  */

inline void
bp_pack_int_in_range (struct bitpack_d *bp,
		      HOST_WIDE_INT min,
		      HOST_WIDE_INT max,
		      HOST_WIDE_INT val)
{
  HOST_WIDE_INT range = max - min;
  int nbits = floor_log2 (range) + 1;

  gcc_checking_assert (val >= min && val <= max && range > 0
		       && range < 0x7fffffff);

  val -= min;
  bp_pack_value (bp, val, nbits);
}

/* Input VAL into BP and verify it is in range MIN...MAX that is supposed
   to be compile time constant.  PURPOSE is used for error reporting.  */

inline HOST_WIDE_INT
bp_unpack_int_in_range (struct bitpack_d *bp,
		        const char *purpose,
		        HOST_WIDE_INT min,
		        HOST_WIDE_INT max)
{
  HOST_WIDE_INT range = max - min;
  int nbits = floor_log2 (range) + 1;
  HOST_WIDE_INT val = bp_unpack_value (bp, nbits);

  gcc_checking_assert (range > 0 && range < 0x7fffffff);

  if (val < min || val > max)
    lto_value_range_error (purpose, val, min, max);
  return val;
}

/* Output VAL of type "enum enum_name" into OBS.
   Assume range 0...ENUM_LAST - 1.  */
#define streamer_write_enum(obs,enum_name,enum_last,val) \
  streamer_write_hwi_in_range ((obs), 0, (int)(enum_last) - 1, (int)(val))

/* Input enum of type "enum enum_name" from IB.
   Assume range 0...ENUM_LAST - 1.  */
#define streamer_read_enum(ib,enum_name,enum_last) \
  (enum enum_name)streamer_read_hwi_in_range ((ib), #enum_name, 0, \
					      (int)(enum_last) - 1)

/* Output VAL of type "enum enum_name" into BP.
   Assume range 0...ENUM_LAST - 1.  */
#define bp_pack_enum(bp,enum_name,enum_last,val) \
  bp_pack_int_in_range ((bp), 0, (int)(enum_last) - 1, (int)(val))

/* Input enum of type "enum enum_name" from BP.
   Assume range 0...ENUM_LAST - 1.  */
#define bp_unpack_enum(bp,enum_name,enum_last) \
  (enum enum_name)bp_unpack_int_in_range ((bp), #enum_name, 0, \
					(int)(enum_last) - 1)

/* Output the start of a record with TAG to output block OB.  */

inline void
streamer_write_record_start (struct output_block *ob, enum LTO_tags tag)
{
  streamer_write_enum (ob->main_stream, LTO_tags, LTO_NUM_TAGS, tag);
}

/* Return the next tag in the input block IB.  */

inline enum LTO_tags
streamer_read_record_start (class lto_input_block *ib)
{
  return streamer_read_enum (ib, LTO_tags, LTO_NUM_TAGS);
}

#endif  /* GCC_DATA_STREAMER_H  */
