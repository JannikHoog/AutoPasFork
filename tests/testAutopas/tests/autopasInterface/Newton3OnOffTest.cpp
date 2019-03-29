/**
 * @file Newton3OnOffTest.cpp
 * @author seckler
 * @date 18.04.18
 */

#include "Newton3OnOffTest.h"
#include "autopas/utils/Logger.h"

using ::testing::_;  // anything is ok
using ::testing::Combine;
using ::testing::Return;  // anything is ok
using ::testing::ValuesIn;

// Parse combination strings and call actual test function
TEST_P(Newton3OnOffTest, countFunctorCallsTest) {
  auto contTravStr = std::get<0>(GetParam());
  auto dataLayoutStr = std::get<1>(GetParam());

  transform(contTravStr.begin(), contTravStr.end(), contTravStr.begin(), ::tolower);
  transform(dataLayoutStr.begin(), dataLayoutStr.end(), dataLayoutStr.begin(), ::tolower);
  auto containerOption = autopas::utils::StringUtils::parseContainerOptions(contTravStr)[0];
  auto traversalOption = autopas::utils::StringUtils::parseTraversalOptions(contTravStr)[0];
  auto dataLayoutOption = autopas::utils::StringUtils::parseDataLayout(dataLayoutStr)[0];
  countFunctorCalls(containerOption, traversalOption, dataLayoutOption);
}

// Generate Unittests for all Container / Traversal / Datalayout combinations
// Use strings such that generated test names are readable
INSTANTIATE_TEST_SUITE_P(
    Generated, Newton3OnOffTest,
    Combine(ValuesIn([]() -> std::vector<std::string> {
              // needed because CellBlock3D (called when building containers) logs always
              autopas::Logger::create();

              std::vector<std::string> ret;

              for (auto containerOption : autopas::allContainerOptions) {
                // @TODO: let verlet lists support Newton 3
                if (containerOption == autopas::ContainerOption::verletLists ||
                    containerOption == autopas::ContainerOption::verletListsCells ||
                    containerOption == autopas::ContainerOption::verletClusterLists) {
                  continue;
                }

                autopas::ContainerSelector<Particle, FPCell> containerSelector({0, 0, 0}, {10, 10, 10}, 1, 0, 10);

                containerSelector.selectContainer(containerOption);

                auto container = containerSelector.getCurrentContainer();

                for (auto traversalOption : container->getAllTraversals()) {
                  if (traversalOption == autopas::TraversalOption::c01 /*and autopas::autopas_get_max_threads() > 1*/) {
                    continue;
                  }

                  std::stringstream name;

                  name << autopas::utils::StringUtils::to_string(containerOption);
                  name << "+";
                  name << autopas::utils::StringUtils::to_string(traversalOption);

                  ret.push_back(name.str());
                }
              }

              autopas::Logger::unregister();

              return ret;
            }()),
            ValuesIn([]() {
              std::vector<std::string> ret;
              std::transform(
                  autopas::allDataLayoutOptions.begin(), autopas::allDataLayoutOptions.end(), std::back_inserter(ret),
                  [](autopas::DataLayoutOption d) -> std::string { return autopas::utils::StringUtils::to_string(d); });
              return ret;
            }())));

// Count number of Functor calls with and without newton 3 and compare
void Newton3OnOffTest::countFunctorCalls(autopas::ContainerOption containerOption,
                                         autopas::TraversalOption traversalOption,
                                         autopas::DataLayoutOption dataLayout) {
  autopas::ContainerSelector<Particle, FPCell> containerSelector(getBoxMin(), getBoxMax(), getCutoff(), getVerletSkin(),
                                                                 getVerletRebuildFrequency());

  containerSelector.selectContainer(containerOption);

  auto container = containerSelector.getCurrentContainer();
  auto traversalSelector = container->generateTraversalSelector();

  autopas::MoleculeLJ defaultParticle;
  RandomGenerator::fillWithParticles(*container, defaultParticle, 100);
  RandomGenerator::fillWithHaloParticles(*container, defaultParticle, container->getCutoff(), 10);

  EXPECT_CALL(mockFunctor, isRelevantForTuning()).WillRepeatedly(Return(true));

  if (dataLayout == autopas::DataLayoutOption::soa) {
    // loader and extractor will be called, we don't care how often.
    EXPECT_CALL(mockFunctor, SoALoader(_, _)).Times(testing::AtLeast(1));
    EXPECT_CALL(mockFunctor, SoAExtractor(_, _)).Times(testing::AtLeast(1));
  }

  // with newton 3:
  int callsNewton3SC = 0;    // same cell
  int callsNewton3Pair = 0;  // pair of cells
  EXPECT_CALL(mockFunctor, allowsNewton3()).WillRepeatedly(Return(true));
  EXPECT_CALL(mockFunctor, allowsNonNewton3()).WillRepeatedly(Return(false));

  switch (dataLayout) {
    case autopas::DataLayoutOption::soa: {
      // single cell
      EXPECT_CALL(mockFunctor, SoAFunctor(_, true)).WillRepeatedly(testing::InvokeWithoutArgs([&]() {
        callsNewton3SC++;
      }));
      EXPECT_CALL(mockFunctor, SoAFunctor(_, true)).Times(testing::AtLeast(1));

      // pair of cells
      EXPECT_CALL(mockFunctor, SoAFunctor(_, _, true)).WillRepeatedly(testing::InvokeWithoutArgs([&]() {
        callsNewton3Pair++;
      }));
      EXPECT_CALL(mockFunctor, SoAFunctor(_, _, true)).Times(testing::AtLeast(1));

      // non newton3 variant should not happen
      EXPECT_CALL(mockFunctor, SoAFunctor(_, _, false)).Times(0);
      iterate(
          container,
          traversalSelector.generateTraversal<MockFunctor<Particle, FPCell>, true, true>(traversalOption, mockFunctor),
          dataLayout, autopas::Newton3Option::enabled, &mockFunctor);
      break;
    }
    case autopas::DataLayoutOption::aos: {
      EXPECT_CALL(mockFunctor, AoSFunctor(_, _, true)).WillRepeatedly(testing::InvokeWithoutArgs([&]() {
        callsNewton3Pair++;
      }));
      EXPECT_CALL(mockFunctor, AoSFunctor(_, _, true)).Times(testing::AtLeast(1));
      // non newton3 variant should not happen
      EXPECT_CALL(mockFunctor, AoSFunctor(_, _, false)).Times(0);
      iterate(
          container,
          traversalSelector.generateTraversal<MockFunctor<Particle, FPCell>, false, true>(traversalOption, mockFunctor),
          dataLayout, autopas::Newton3Option::enabled, &mockFunctor);
      break;
    }
    default:
      FAIL() << "This test does not support data layout : " << autopas::utils::StringUtils::to_string(dataLayout);
  }

  // without newton 3:
  int callsNonNewton3SC = 0;
  int callsNonNewton3Pair = 0;
  EXPECT_CALL(mockFunctor, allowsNewton3()).WillRepeatedly(Return(false));
  EXPECT_CALL(mockFunctor, allowsNonNewton3()).WillRepeatedly(Return(true));

  switch (dataLayout) {
    case autopas::DataLayoutOption::soa: {
      // single cell
      EXPECT_CALL(mockFunctor, SoAFunctor(_, false)).WillRepeatedly(testing::InvokeWithoutArgs([&]() {
        callsNonNewton3SC++;
      }));
      EXPECT_CALL(mockFunctor, SoAFunctor(_, false)).Times(testing::AtLeast(1));

      // pair of cells
      EXPECT_CALL(mockFunctor, SoAFunctor(_, _, false)).WillRepeatedly(testing::InvokeWithoutArgs([&]() {
        callsNonNewton3Pair++;
      }));
      EXPECT_CALL(mockFunctor, SoAFunctor(_, _, false)).Times(testing::AtLeast(1));

      // newton3 variant should not happen
      EXPECT_CALL(mockFunctor, SoAFunctor(_, _, true)).Times(0);
      iterate(
          container,
          traversalSelector.generateTraversal<MockFunctor<Particle, FPCell>, true, false>(traversalOption, mockFunctor),
          dataLayout, autopas::Newton3Option::disabled, &mockFunctor);
      break;
    }
    case autopas::DataLayoutOption::aos: {
      EXPECT_CALL(mockFunctor, AoSFunctor(_, _, false)).WillRepeatedly(testing::InvokeWithoutArgs([&]() {
        callsNewton3Pair++;
      }));
      EXPECT_CALL(mockFunctor, AoSFunctor(_, _, false)).Times(testing::AtLeast(1));
      // newton3 variant should not happen
      EXPECT_CALL(mockFunctor, AoSFunctor(_, _, true)).Times(0);
      iterate(container,
              traversalSelector.generateTraversal<MockFunctor<Particle, FPCell>, false, false>(traversalOption,
                                                                                               mockFunctor),
              dataLayout, autopas::Newton3Option::disabled, &mockFunctor);
      break;
    }
    default:
      FAIL() << "This test does not support data layout : " << autopas::utils::StringUtils::to_string(dataLayout);
  }

  if (autopas::DataLayoutOption::soa) {
    // within one cell no N3 optimization
    EXPECT_EQ(callsNewton3SC, callsNonNewton3SC) << "for containeroption: " << containerOption;
  }
  // should be called exactly two times
  EXPECT_EQ(callsNewton3Pair * 2, callsNonNewton3Pair) << "for containeroption: " << containerOption;

  if (::testing::Test::HasFailure()) {
    std::cerr << "Failures for Container: " << autopas::utils::StringUtils::to_string(containerOption)
              << ", Traversal: " << autopas::utils::StringUtils::to_string(traversalOption)
              << ", Data Layout: " << autopas::utils::StringUtils::to_string(dataLayout) << std::endl;
  }
}

template <class ParticleFunctor, class Container, class Traversal>
void Newton3OnOffTest::iterate(Container container, Traversal traversal, autopas::DataLayoutOption dataLayout,
                               autopas::Newton3Option newton3, ParticleFunctor *f) {
  switch (dataLayout) {
    case autopas::DataLayoutOption::soa: {
      withStaticContainerType(container, [&](auto container) {
        container->iteratePairwiseSoA(f, traversal.get(), newton3 == autopas::Newton3Option::enabled);
      });
      break;
    }
    case autopas::DataLayoutOption::aos: {
      withStaticContainerType(container, [&](auto container) {
        container->iteratePairwiseAoS(f, traversal.get(), newton3 == autopas::Newton3Option::enabled);
      });
    } break;
  }
}