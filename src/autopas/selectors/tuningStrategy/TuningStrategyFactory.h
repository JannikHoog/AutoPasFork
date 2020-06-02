/**
 * @file TuningStrategyFactory.h
 * @author seckler
 * @date 07.02.2020
 */

#pragma once

#include "TuningStrategyInterface.h"
#include "autopas/options/AcquisitionFunctionOption.h"
#include "autopas/options/TuningStrategyOption.h"
#include "autopas/options/MPIStrategyOption.h"
#include "autopas/utils/WrapMPI.h"

namespace autopas::TuningStrategyFactory {
/**
 * Generates a new Tuning Strategy object.
 * @param tuningStrategyOption
 * @param allowedContainers
 * @param allowedCellSizeFactors
 * @param allowedTraversals
 * @param allowedDataLayouts
 * @param allowedNewton3Options
 * @param maxEvidence
 * @param relativeOptimum
 * @param maxTuningPhasesWithoutTest
 * @param acquisitionFunctionOption
 * @param comm Default value only used for tests
 * @return Pointer to the tuning strategy object or the nullpointer if an exception was suppressed.
 */
std::unique_ptr<autopas::TuningStrategyInterface> generateTuningStrategy(
    autopas::TuningStrategyOption tuningStrategyOption,  std::set<autopas::ContainerOption> &allowedContainers,
    autopas::NumberSet<double> &allowedCellSizeFactors,  std::set<autopas::TraversalOption> &allowedTraversals,
    std::set<autopas::DataLayoutOption> &allowedDataLayouts,
    std::set<autopas::Newton3Option> &allowedNewton3Options, unsigned int maxEvidence, double relativeOptimum,
    unsigned int maxTuningPhasesWithoutTest, AcquisitionFunctionOption acquisitionFunctionOption,
    MPIStrategyOption mpiStrategyOption, AutoPas_MPI_Comm comm = AUTOPAS_MPI_COMM_WORLD);
}  // namespace autopas::TuningStrategyFactory
