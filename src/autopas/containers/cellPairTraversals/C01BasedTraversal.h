/**
 * @file C01BasedTraversal.h
 * @author nguyen
 * @date 16.09.18
 */

#pragma once

#include <autopas/utils/WrapOpenMP.h>
#include "autopas/containers/cellPairTraversals/CellPairTraversal.h"
#include "autopas/utils/ThreeDimensionalMapping.h"

namespace autopas {

/**
 * This class provides the base for traversals using the c01 base step.
 *
 * The traversal is defined in the function c01Traversal and uses 1 color. Interactions between two cells are allowed
 * only if particles of the first cell are modified. This means that newton3 optimizations are NOT allowed.
 *
 * @tparam ParticleCell the type of cells
 * @tparam PairwiseFunctor The functor that defines the interaction of two particles.
 * @tparam useSoA
 */
template <class ParticleCell, class PairwiseFunctor, bool useSoA, bool useNewton3>
class C01BasedTraversal : public CellPairTraversal<ParticleCell> {
 public:
  /**
   * Constructor of the c01 traversal.
   * @param dims The dimensions of the cellblock, i.e. the number of cells in x,
   * y and z direction.
   * @param pairwiseFunctor The functor that defines the interaction of two particles.
   */
  explicit C01BasedTraversal(const std::array<unsigned long, 3>& dims, PairwiseFunctor* pairwiseFunctor)
      : CellPairTraversal<ParticleCell>(dims) {}

  /**
   * C01 traversals are only usable if useNewton3 is disabled.
   *
   * This is because the cell functor in the c01 traversal is hardcoded to not allow newton 3 even if only one thread is
   * used.
   *
   * @return
   */
  bool isApplicable() override { return not useNewton3; }

 protected:
  /**
   * The main traversal of the C01Traversal.
   * This provides the structure of the loops and its parallelization.
   * @tparam LoopBody
   * @param loopBody The body of the loop as a function. Normally a lambda function, that takes as as parameters
   * (x,y,z). If you need additional input from outside, please use captures (by reference).
   */
  template <typename LoopBody>
  inline void c01Traversal(LoopBody&& loopBody);
};

template <class ParticleCell, class PairwiseFunctor, bool useSoA, bool useNewton3>
template <typename LoopBody>
inline void C01BasedTraversal<ParticleCell, PairwiseFunctor, useSoA, useNewton3>::c01Traversal(LoopBody&& loopBody) {
  const unsigned long end_x = this->_cellsPerDimension[0] - 1;
  const unsigned long end_y = this->_cellsPerDimension[1] - 1;
  const unsigned long end_z = this->_cellsPerDimension[2] - 1;

#if defined(AUTOPAS_OPENMP)
  // @todo: find optimal chunksize
#pragma omp parallel for schedule(dynamic) collapse(3)
#endif
  for (unsigned long z = 1; z < end_z; ++z) {
    for (unsigned long y = 1; y < end_y; ++y) {
      for (unsigned long x = 1; x < end_x; ++x) {
        loopBody(x, y, z);
      }
    }
  }
}

}  // namespace autopas