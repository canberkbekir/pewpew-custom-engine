/*
 * TriBox.h
 * Triangle-Box overlap test
 * Based on Tomas Akenine-Möller's algorithm
 *
 * Original: https://github.com/topskychen/voxelizer
 * Simplified for PewPew Engine - no external dependencies
 */

#ifndef VOXELIZER_TRIBOX_H
#define VOXELIZER_TRIBOX_H

namespace voxelizer {

// Tests if a triangle overlaps with an axis-aligned bounding box
// boxcenter: center of the AABB
// boxhalfsize: half-extents of the AABB
// triverts: 3 vertices of the triangle (triverts[0], triverts[1], triverts[2])
// Returns: 1 if overlap, 0 if no overlap
int TriBoxOverlap(float boxcenter[3], float boxhalfsize[3], float triverts[3][3]);

} // namespace voxelizer

#endif // VOXELIZER_TRIBOX_H
