/**
 * @file TraversalSelectorTest.cpp
 * @author F. Gratl
 * @date 21.06.18
 */

#include "TraversalSelectorTest.h"

/**
 * Check if the only allowed traversal is returned
 */
TEST_F(TraversalSelectorTest, testGetOptimalTraversalOneOption) {
  MFunctor functor;

  std::vector<autopas::TraversalOptions> optionVectorC08 = {autopas::TraversalOptions::c08};
  std::vector<autopas::TraversalOptions> optionVectorSliced = {autopas::TraversalOptions::sliced};

  // this should be high enough so that sliced is still valid for the current processors thread count.
  constexpr size_t domainSize = 1000;

  autopas::TraversalSelector<FPCell> traversalSelectorC08({domainSize, domainSize, domainSize}, optionVectorC08);
  autopas::TraversalSelector<FPCell> traversalSelectorSlice({domainSize, domainSize, domainSize}, optionVectorSliced);
  auto traversalC08 = traversalSelectorC08.getOptimalTraversal<MFunctor, false, true>(functor);
  auto traversalSlice = traversalSelectorSlice.getOptimalTraversal<MFunctor, false, true>(functor);

  // check that traversals are of the expected type
  EXPECT_TRUE((dynamic_cast<autopas::C08Traversal<FPCell, MFunctor, false, true> *>(traversalC08.get())));
  EXPECT_TRUE((dynamic_cast<autopas::SlicedTraversal<FPCell, MFunctor, false, true> *>(traversalSlice.get())))
      << "Is the domain size large enough for the processors' thread count?";

  // now that the functor is known check if still the same is returned
  traversalC08 = traversalSelectorC08.getOptimalTraversal<MFunctor, false, true>(functor);
  traversalSlice = traversalSelectorSlice.getOptimalTraversal<MFunctor, false, true>(functor);
  // check that traversals are of the expected type
  EXPECT_TRUE((dynamic_cast<autopas::C08Traversal<FPCell, MFunctor, false, true> *>(traversalC08.get())))
      << "Repeated call for c08 failed";
  EXPECT_TRUE((dynamic_cast<autopas::SlicedTraversal<FPCell, MFunctor, false, true> *>(traversalSlice.get())))
      << "Repeated call for sliced failed";
}

TEST_F(TraversalSelectorTest, testGetOptimalTraversalBadFirstOption) {
  MFunctor functor;

  std::vector<autopas::TraversalOptions> optionVector = {autopas::TraversalOptions::sliced,
                                                         autopas::TraversalOptions::c08};

  autopas::TraversalSelector<FPCell> traversalSelectorC08({1, 1, 1}, optionVector);
  auto traversal = traversalSelectorC08.getOptimalTraversal<MFunctor, false, true>(functor);

  // check that traversals are of the expected type
  EXPECT_TRUE((dynamic_cast<autopas::C08Traversal<FPCell, MFunctor, false, true> *>(traversal.get())));

  // also after the functor is known
  traversal = traversalSelectorC08.getOptimalTraversal<MFunctor, false, true>(functor);
  EXPECT_TRUE((dynamic_cast<autopas::C08Traversal<FPCell, MFunctor, false, true> *>(traversal.get())));
}