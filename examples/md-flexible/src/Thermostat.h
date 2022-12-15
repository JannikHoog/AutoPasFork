/**
 * @file Thermostat.h
 * @author N. Fottner
 * @date 27/8/19
 */

#pragma once
#include <cstdlib>

#include "TypeDefinitions.h"
#include "autopas/AutoPasDecl.h"
#include "autopas/utils/ArrayMath.h"
#include "autopas/utils/WrapMPI.h"
#include "autopas/utils/WrapOpenMP.h"

/**
 * Thermostat to adjust the Temperature of the Simulation.
 * Todo Update for rotation
 */
namespace Thermostat {

namespace {
/**
 * Add a random velocity according to the Maxwell-Boltzmann distribution to the particle.
 *
 * @param p The particle to initialize.
 * @param averageVelocity Average velocity per dimension to be added.
 * @param randomEngine Random engine used for the generation of the velocity.
 * @param normalDistribution Distribution used for constructing the maxwell boltzmann distribution.
 */
void addMaxwellBoltzmannDistributedVelocity(ParticleType &p, const double averageVelocity,
                                            std::default_random_engine &randomEngine,
                                            std::normal_distribution<double> &normalDistribution) {
  // when adding independent normally distributed values to all velocity components
  // the velocity change is maxwell boltzmann distributed
  std::array<double, 3> randomVelocity{};
  for (double &v : randomVelocity) {
    v = averageVelocity * normalDistribution(randomEngine);
  }
  p.setV(autopas::utils::ArrayMath::add(p.getV(), randomVelocity));
}
}  // namespace

/**
 * Calculates temperature of system.
 * Assuming dimension-less units and Boltzmann constant = 1.
 * @tparam AutoPasTemplate Type of AutoPas Object (no pointer)
 * @tparam ParticlePropertiesLibraryTemplate Type of ParticlePropertiesLibrary Object (no pointer)
 * @param autopas
 * @param particlePropertiesLibrary
 * @return Temperature of system.
 */
template <class AutoPasTemplate, class ParticlePropertiesLibraryTemplate>
double calcTemperature(const AutoPasTemplate &autopas, ParticlePropertiesLibraryTemplate &particlePropertiesLibrary) {
  // kinetic energy times 2
  double kineticEnergyMul2 = 0;
#ifdef AUTOPAS_OPENMP
#pragma omp parallel reduction(+ : kineticEnergyMul2) default(none) shared(autopas, particlePropertiesLibrary)
#endif
  for (auto iter = autopas.begin(); iter.isValid(); ++iter) {
    auto vel = iter->getV();
    kineticEnergyMul2 +=
        particlePropertiesLibrary.getMass(iter->getTypeId()) * autopas::utils::ArrayMath::dot(vel, vel);
  }
  // AutoPas works always on 3 dimensions
  constexpr unsigned int dimensions{3};
  return kineticEnergyMul2 / (autopas.getNumberOfParticles() * dimensions);
}

/**
 * Calculates temperature of system, for each component separately.
 *
 * Kinetic Energy for each molecule is
 *      1/2 * mass * dot(vel, vel) + 1/2 \Sum_{0 \leq i < 3} MoI_i * angVel_i^2
 * where MoI is the diagonal Moment of Inertia. This formula comes from Rapport, The Art of MD, equation (8.2.34).
 *
 * The second term is only applied for Multi-Site MD.
 *
 * Assuming dimension-less units and Boltzmann constant = 1.
 *
 * @tparam AutoPasTemplate Type of AutoPas Object (no pointer)
 * @tparam ParticlePropertiesLibraryTemplate Type of ParticlePropertiesLibrary Object (no pointer)
 * @param autopas
 * @param particlePropertiesLibrary
 * @return map of: particle typeID -> temperature for this type
 */
template <class AutoPasTemplate, class ParticlePropertiesLibraryTemplate>
auto calcTemperatureComponent(const AutoPasTemplate &autopas,
                              ParticlePropertiesLibraryTemplate &particlePropertiesLibrary) {
  using autopas::utils::ArrayMath::dot;
  using autopas::utils::ArrayMath::mul;

  // map of: particle typeID -> kinetic energy times 2 for this type
  std::map<size_t, double> kineticEnergyMul2Map;
  // map of: particle typeID -> number of particles of this type
  std::map<size_t, size_t> numParticleMap;

  for (const auto &typeID : particlePropertiesLibrary.getTypes()) {
    kineticEnergyMul2Map[typeID] = 0.;
    numParticleMap[typeID] = 0ul;
  }

#ifdef AUTOPAS_OPENMP
//#pragma omp parallel
#endif
  {
    // create aggregators for each thread
    std::map<size_t, double> kineticEnergyMul2MapThread;
    std::map<size_t, size_t> numParticleMapThread;
    for (const auto &typeID : particlePropertiesLibrary.getTypes()) {
      kineticEnergyMul2MapThread[typeID] = 0.;
      numParticleMapThread[typeID] = 0ul;
    }
    // parallel iterators
    for (auto iter = autopas.begin(); iter.isValid(); ++iter) {
      const auto &vel = iter->getV();
#if defined(MD_FLEXIBLE_USE_MULTI_SITE)
      const auto &angVel = iter->getAngularVel();
#endif
      kineticEnergyMul2MapThread.at(iter->getTypeId()) +=
          particlePropertiesLibrary.getMass(iter->getTypeId()) * dot(vel, vel);
#if defined(MD_FLEXIBLE_USE_MULTI_SITE)
      kineticEnergyMul2MapThread.at(iter->getTypeId()) += dot(particlePropertiesLibrary.getgetMomentOfInertia(),
                                                              mul(angVel, angVel));
#endif
      numParticleMapThread.at(iter->getTypeId())++;
    }
    // manual reduction
#ifdef AUTOPAS_OPENMP
#pragma omp critical
#endif
    {
      for (const auto &typeID : particlePropertiesLibrary.getTypes()) {
        kineticEnergyMul2Map[typeID] += kineticEnergyMul2MapThread[typeID];
        numParticleMap[typeID] += numParticleMapThread[typeID];
      }
    }
  }
  // AutoPas works always on 3 dimensions
  constexpr unsigned int dimensions{3};

  for (const auto &typeID : particlePropertiesLibrary.getTypes()) {
    // workaround for MPICH: send and receive buffer must not be the same.
    autopas::AutoPas_MPI_Allreduce(AUTOPAS_MPI_IN_PLACE, &kineticEnergyMul2Map[typeID], 1, AUTOPAS_MPI_DOUBLE,
                                   AUTOPAS_MPI_SUM, AUTOPAS_MPI_COMM_WORLD);

    autopas::AutoPas_MPI_Allreduce(AUTOPAS_MPI_IN_PLACE, &numParticleMap[typeID], 1, AUTOPAS_MPI_UNSIGNED_LONG,
                                   AUTOPAS_MPI_SUM, AUTOPAS_MPI_COMM_WORLD);
  }

  auto kineticEnergyAndParticleMaps = std::make_tuple(kineticEnergyMul2Map.begin(), numParticleMap.begin());

  for (auto [kineticEnergyMapIter, numParticleMapIter] = kineticEnergyAndParticleMaps;
       kineticEnergyMapIter != kineticEnergyMul2Map.end(); ++kineticEnergyMapIter, ++numParticleMapIter) {
    kineticEnergyMapIter->second /= static_cast<double>(numParticleMapIter->second) * dimensions;
  }
  return kineticEnergyMul2Map;
}

/**
 * Adds brownian motion to the given system.
 *
 * If useCurrentTemp is set to true the factor of the brownian motion is calculated per particle based on its mass and
 * the system's temperature. Otherwise a constant factor of 0.1 is used.
 * Set this to false if the system is initialized without velocities.
 *
 * @tparam AutoPasTemplate Type of AutoPas Object (no pointer)
 * @tparam ParticlePropertiesLibraryTemplate Type of ParticlePropertiesLibrary Object (no pointer)
 * @param autopas
 * @param particlePropertiesLibrary
 * @param targetTemperature temperature of the system after applying the function on a system with temperature = 0.
 */
template <class AutoPasTemplate, class ParticlePropertiesLibraryTemplate>
void addBrownianMotion(AutoPasTemplate &autopas, ParticlePropertiesLibraryTemplate &particlePropertiesLibrary,
                       const double targetTemperature) {
  // factors for the brownian motion per particle type.
  std::map<size_t, double> factors;
  // brownian motion with disturbance depending on current temperature and mass
  for (auto typeID : particlePropertiesLibrary.getTypes()) {
    factors.emplace(typeID, std::sqrt(targetTemperature / particlePropertiesLibrary.getMass(typeID)));
  }
#ifdef AUTOPAS_OPENMP
#pragma omp parallel default(none) shared(autopas, factors)
#endif
  {
    // we use a constant seed for repeatability.
    // we need one random engine and distribution per thread
    std::default_random_engine randomEngine(42 + autopas::autopas_get_thread_num());
    std::normal_distribution<double> normalDistribution{0, 1};
    for (auto iter = autopas.begin(); iter.isValid(); ++iter) {
      addMaxwellBoltzmannDistributedVelocity(*iter, factors[iter->getTypeId()], randomEngine, normalDistribution);
    }
  }
}

/**
 * Scales velocity of particles towards a given temperature. For Multi-site simulations, angular velocity is also scaled.
 * @tparam AutoPasTemplate Type of AutoPas Object (no pointer)
 * @tparam ParticlePropertiesLibraryTemplate Type of ParticlePropertiesLibrary Object (no pointer)
 * @param autopas
 * @param particlePropertiesLibrary
 * @param targetTemperature
 * @param deltaTemperature Maximum temperature change.
 */
template <class AutoPasTemplate, class ParticlePropertiesLibraryTemplate>
void apply(AutoPasTemplate &autopas, ParticlePropertiesLibraryTemplate &particlePropertiesLibrary,
           const double targetTemperature, const double deltaTemperature) {
  const auto currentTemperatureMap = calcTemperatureComponent(autopas, particlePropertiesLibrary);

  // make sure we work with a positive delta
  const double absoluteDeltaTemperature = std::abs(deltaTemperature);

  std::remove_const_t<decltype(currentTemperatureMap)> scalingMap;

  for (const auto &[particleTypeID, currentTemperature] : currentTemperatureMap) {
    // If the current temperature is within absoluteDeltaTemperature of target temperature,
    //      set immediate target temperature to target temperature.
    //
    // Else, set immediate target temperature to absoluteDeltaTemperature towards the target temperature from the
    // current temperature.

    const auto immediateTargetTemperature = currentTemperature < targetTemperature ? std::min(currentTemperature + absoluteDeltaTemperature, targetTemperature) :
                                                                                   std::max(currentTemperature - absoluteDeltaTemperature, targetTemperature);
    // Determine a scaling factor for each particle type.
    scalingMap[particleTypeID] = std::sqrt(immediateTargetTemperature / currentTemperature);
  }

  // Scale velocities (and angular velocities) with the scaling map
#ifdef AUTOPAS_OPENMP
#pragma omp parallel default(none) shared(autopas, scalingMap)
#endif
  for (auto iter = autopas.begin(); iter.isValid(); ++iter) {
    iter->setV(autopas::utils::ArrayMath::mulScalar(iter->getV(), scalingMap[iter->getTypeId()]));
#if defined(MD_FLEXIBLE_USE_MULTI_SITE)
    iter->setAngularVel(autopas::utils::ArrayMath::mulScalar(iter->getAngularVel(), scalingMap[iter->getTypeId()]));
#endif
  }
}
}  // namespace Thermostat
