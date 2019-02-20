/**
 * @file TraversalSelectorTest.cpp
 * @author F. Gratl
 * @date 21.06.18
 */

#include "TraversalSelectorTest.h"

using ::testing::Return;

/**
 * Check if the only allowed traversal is returned
 */
TEST_F(TraversalSelectorTest, testSelectAndGetCurrentTraversal) {
  MFunctor functor;

  // this should be high enough so that sliced is still valid for the current processors thread count.
  constexpr size_t domainSize = 1000;
  autopas::TraversalSelector<FPCell> traversalSelector({domainSize, domainSize, domainSize});

  // expect an exception if nothing is selected yet
  EXPECT_THROW((traversalSelector.generateTraversal<MFunctor, false, false>(autopas::TraversalOption(-1), functor)),
               autopas::utils::ExceptionHandler::AutoPasException);

  for (auto &traversalOption : autopas::allTraversalOptions) {
    auto traversal = traversalSelector.generateTraversal<MFunctor, false, false>(traversalOption, functor);

    // check that traversals are of the expected type
    EXPECT_EQ(traversalOption, traversal->getTraversalType())
        << "Is the domain size large enough for the processors' thread count?";
  }
}
