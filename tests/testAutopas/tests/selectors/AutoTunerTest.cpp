/**
 * @file AutoTunerTest.cpp
 * @author F. Gratl
 * @date 8/10/18
 */

#include "AutoTunerTest.h"
#include "autopas/selectors/AutoTuner.h"

using ::testing::_;

TEST_F(AutoTunerTest, testAllConfigurations) {
  std::array<double, 3> bBoxMin = {0, 0, 0}, bBoxMax = {10, 10, 42};
  // adaptive domain size so sliced is always applicable.
  bBoxMax[2] = autopas::autopas_get_max_threads() * 2;
  const double cutoff = 1;
  const double verletSkin = 0;
  const unsigned int verletRebuildFrequency = 1;
  const unsigned int maxSamples = 2;

  autopas::LJFunctor<Particle, FPCell> functor(cutoff, 1., 1., 0.);
  autopas::AutoTuner<Particle, FPCell> autoTuner(bBoxMin, bBoxMax, cutoff, verletSkin, verletRebuildFrequency,
                                                 autopas::allContainerOptions, autopas::allTraversalOptions,
                                                 autopas::allDataLayoutOptions, autopas::allNewton3Options,
                                                 autopas::SelectorStrategy::fastestAbs, 100, maxSamples);

  autopas::Logger::get()->set_level(autopas::Logger::LogLevel::off);
  //  autopas::Logger::get()->set_level(autopas::Logger::LogLevel::debug);
  bool stillTuning = true;
  auto prevConfig = autopas::Configuration(autopas::ContainerOption(-1), autopas::TraversalOption(-1),
                                           autopas::DataLayoutOption(-1), autopas::Newton3Option(-1));

  // total number of possible configurations * number of samples + last iteration after tuning
  // number of configs manually counted
  size_t expectedNumberOfIterations = 34 * maxSamples + 1;

  int collectedSamples = 0;
  int iterations = 0;
  while (stillTuning) {
    if (collectedSamples == maxSamples) {
      collectedSamples = 0;
    }

    stillTuning = autoTuner.iteratePairwise(&functor);
    ++iterations;
    ++collectedSamples;

    auto currentConfig = autoTuner.getCurrentConfig();
    if (collectedSamples == 1) {
      EXPECT_NE(currentConfig, prevConfig);
    } else {
      EXPECT_EQ(currentConfig, prevConfig);
    }
    prevConfig = currentConfig;
  }

  EXPECT_EQ(expectedNumberOfIterations, iterations);
}

TEST_F(AutoTunerTest, testWillRebuildDDL) {
  // also check if rebuild is detected if next config is invalid

  std::set<autopas::Configuration> configs;
  configs.emplace(autopas::ContainerOption::directSum, autopas::TraversalOption::directSumTraversal,
                  autopas::DataLayoutOption::aos, autopas::Newton3Option::disabled);
  configs.emplace(autopas::ContainerOption::directSum, autopas::TraversalOption::directSumTraversal,
                  autopas::DataLayoutOption::aos, autopas::Newton3Option::enabled);
  configs.emplace(autopas::ContainerOption::linkedCells, autopas::TraversalOption::c08, autopas::DataLayoutOption::aos,
                  autopas::Newton3Option::disabled);

  autopas::AutoTuner<Particle, FPCell> autoTuner({0, 0, 0}, {10, 10, 10}, 1, 0, 100, configs,
                                                 autopas::SelectorStrategy::fastestAbs, 1000, 2);

  EXPECT_EQ(*(configs.begin()), autoTuner.getCurrentConfig());

  MockFunctor<Particle, FPCell> functor;
  EXPECT_CALL(functor, isRelevantForTuning()).WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(functor, allowsNewton3()).WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(functor, allowsNonNewton3()).WillRepeatedly(::testing::Return(true));

  // Intended false positive
  EXPECT_TRUE(autoTuner.willRebuild()) << "Expect rebuild for first iteration.";
  autoTuner.iteratePairwise(&functor);  // DS NoN3
  EXPECT_FALSE(autoTuner.willRebuild()) << "Expect no rebuild because more samples needed.";
  autoTuner.iteratePairwise(&functor);  // DS NoN3
  // Intended false positive
  EXPECT_TRUE(autoTuner.willRebuild()) << "Expect rebuild because we change config.";
  autoTuner.iteratePairwise(&functor);  // DS N3
  EXPECT_FALSE(autoTuner.willRebuild()) << "Expect no rebuild because more samples needed.";
  autoTuner.iteratePairwise(&functor);  // DS N3
  EXPECT_TRUE(autoTuner.willRebuild()) << "Expect rebuild because we change config.";
  autoTuner.iteratePairwise(&functor);  // LC NoN3
  EXPECT_FALSE(autoTuner.willRebuild()) << "Expect no rebuild because more samples needed.";
  autoTuner.iteratePairwise(&functor);  // LC NoN3
  EXPECT_TRUE(autoTuner.willRebuild()) << "Expect rebuild because reached end of tuning phase.";
  autoTuner.iteratePairwise(&functor);  // optimum
  EXPECT_FALSE(autoTuner.willRebuild()) << "Expect no rebuild because not tuning.";
}

/**
 * This test simulates that the next config (which is checked by willRebuild) is invalid.
 */
TEST_F(AutoTunerTest, testWillRebuildDDLOneConfigKicked) {
  // also check if rebuild is detected if next config is invalid

  std::set<autopas::Configuration> configs;
  configs.emplace(autopas::ContainerOption::directSum, autopas::TraversalOption::directSumTraversal,
                  autopas::DataLayoutOption::aos, autopas::Newton3Option::enabled);
  configs.emplace(autopas::ContainerOption::directSum, autopas::TraversalOption::directSumTraversal,
                  autopas::DataLayoutOption::aos, autopas::Newton3Option::disabled);
  configs.emplace(autopas::ContainerOption::linkedCells, autopas::TraversalOption::c08, autopas::DataLayoutOption::aos,
                  autopas::Newton3Option::enabled);

  autopas::AutoTuner<Particle, FPCell> autoTuner({0, 0, 0}, {10, 10, 10}, 1, 0, 100, configs,
                                                 autopas::SelectorStrategy::fastestAbs, 1000, 2);

  EXPECT_EQ(*(configs.begin()), autoTuner.getCurrentConfig());

  MockFunctor<Particle, FPCell> functor;
  EXPECT_CALL(functor, isRelevantForTuning()).WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(functor, allowsNewton3()).WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(functor, allowsNonNewton3()).WillRepeatedly(::testing::Return(false));

  // Intended false positive
  EXPECT_TRUE(autoTuner.willRebuild()) << "Expect rebuild for first iteration.";
  autoTuner.iteratePairwise(&functor);  // DS N3
  EXPECT_FALSE(autoTuner.willRebuild()) << "Expect no rebuild because more samples needed.";
  autoTuner.iteratePairwise(&functor);  // DS N3
  EXPECT_TRUE(autoTuner.willRebuild()) << "Expect rebuild because we change config.";
  autoTuner.iteratePairwise(&functor);  // LC N3
  EXPECT_FALSE(autoTuner.willRebuild()) << "Expect no rebuild because more samples needed.";
  autoTuner.iteratePairwise(&functor);  // LC N3
  EXPECT_TRUE(autoTuner.willRebuild()) << "Expect rebuild because reached end of tuning phase.";
  autoTuner.iteratePairwise(&functor);  // optimum
  EXPECT_FALSE(autoTuner.willRebuild()) << "Expect no rebuild because not tuning.";
}

TEST_F(AutoTunerTest, testWillRebuildDL) {
  // also check if rebuild is detected if next config is invalid

  std::set<autopas::Configuration> configs;
  configs.emplace(autopas::ContainerOption::directSum, autopas::TraversalOption::directSumTraversal,
                  autopas::DataLayoutOption::aos, autopas::Newton3Option::disabled);
  configs.emplace(autopas::ContainerOption::linkedCells, autopas::TraversalOption::c08, autopas::DataLayoutOption::aos,
                  autopas::Newton3Option::disabled);

  autopas::AutoTuner<Particle, FPCell> autoTuner({0, 0, 0}, {10, 10, 10}, 1, 0, 100, configs,
                                                 autopas::SelectorStrategy::fastestAbs, 1000, 2);

  EXPECT_EQ(*(configs.begin()), autoTuner.getCurrentConfig());

  MockFunctor<Particle, FPCell> functor;
  EXPECT_CALL(functor, isRelevantForTuning()).WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(functor, allowsNewton3()).WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(functor, allowsNonNewton3()).WillRepeatedly(::testing::Return(true));

  // Intended false positive
  EXPECT_TRUE(autoTuner.willRebuild()) << "Expect rebuild for first iteration.";
  autoTuner.iteratePairwise(&functor);  // DS NoN3
  EXPECT_FALSE(autoTuner.willRebuild()) << "Expect no rebuild because more samples needed.";
  autoTuner.iteratePairwise(&functor);  // DS NoN3
  EXPECT_TRUE(autoTuner.willRebuild()) << "Expect rebuild because we change config.";
  autoTuner.iteratePairwise(&functor);  // LC NoN3
  EXPECT_FALSE(autoTuner.willRebuild()) << "Expect no rebuild because more samples needed.";
  autoTuner.iteratePairwise(&functor);  // LC NoN3
  EXPECT_TRUE(autoTuner.willRebuild()) << "Expect rebuild because reached end of tuning phase.";
  autoTuner.iteratePairwise(&functor);  // optimum
  EXPECT_FALSE(autoTuner.willRebuild()) << "Expect no rebuild because not tuning.";
}

/**
 * Generates no configurations.
 */
TEST_F(AutoTunerTest, testNoConfig) {
  // wrap constructor call into lambda to avoid parser errors
  auto exp1 = []() {
    autopas::AutoTuner<Particle, FPCell> autoTuner({0, 0, 0}, {10, 10, 10}, 1, 0, 100, {},
                                                   autopas::SelectorStrategy::fastestAbs, 1000, 3);
  };

  EXPECT_THROW(exp1(), autopas::utils::ExceptionHandler::AutoPasException) << "Constructor with given configs";

  // wrap constructor call into lambda to avoid parser errors
  auto exp2 = []() {
    autopas::AutoTuner<Particle, FPCell> autoTuner({0, 0, 0}, {10, 10, 10}, 1, 0, 100, {}, {}, {}, {},
                                                   autopas::SelectorStrategy::fastestAbs, 1000, 3);
  };

  EXPECT_THROW(exp2(), autopas::utils::ExceptionHandler::AutoPasException) << "Constructor which generates configs";
}

/**
 * Generates exactly one valid configuration.
 */
TEST_F(AutoTunerTest, testOneConfig) {
  autopas::Configuration conf(autopas::ContainerOption::linkedCells, autopas::TraversalOption::c08,
                              autopas::DataLayoutOption::aos, autopas::Newton3Option::enabled);

  autopas::AutoTuner<Particle, FPCell> tuner({0, 0, 0}, {10, 10, 10}, 1, 0, 100, {conf},
                                             autopas::SelectorStrategy::fastestAbs, 1000, 3);

  EXPECT_EQ(conf, tuner.getCurrentConfig());

  MFunctor functor;
  EXPECT_CALL(functor, isRelevantForTuning()).WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(functor, allowsNewton3()).WillRepeatedly(::testing::Return(true));

  tuner.iteratePairwise(&functor);

  EXPECT_EQ(conf, tuner.getCurrentConfig());
}

/**
 * Generates exactly one valid and one invalid configuration.
 */
TEST_F(AutoTunerTest, testConfigSecondInvalid) {
  autopas::Configuration confN3(autopas::ContainerOption::linkedCells, autopas::TraversalOption::c08,
                                autopas::DataLayoutOption::aos, autopas::Newton3Option::enabled);
  autopas::Configuration confNoN3(autopas::ContainerOption::linkedCells, autopas::TraversalOption::c08,
                                  autopas::DataLayoutOption::aos, autopas::Newton3Option::disabled);

  autopas::AutoTuner<Particle, FPCell> tuner({0, 0, 0}, {10, 10, 10}, 1, 0, 100, {confN3, confNoN3},
                                             autopas::SelectorStrategy::fastestAbs, 1000, 3);

  EXPECT_EQ(confN3, tuner.getCurrentConfig());

  MFunctor functor;
  EXPECT_CALL(functor, isRelevantForTuning()).WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(functor, allowsNewton3()).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(functor, allowsNonNewton3()).WillRepeatedly(::testing::Return(true));

  tuner.iteratePairwise(&functor);
  EXPECT_EQ(confNoN3, tuner.getCurrentConfig());

  tuner.iteratePairwise(&functor);
  EXPECT_EQ(confNoN3, tuner.getCurrentConfig());

  tuner.iteratePairwise(&functor);
  EXPECT_EQ(confNoN3, tuner.getCurrentConfig());
}

/**
 * All generated configurations are thrown out at runtime.
 */
TEST_F(AutoTunerTest, testLastConfigThrownOut) {
  autopas::Configuration confN3(autopas::ContainerOption::linkedCells, autopas::TraversalOption::c08,
                                autopas::DataLayoutOption::aos, autopas::Newton3Option::enabled);
  autopas::Configuration confNoN3(autopas::ContainerOption::linkedCells, autopas::TraversalOption::c08,
                                  autopas::DataLayoutOption::soa, autopas::Newton3Option::enabled);

  autopas::AutoTuner<Particle, FPCell> tuner({0, 0, 0}, {10, 10, 10}, 1, 0, 100, {confN3, confNoN3},
                                             autopas::SelectorStrategy::fastestAbs, 1000, 3);

  EXPECT_EQ(confN3, tuner.getCurrentConfig());

  MFunctor functor;
  EXPECT_CALL(functor, isRelevantForTuning()).WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(functor, allowsNewton3()).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(functor, allowsNonNewton3()).WillRepeatedly(::testing::Return(true));

  EXPECT_THROW(tuner.iteratePairwise(&functor), autopas::utils::ExceptionHandler::AutoPasException);
}