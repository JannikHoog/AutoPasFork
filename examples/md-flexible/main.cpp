/**
 * @file main.cpp
 * @date 23.02.2018
 * @author F. Gratl
 */

#include "Simulation.h"

#include <autopas/utils/MemoryProfiler.h>
#include <yaml-cpp/yaml.h>
#include <iostream>
#include "PrintableMolecule.h"
#include "YamlParser.h"
#include "autopas/AutoPas.h"
#include "autopas/molecularDynamics/LJFunctorAVX.h"

int main(int argc, char **argv) {
  // start simulation timer
  Simulation<PrintableMolecule, autopas::FullParticleCell<PrintableMolecule>> simulation;
  // Parsing
  auto parser = std::make_shared<YamlParser>();
  if (not parser->parseInput(argc, argv)) {
    exit(-1);
  }
  auto vtkFilename(parser->getVTKFileName());
  parser->printConfig();
  std::cout << std::endl;

  // Initialization
  simulation.initialize(parser);
  std::cout << "Using " << autopas::autopas_get_max_threads() << " Threads" << std::endl;

  // Simulation
  std::cout << "Starting simulation... " << std::endl;
  simulation.simulate();
  std::cout << "Simulation done!" << std::endl;

  simulation.printStatistics();

  return EXIT_SUCCESS;
}
