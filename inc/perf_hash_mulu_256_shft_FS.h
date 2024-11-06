#ifndef _TAB_MULU_DIST_256_SHFT_FS_H_
#define _TAB_MULU_DIST_256_SHFT_FS_H_

#include <types.h>

#define MPH_VALUES_DELTADIST_NKEYS 915 // How many keys were hashed

/// @brief Using a perfect hash function over op2 to get an index between 0 and MPH_VALUES_DELTADIST_NKEYS-1 to get
/// the multiplication result by op1, including the final >> FS.
/// op1 value is in [0, 256].
/// op2 value is one of the extracted values from tab_deltas[] columns 0 or 1 (depending on axis X or Y).
/// @param op1_rowStride This is one of: sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1. And is expected to be
/// multiplied by MPH_VALUES_DELTADIST_NKEYS and by 2 (for faster tab_mulu_dist_256_shft_FS[] access in ASM)
/// @param op2_index index of op2 in the row, multiplied by 2 (for faster tab_mulu_dist_256_shft_FS[] access in ASM).
/// @return Same than: (op1*op2) >> FS
u16 perf_hash_mulu_shft_FS (u32 op1_rowStride, u16 op2_index);

#endif // _TAB_MULU_DIST_256_SHFT_FS_H_