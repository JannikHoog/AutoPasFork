#include "autopas/selectors/tuningStrategy/TuningStrategyLoggerProxy.h"
#include "autopas/selectors/tuningStrategy/ruleBasedTuning/RuleBasedTuning.h"

int main(int argc, char** argv) {
  autopas::Logger::create();

  if(argc <= 1) {
    std::cerr << "Please provide the data files as arguments" << std::endl;
    exit(-1);
  }

  if(not getenv("DISABLE_DEBUG_LOG")) {
    autopas::Logger::get()->set_level(spdlog::level::info);
  }
  auto containers = autopas::ContainerOption::getAllOptions();
  auto traversals = autopas::TraversalOption::getAllOptions();
  auto loadEstimators = autopas::LoadEstimatorOption::getAllOptions();
  auto dataLayouts = autopas::DataLayoutOption::getAllOptions();
  auto newton3Options = autopas::Newton3Option::getAllOptions();

  // Map configuration to indices of files where they were best
  std::map<autopas::Configuration, std::vector<int>> bestConfigs;

  int numErrors = 0;
  constexpr double bigErrorThreshold = 1.15;
  int numBigErrors = 0;
  auto errorHandler = [&](const autopas::rule_syntax::ConfigurationOrder& order,
                          const autopas::Configuration& actualBetterConfig, unsigned long betterRuntime,
                          const autopas::Configuration& shouldBeBetterConfig, unsigned long shouldBeBetterRuntime,
                          const autopas::LiveInfo& liveInfo) {
    numErrors++;

    auto factorDifference = static_cast<double>(shouldBeBetterRuntime) / static_cast<double>(betterRuntime);
    auto factorDifferenceRounded = std::round(factorDifference * 100) / 100;

    if(factorDifference >= bigErrorThreshold) {
      numBigErrors++;
    }

    AutoPasLog(error, "\n"
               "\tError in ConfigurationOrder {}:\n"
               "\t\t{}ns for config\t{}\n"
               "\t\t{}ns for config\t{}\n"
               "\t\tx{} difference",
               order.toString(), betterRuntime, actualBetterConfig.toShortString(), shouldBeBetterRuntime,
               shouldBeBetterConfig.toShortString(), factorDifferenceRounded);
  };

  unsigned long tuningTimeSum = 0;
  unsigned long wouldHaveSkippedTuningTimeSum = 0;
  for(int i = 1; i < argc; i++) {
    auto filename = argv[i];
    AutoPasLog(info, "Checking file {}: {}", i, filename);
    auto strategy = std::make_shared<autopas::RuleBasedTuning>(
        containers, std::set<double>({1., 2.}), traversals, loadEstimators, dataLayouts, newton3Options,
        true, "tuningRules.rule", errorHandler);
    autopas::TuningStrategyLogReplayer logReplayer{argv[i], strategy};
    auto optBestConfig = logReplayer.replay();
    AutoPasLog(info, "");

    if(optBestConfig.has_value()) {
      bestConfigs[optBestConfig.value()].push_back(i);
    }
    tuningTimeSum += strategy->getLifetimeTuningTime();
    wouldHaveSkippedTuningTimeSum += strategy->getLifetimeWouldHaveSkippedTuningTime();
  }

  std::stringstream str;
  for(const auto& [config, filenames] : bestConfigs) {
    str << "\t" << "Best in " << filenames.size() << " scenarios:\t" << config.toShortString() << " (file numbers: ";
    for(const auto& file : filenames) {
      str << file << ", ";
    }
    str << ")" << std::endl;
  }

  AutoPasLog(info, "Finished replaying {} scenarios!", argc - 1);
  AutoPasLog(info, "\nSummary of best configurations:\n{}", str.str());
  AutoPasLog(info, "In sum, found {} errors! Of these, {} errors where greater than {}", numErrors, numBigErrors,
             bigErrorThreshold);

  auto savedTuningTimeRatio = static_cast<double>(wouldHaveSkippedTuningTimeSum) / static_cast<double>(tuningTimeSum) * 100;
  auto savedTuningTimeRatioRounded = std::round(savedTuningTimeRatio * 100) / 100;
  AutoPasLog(info, "Overall, {}% of the tuning time would have been saved.", savedTuningTimeRatioRounded);

  autopas::Logger::unregister();
}