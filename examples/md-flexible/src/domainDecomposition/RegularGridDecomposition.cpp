/**
 * @file RegularGridDecomposition.cpp
 * @author J. Körner
 * @date 19.04.2021
 */
#include "RegularGridDecomposition.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <numeric>

#include "DomainTools.h"
#include "autopas/AutoPas.h"
#include "autopas/utils/ArrayUtils.h"
#include "src/ParticleSerializationTools.h"

RegularGridDecomposition::RegularGridDecomposition(const std::array<double, 3> &globalBoxMin,
                                                   const std::array<double, 3> &globalBoxMax, const double &cutoffWidth,
                                                   const double &skinWidth)
    : _cutoffWidth(cutoffWidth), _skinWidth(skinWidth) {
#if defined(AUTOPAS_INCLUDE_MPI)
  _mpiIsEnabled = true;
#else
  _mpiIsEnabled = false;
#endif

  autopas::AutoPas_MPI_Comm_size(AUTOPAS_MPI_COMM_WORLD, &_subdomainCount);

  if (_subdomainCount == 1) {
    _mpiIsEnabled = false;
  }

  if (_mpiIsEnabled) {
    std::cout << "MPI will be used." << std::endl;
  } else {
    std::cout << "MPI will not be used." << std::endl;
  }

  DomainTools::generateDecomposition(_subdomainCount, _decomposition);

  initializeMPICommunicator();

  initializeLocalDomain();

  initializeGlobalBox(globalBoxMin, globalBoxMax);

  initializeLocalBox();

  initializeNeighbourIds();
}

RegularGridDecomposition::~RegularGridDecomposition() {}

void RegularGridDecomposition::update() { updateLocalBox(); }

void RegularGridDecomposition::initializeMPICommunicator() {
  std::vector<int> periods(3, 1);
  autopas::AutoPas_MPI_Cart_create(AUTOPAS_MPI_COMM_WORLD, 3, _decomposition.data(), periods.data(), true,
                                   &_communicator);
  autopas::AutoPas_MPI_Comm_rank(_communicator, &_domainIndex);
}

void RegularGridDecomposition::initializeLocalDomain() {
  _domainId = {0, 0, 0};
  autopas::AutoPas_MPI_Comm_rank(_communicator, &_domainIndex);

  std::vector<int> periods(3, 1);
  autopas::AutoPas_MPI_Cart_get(_communicator, 3, _decomposition.data(), periods.data(), _domainId.data());
}

void RegularGridDecomposition::initializeLocalBox() { updateLocalBox(); }

void RegularGridDecomposition::initializeNeighbourIds() {
  for (int i = 0; i < 3; ++i) {
    auto neighbourIndex = i * 2;
    auto preceedingNeighbourId = _domainId;
    preceedingNeighbourId[i] = (--preceedingNeighbourId[i] + _decomposition[i]) % _decomposition[i];
    _neighbourDomainIndices[neighbourIndex] = convertIdToIndex(preceedingNeighbourId);

    ++neighbourIndex;
    auto succeedingNeighbourId = _domainId;
    succeedingNeighbourId[i] = (++succeedingNeighbourId[i] + _decomposition[i]) % _decomposition[i];
    _neighbourDomainIndices[neighbourIndex] = convertIdToIndex(succeedingNeighbourId);
  }
}

void RegularGridDecomposition::updateLocalBox() {
  for (int i = 0; i < 3; ++i) {
    double localBoxWidth = (_globalBoxMax[i] - _globalBoxMin[i]) / static_cast<double>(_decomposition[i]);

    _localBoxMin[i] = _domainId[i] * localBoxWidth + _globalBoxMin[i];
    _localBoxMax[i] = (_domainId[i] + 1) * localBoxWidth + _globalBoxMin[i];

    if (_domainId[i] == 0) {
      _localBoxMin[i] = _globalBoxMin[i];
    } else if (_domainId[i] == _decomposition[i] - 1) {
      _localBoxMax[i] = _globalBoxMax[i];
    }
  }
}

void RegularGridDecomposition::initializeGlobalBox(const std::array<double, 3> &globalBoxMin,
                                                   const std::array<double, 3> &globalBoxMax) {
  for (int i = 0; i < 3; ++i) {
    _globalBoxMin[i] = globalBoxMin[i];
    _globalBoxMax[i] = globalBoxMax[i];
  }
}

bool RegularGridDecomposition::isInsideLocalDomain(const std::array<double, 3> &coordinates) {
  return DomainTools::isInsideDomain(coordinates, _localBoxMin, _localBoxMax);
}

void RegularGridDecomposition::exchangeHaloParticles(SharedAutoPasContainer &autoPasContainer) {
  for (int i = 0; i < _dimensionCount; ++i) {
    std::vector<ParticleType> particlesForLeftNeighbour, particlesForRightNeighbour, haloParticles;

    collectHaloParticlesForLeftNeighbour(autoPasContainer, i, particlesForLeftNeighbour);
    collectHaloParticlesForRightNeighbour(autoPasContainer, i, particlesForRightNeighbour);

    // See documentation for _neighbourDomainIndices to explain the indexing
    int leftNeighbour = _neighbourDomainIndices[(i * 2) % _neighbourCount];
    int rightNeighbour = _neighbourDomainIndices[(i * 2 + 1) % _neighbourCount];
    sendAndReceiveParticlesLeftAndRight(particlesForLeftNeighbour, particlesForRightNeighbour, leftNeighbour,
                                        rightNeighbour, haloParticles);

    for (auto &particle : haloParticles) {
      autoPasContainer->addOrUpdateHaloParticle(particle);
    }

    particlesForLeftNeighbour.clear();
    particlesForRightNeighbour.clear();

    // index of next dimension
    int j = (i + 1) % _dimensionCount;
    double leftHaloMin = _localBoxMin[j] - _skinWidth;
    double leftHaloMax = _localBoxMin[j] + _cutoffWidth + _skinWidth;
    double rightHaloMin = _localBoxMax[j] - _cutoffWidth - _skinWidth;
    double rightHaloMax = _localBoxMax[j] + _skinWidth;

    for (const auto &particle : haloParticles) {
      std::array<double, _dimensionCount> position = particle.getR();

      // Collect halo particles for left neighbour
      if (position[j] >= leftHaloMin && position[j] < leftHaloMax) {
        particlesForLeftNeighbour.push_back(particle);

        // Apply boundary condition
        if (_localBoxMin[j] == _globalBoxMin[j]) {
          position[j] = position[j] + (_globalBoxMax[j] - _globalBoxMin[j]);
          particlesForLeftNeighbour.back().setR(position);
        }
<<<<<<< HEAD
      } else if (position[j] >= rightHaloMin && position[j] < rightHaloMax) {
        particlesForRightNeighbour.push_back(particle);

        // Apply boundary condition
        if (_localBoxMax[j] == _globalBoxMax[j]) {
          position[j] = position[j] - (_globalBoxMax[j] - _globalBoxMin[j]);
          particlesForRightNeighbour.back().setR(position);
        }
      }
    }

    haloParticles.clear();

    // See documentation for _neighbourDomainIndices to explain the indexing
    leftNeighbour = _neighbourDomainIndices[(j * 2) % _neighbourCount];
    rightNeighbour = _neighbourDomainIndices[(j * 2 + 1) % _neighbourCount];
    sendAndReceiveParticlesLeftAndRight(particlesForLeftNeighbour, particlesForRightNeighbour, leftNeighbour,
                                        rightNeighbour, haloParticles);

    for (auto &particle : haloParticles) {
      autoPasContainer->addOrUpdateHaloParticle(particle);
    }
  }
}

void RegularGridDecomposition::exchangeMigratingParticles(SharedAutoPasContainer &autoPasContainer) {
  auto [emigrants, updated] = autoPasContainer->updateContainer(false);

  if (updated) {
    for (int i = 0; i < _dimensionCount; ++i) {
      std::vector<ParticleType> immigrants, migrants, remainingEmigrants;

      std::vector<ParticleType> particlesForLeftNeighbour;
      std::vector<ParticleType> particlesForRightNeighbour;

      categorizeParticlesIntoLeftAndRightNeighbour(emigrants, i, particlesForLeftNeighbour, particlesForRightNeighbour,
                                                   remainingEmigrants);

      // Assign remaining emigrants to emigrants for next loop iteration.
      emigrants = remainingEmigrants;

      // See documentation for _neighbourDomainIndices to explain the indexing
      int leftNeighbour = _neighbourDomainIndices[(i * 2) % _neighbourCount];
      int rightNeighbour = _neighbourDomainIndices[(i * 2 + 1) % _neighbourCount];

      sendAndReceiveParticlesLeftAndRight(particlesForLeftNeighbour, particlesForRightNeighbour, leftNeighbour,
                                          rightNeighbour, immigrants);

      particlesForLeftNeighbour.clear();
      particlesForRightNeighbour.clear();

      for (const auto &particle : immigrants) {
        if (isInsideLocalDomain(particle.getR())) {
          autoPasContainer->addParticle(particle);
        } else {
          migrants.push_back(particle);
        }
      }

      immigrants.clear();

      // index of next dimension
      int j = (i + 1) % _dimensionCount;

      // See documentation for _neighbourDomainIndices to explain the indexing
      leftNeighbour = _neighbourDomainIndices[(j * 2) % _neighbourCount];
      rightNeighbour = _neighbourDomainIndices[(j * 2 + 1) % _neighbourCount];

      // This call should not change the reaminingEmigrants vector because there should not be any particles
      // which cannot be assigned to either the left or the right neighbour.
      categorizeParticlesIntoLeftAndRightNeighbour(migrants, j, particlesForLeftNeighbour, particlesForRightNeighbour,
                                                   remainingEmigrants);

      sendAndReceiveParticlesLeftAndRight(particlesForLeftNeighbour, particlesForRightNeighbour, leftNeighbour,
                                          rightNeighbour, immigrants);

      for (auto &particle : immigrants) {
        autoPasContainer->addParticle(particle);
      }

      waitForSendRequests();
    }
  }
}

void RegularGridDecomposition::sendParticles(const std::vector<ParticleType> &particles, const int &receiver) {
  std::vector<char> buffer;

  for (auto &particle : particles) {
    ParticleSerializationTools::serializeParticle(particle, buffer);
  }

  size_t sizeOfParticleAttributes = sizeof(ParticleAttributes);

  sendDataToNeighbour(buffer, receiver);
}

void RegularGridDecomposition::receiveParticles(std::vector<ParticleType> &receivedParticles, const int &source) {
  std::vector<char> receiveBuffer;

  receiveDataFromNeighbour(source, receiveBuffer);

  if (!receiveBuffer.empty()) {
    ParticleSerializationTools::deserializeParticles(receiveBuffer, receivedParticles);
  }
}

void RegularGridDecomposition::sendDataToNeighbour(std::vector<char> sendBuffer, const int &neighbour) {
  _sendBuffers.push_back(sendBuffer);

  autopas::AutoPas_MPI_Request sendRequest;
  _sendRequests.push_back(sendRequest);

  autopas::AutoPas_MPI_Isend(_sendBuffers.back().data(), _sendBuffers.back().size(), AUTOPAS_MPI_CHAR, neighbour, 0,
                             _communicator, &_sendRequests.back());
}

void RegularGridDecomposition::receiveDataFromNeighbour(const int &neighbour, std::vector<char> &receiveBuffer) {
  autopas::AutoPas_MPI_Status status;
  autopas::AutoPas_MPI_Probe(neighbour, 0, _communicator, &status);

  int receiveBufferSize;
  autopas::AutoPas_MPI_Get_count(&status, AUTOPAS_MPI_CHAR, &receiveBufferSize);
  receiveBuffer.resize(receiveBufferSize);

  autopas::AutoPas_MPI_Recv(receiveBuffer.data(), receiveBufferSize, AUTOPAS_MPI_CHAR, neighbour, 0, _communicator,
                            AUTOPAS_MPI_STATUS_IGNORE);
}

void RegularGridDecomposition::sendAndReceiveParticlesLeftAndRight(const std::vector<ParticleType> &particlesToLeft,
                                                                   const std::vector<ParticleType> &particlesToRight,
                                                                   const int &leftNeighbour, const int &rightNeighbour,
                                                                   std::vector<ParticleType> &receivedParticles) {
  if (_mpiIsEnabled && leftNeighbour != _domainIndex) {
    sendParticles(particlesToLeft, leftNeighbour);
    sendParticles(particlesToRight, rightNeighbour);

    receiveParticles(receivedParticles, leftNeighbour);
    receiveParticles(receivedParticles, rightNeighbour);

    waitForSendRequests();
  } else {
    receivedParticles.insert(receivedParticles.end(), particlesToLeft.begin(), particlesToLeft.end());
    receivedParticles.insert(receivedParticles.end(), particlesToRight.begin(), particlesToRight.end());
  }
}

void RegularGridDecomposition::waitForSendRequests() {
  std::vector<autopas::AutoPas_MPI_Status> sendStates;
  sendStates.resize(_sendRequests.size());
  autopas::AutoPas_MPI_Waitall(_sendRequests.size(), _sendRequests.data(), sendStates.data());
  _sendRequests.clear();
  _sendBuffers.clear();
}

void RegularGridDecomposition::collectHaloParticlesForLeftNeighbour(SharedAutoPasContainer &autoPasContainer,
                                                                    const size_t &direction,
                                                                    std::vector<ParticleType> &haloParticles) {
  std::array<double, _dimensionCount> boxMin, boxMax;

  // Calculate halo box for left neighbour
  boxMin = {_localBoxMin[0] - _skinWidth, _localBoxMin[1] - _skinWidth, _localBoxMin[2] - _skinWidth};
  for (int i = 0; i < _dimensionCount; ++i) {
    boxMax[i] = _localBoxMin[i] + _skinWidth + (_localBoxMax[i] - _localBoxMin[i]);
  }
  boxMax[direction] = _localBoxMin[direction] + _cutoffWidth + _skinWidth;

  // Collect the halo particles for the left neighbour
  for (auto particle = autoPasContainer->getRegionIterator(boxMin, boxMax, autopas::IteratorBehavior::owned);
       particle.isValid(); ++particle) {
    std::array<double, _dimensionCount> position = particle->getR();
    haloParticles.push_back(*particle);

    // Apply boundary condition
    if (_localBoxMin[direction] == _globalBoxMin[direction]) {
      position[direction] = position[direction] + (_globalBoxMax[direction] - _globalBoxMin[direction]);
      haloParticles.back().setR(position);
    }
  }
}

void RegularGridDecomposition::collectHaloParticlesForRightNeighbour(SharedAutoPasContainer &autoPasContainer,
                                                                     const size_t &direction,
                                                                     std::vector<ParticleType> &haloParticles) {
  std::array<double, _dimensionCount> boxMin, boxMax;

  // Calculate left halo box of right neighbour
  for (int i = 0; i < _dimensionCount; ++i) {
    boxMin[i] = _localBoxMax[i] - _skinWidth - (_localBoxMax[i] - _localBoxMin[i]);
  }
  boxMin[direction] = _localBoxMax[direction] - _cutoffWidth - _skinWidth;
  boxMax = {_localBoxMax[0] + _skinWidth, _localBoxMax[1] + _skinWidth, _localBoxMax[2] + _skinWidth};

  // Collect the halo particles for the right neighbour
  for (auto particle = autoPasContainer->getRegionIterator(boxMin, boxMax, autopas::IteratorBehavior::owned);
       particle.isValid(); ++particle) {
    std::array<double, _dimensionCount> position = particle->getR();
    haloParticles.push_back(*particle);

    // Apply boundary condition
    if (_localBoxMax[direction] == _globalBoxMax[direction]) {
      position[direction] = position[direction] - (_globalBoxMax[direction] - _globalBoxMin[direction]);
      haloParticles.back().setR(position);
    }
  }
}

void RegularGridDecomposition::categorizeParticlesIntoLeftAndRightNeighbour(
    const std::vector<ParticleType> &particles, const size_t &direction,
    std::vector<ParticleType> &leftNeighbourParticles, std::vector<ParticleType> &rightNeighbourParticles,
    std::vector<ParticleType> &uncategorizedParticles) {
  for (const auto &particle : particles) {
    std::array<double, _dimensionCount> position = particle.getR();
    if (position[direction] < _localBoxMin[direction]) {
      leftNeighbourParticles.push_back(particle);

      // Apply boundary condition
      if (_localBoxMin[direction] == _globalBoxMin[direction]) {
        position[direction] = std::min(std::nextafter(_globalBoxMax[direction], _globalBoxMin[direction]),
                                       position[direction] + _globalBoxMax[direction] - _globalBoxMin[direction]);
        leftNeighbourParticles.back().setR(position);
      }
    } else if (position[direction] >= _localBoxMax[direction]) {
      rightNeighbourParticles.push_back(particle);

      // Apply boundary condition
      if (_localBoxMax[direction] == _globalBoxMax[direction]) {
        position[direction] = std::max(_globalBoxMin[direction],
                                       position[direction] - (_globalBoxMax[direction] - _globalBoxMin[direction]));
        rightNeighbourParticles.back().setR(position);
      }
    } else {
      uncategorizedParticles.push_back(particle);
    }
  }
}

int RegularGridDecomposition::convertIdToIndex(const std::array<int, 3> &domainId) {
  int neighbourDomainIndex = 0;

  for (int i = 0; i < 3; ++i) {
    int accumulatedTail = 1;

    if (i < _decomposition.size() - 1) {
      accumulatedTail =
          std::accumulate(_decomposition.begin() + i + 1, _decomposition.end(), 1, std::multiplies<int>());
    }

    neighbourDomainIndex += accumulatedTail * domainId[i];
  }

  return neighbourDomainIndex;
}
