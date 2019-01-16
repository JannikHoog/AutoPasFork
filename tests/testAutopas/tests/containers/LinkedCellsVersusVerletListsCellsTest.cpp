/**
 * @file LinkedCellsVersusVerletListsCellsTest.cpp
 * @author nguyen
 * @date 08.09.18
 */

#include "LinkedCellsVersusVerletListsCellsTest.h"

LinkedCellsVersusVerletListsCellsTest::LinkedCellsVersusVerletListsCellsTest()
    : _verletListsCells(getBoxMin(), getBoxMax(), getCutoff(), autopas::TraversalOptions::c18, 0.1 * getCutoff(), 2),
      _linkedCells(getBoxMin(), getBoxMax(), getCutoff()) {}

void LinkedCellsVersusVerletListsCellsTest::test(unsigned long numMolecules, double rel_err_tolerance) {
  RandomGenerator::fillWithParticles(_verletListsCells, autopas::MoleculeLJ({0., 0., 0.}, {0., 0., 0.}, 0),
                                     numMolecules);
  // now fill second container with the molecules from the first one, because
  // otherwise we generate new particles
  for (auto it = _verletListsCells.begin(); it.isValid(); ++it) {
    _linkedCells.addParticle(*it);
  }

  double eps = 1.0;
  double sig = 1.0;
  double shift = 0.0;
  autopas::MoleculeLJ::setEpsilon(eps);
  autopas::MoleculeLJ::setSigma(sig);
  autopas::LJFunctor<Molecule, FMCell> func(getCutoff(), eps, sig, shift);

  autopas::C18TraversalVerlet<FMCell, autopas::LJFunctor<Molecule, FMCell>, false, true> traversalVerletLJ(
      _verletListsCells.getCellsPerDimension(), &func);
  autopas::C18Traversal<FMCell, autopas::LJFunctor<Molecule, FMCell>, false, true> traversalLinkedLJ(
      _linkedCells.getCellBlock().getCellsPerDimensionWithHalo(), &func);
  _verletListsCells.iteratePairwiseAoS(&func, &traversalVerletLJ);
  _linkedCells.iteratePairwiseAoS(&func, &traversalLinkedLJ);

  std::vector<std::array<double, 3>> forcesDirect(numMolecules), forcesLinked(numMolecules);
  // get and sort by id
  for (auto it = _verletListsCells.begin(); it.isValid(); ++it) {
    autopas::MoleculeLJ &m = *it;
    forcesDirect.at(m.getID()) = m.getF();
  }

  for (auto it = _linkedCells.begin(); it.isValid(); ++it) {
    autopas::MoleculeLJ &m = *it;
    forcesLinked.at(m.getID()) = m.getF();
  }

  for (unsigned long i = 0; i < numMolecules; ++i) {
    for (int d = 0; d < 3; ++d) {
      double f1 = forcesDirect[i][d];
      double f2 = forcesLinked[i][d];
      double abs_err = std::abs(f1 - f2);
      double rel_err = std::abs(abs_err / f1);
      EXPECT_LT(rel_err, rel_err_tolerance);
    }
  }

  autopas::FlopCounterFunctor<Molecule, FMCell> flopsVerlet(getCutoff()), flopsLinked(getCutoff());
  autopas::C18TraversalVerlet<FMCell, autopas::FlopCounterFunctor<Molecule, FMCell>, false, true> traversalVerletFLOPS(
      _verletListsCells.getCellsPerDimension(), &flopsVerlet);
  autopas::C18Traversal<FMCell, autopas::FlopCounterFunctor<Molecule, FMCell>, false, true> traversalLinkedFLOPS(
      _linkedCells.getCellBlock().getCellsPerDimensionWithHalo(), &flopsLinked);
  _verletListsCells.iteratePairwiseAoS(&flopsVerlet, &traversalVerletFLOPS);
  _linkedCells.iteratePairwiseAoS(&flopsLinked, &traversalLinkedFLOPS);

  ASSERT_EQ(flopsLinked.getKernelCalls(), flopsVerlet.getKernelCalls());
  ASSERT_GE(flopsLinked.getDistanceCalculations(), flopsVerlet.getDistanceCalculations());
}

TEST_F(LinkedCellsVersusVerletListsCellsTest, test100) {
  unsigned long numMolecules = 100;

  // empirically determined and set near the minimal possible value
  // i.e. if something changes, it may be needed to increase value
  // (and OK to do so)
  double rel_err_tolerance = 1e-14;

  test(numMolecules, rel_err_tolerance);
}

TEST_F(LinkedCellsVersusVerletListsCellsTest, test1000) {
  unsigned long numMolecules = 1000;

  // empirically determined and set near the minimal possible value
  // i.e. if something changes, it may be needed to increase value
  // (and OK to do so)
  double rel_err_tolerance = 2e-12;
  test(numMolecules, rel_err_tolerance);
}

TEST_F(LinkedCellsVersusVerletListsCellsTest, test2000) {
  unsigned long numMolecules = 2000;

  // empirically determined and set near the minimal possible value
  // i.e. if something changes, it may be needed to increase value
  // (and OK to do so)
  double rel_err_tolerance = 1e-10;
  test(numMolecules, rel_err_tolerance);
}
