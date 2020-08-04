/**
 * @file LJFunctorCuda.cuh
 *
 * @date 14.12.2018
 * @author jspahl
 */

#pragma once

#include "autopas/pairwiseFunctors/FunctorCuda.cuh"
#include "autopas/particles/OwnershipState.h"
#include "cuda_runtime.h"

namespace autopas {

/**
 * Stores all pointers to the device Memory SoAs as needed by thqe LJ Functor
 * @tparam floatType of all vectors
 */
template <typename floatType>
class LJFunctorCudaSoA : public FunctorCudaSoA<floatType> {
 public:
  /**
   * Constructor for only positions
   * @param size Number of particles
   * @posX x positions of the particles
   * @posY y positions of the particles
   * @posZ z positions of the particles
   * @param ownershipState
   */
  LJFunctorCudaSoA(unsigned int size, floatType *posX, floatType *posY, floatType *posZ, OwnershipState *ownershipState)
      : _size(size),
        _posX(posX),
        _posY(posY),
        _posZ(posZ),
        _forceX(nullptr),
        _forceY(nullptr),
        _forceZ(nullptr),
        _ownershipState(ownershipState) {}

  /**
   * Constructor for only positions
   * @param size Number of particles
   * @param posX x positions of the particles
   * @param posY y positions of the particles
   * @param posZ z positions of the particles
   * @param forceX x forces of the particles
   * @param forceY y forces of the particles
   * @param forceZ z forces of the particles
   * @param ownershipState
   */
  LJFunctorCudaSoA(unsigned int size, floatType *posX, floatType *posY, floatType *posZ, floatType *forceX,
                   floatType *forceY, floatType *forceZ, OwnershipState *ownershipState)
      : _size(size),
        _posX(posX),
        _posY(posY),
        _posZ(posZ),
        _forceX(forceX),
        _forceY(forceY),
        _forceZ(forceZ),
        _ownershipState(ownershipState) {}

  /**
   * CopyConstructor
   * @param obj other object
   */
  LJFunctorCudaSoA(const LJFunctorCudaSoA &obj)
      : _size(obj._size),
        _posX(obj._posX),
        _posY(obj._posY),
        _posZ(obj._posZ),
        _forceX(obj._forceX),
        _forceY(obj._forceY),
        _forceZ(obj._forceZ),
        _ownershipState(obj._ownershipState) {}

  unsigned int _size;
  floatType *_posX;
  floatType *_posY;
  floatType *_posZ;
  floatType *_forceX;
  floatType *_forceY;
  floatType *_forceZ;
  OwnershipState *_ownershipState;
};

template <typename floatType>
class LJFunctorCudaWrapper : public CudaWrapperInterface<floatType> {
 public:
  LJFunctorCudaWrapper() { _num_threads = 64; }
  virtual ~LJFunctorCudaWrapper() {}

  void setNumThreads(int num_threads) override { _num_threads = num_threads; }

  void loadConstants(FunctorCudaConstants<floatType> *constants) override;

  void SoAFunctorNoN3Wrapper(FunctorCudaSoA<floatType> *cell1Base, cudaStream_t stream = 0) override;
  void SoAFunctorNoN3PairWrapper(FunctorCudaSoA<floatType> *cell1Base, FunctorCudaSoA<floatType> *cell2Base,
                                 cudaStream_t stream) override;

  void SoAFunctorN3Wrapper(FunctorCudaSoA<floatType> *cell1Base, cudaStream_t stream = 0) override;
  void SoAFunctorN3PairWrapper(FunctorCudaSoA<floatType> *cell1Base, FunctorCudaSoA<floatType> *cell2Base,
                               cudaStream_t stream) override;

  void LinkedCellsTraversalNoN3Wrapper(FunctorCudaSoA<floatType> *cell1Base, unsigned int reqThreads,
                                       unsigned int cids_size, unsigned int *cids, unsigned int cellSizes_size,
                                       size_t *cellSizes, cudaStream_t stream) override;

  void LinkedCellsTraversalN3Wrapper(FunctorCudaSoA<floatType> *cell1Base, unsigned int reqThreads,
                                     unsigned int cids_size, unsigned int *cids, unsigned int cellSizes_size,
                                     size_t *cellSizes, cudaStream_t stream) override;

  void CellVerletTraversalNoN3Wrapper(FunctorCudaSoA<floatType> *cell1Base, unsigned int ncells,
                                      unsigned int clusterSize, unsigned int others_size, unsigned int *other_ids,
                                      cudaStream_t stream) override;

  void CellVerletTraversalN3Wrapper(FunctorCudaSoA<floatType> *cell1Base, unsigned int ncells, unsigned int clusterSize,
                                    unsigned int others_size, unsigned int *other_ids, cudaStream_t stream) override;

  void loadLinkedCellsOffsets(unsigned int offsets_size, int *offsets) override;

  bool isAppropriateClusterSize(unsigned int clusterSize) const override {
    return clusterSize % 32 == 0 and clusterSize >= 32 and clusterSize <= 1024;
  }

 private:
  int numRequiredBlocks(int n) { return ((n - 1) / _num_threads) + 1; }

  int _num_threads;
};

}  // namespace autopas
