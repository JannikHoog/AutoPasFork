/**
 * @file LeavingParticleCollectorTest.cpp
 * @author F. Gratl
 * @date 06.02.24
 */

#include "LeavingParticleCollectorTest.h"

#include <gtest/gtest.h>

#include "autopas/containers/LeavingParticleCollector.h"
#include "autopas/containers/linkedCells/LinkedCells.h"
#include "testingHelpers/commonTypedefs.h"

/**
 *
 */
TEST_F(LeavingParticleCollectorTest, testCalculateHaloBoxes) {
  using namespace autopas::utils::ArrayMath::literals;
  using autopas::utils::ArrayUtils::to_string;

  constexpr double cutoff = 4;
  constexpr double skinPerTimestep = .1;
  constexpr size_t rebuildFrequency = 20;
  constexpr double skin = skinPerTimestep * rebuildFrequency;  // = 2
  constexpr double interactionLength = cutoff + skin;          // = 6
  const std::array<double, 3> boxMin = {0.0, 0.0, 0.0};
  // 3x3x3 cells without halo
  const std::array<double, 3> boxMax = {1 * interactionLength, 1 * interactionLength, 1 * interactionLength};  // = 60

  const autopas::LinkedCells<Molecule> linkedCells(boxMin, boxMax, cutoff, skinPerTimestep, rebuildFrequency);
  const auto haloBoxes = autopas::LeavingParticleCollector::calculateHaloVolumes(linkedCells);
  // Sanity check that box corner coordinates are good:
  for (const auto &[lowerCorner, upperCorner] : haloBoxes) {
    ASSERT_EQ(lowerCorner.size(), upperCorner.size()) << "lowerCorner and upperCorner have different dimensions!";
    for (int i = 0; i < lowerCorner.size(); ++i) {
      EXPECT_LE(lowerCorner[i], upperCorner[i])
          << "In dimension " << i << "lower corner is not lower than upper corner!";
    }
  }

  // Halo boxes must not overlap
  // make sure boxes don't overlap
  for (int i = 0; i < haloBoxes.size(); ++i) {
    for (int j = i + 1; j < haloBoxes.size(); ++j) {
      const auto &[outerBoxMin, outerBoxMax] = haloBoxes[i];
      const auto &[innerBoxMin, innerBoxMax] = haloBoxes[j];

      EXPECT_FALSE(autopas::utils::boxesOverlap(outerBoxMin, outerBoxMax, innerBoxMin, innerBoxMax))
          << "Halo boxes overlap!\n"
          << i << ": " << to_string(outerBoxMin) << "-" << to_string(outerBoxMax) << "\n"
          << j << ": " << to_string(innerBoxMin) << "-" << to_string(innerBoxMax);
    }
  }

  // Halo boxes must not reach further into the inner box than half skin
  const auto upperBoundForMisplacement = skin / 2.0;
  const auto lowerInnerCorner = boxMin + upperBoundForMisplacement;
  const auto upperInnerCorner = boxMax - upperBoundForMisplacement;
  for (int i = 0; i < haloBoxes.size(); ++i) {
    const auto &[haloBoxMin, haloBoxMax] = haloBoxes[i];
    EXPECT_FALSE(autopas::utils::boxesOverlap(haloBoxMin, haloBoxMax, lowerInnerCorner, upperInnerCorner))
        << "Halo box " << i << " reaches into the inner box!\n"
        << "Inner box: " << to_string(lowerInnerCorner) << "-" << to_string(upperInnerCorner) << "\n"
        << "Halo box " << i << ": " << to_string(haloBoxMin) << "-" << to_string(haloBoxMax);
  }

  // Useful debug output in case something fails
  if (::testing::Test::HasFailure()) {
    using autopas::utils::ArrayUtils::operator<<;
    const auto lowerHaloCorner = boxMin - interactionLength;
    const auto upperHaloCorner = boxMax + interactionLength;
    std::cout << "Box Size        : " << (linkedCells.getBoxMax() - linkedCells.getBoxMin()) << "\n";
    std::cout << "lowerHaloCorner : " << lowerHaloCorner << "\n";
    std::cout << "lowerInnerCorner: " << lowerInnerCorner << "\n";
    std::cout << "upperInnerCorner: " << upperInnerCorner << "\n";
    std::cout << "upperHaloCorner : " << upperHaloCorner << "\n";
  }
}