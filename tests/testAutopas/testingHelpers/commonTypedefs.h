/**
 * @file commonTypedefs.h
 * @author F. Gratl
 * @date 6/21/18
 */

#pragma once

#include "autopas/molecularDynamics/MoleculeLJ.h"
#include "autopas/particles/Particle.h"
#include "mocks/MockFunctor.h"

// a place for typedefs that are commonly used in tests

/**
 * Short for AutoPas Particle
 */
typedef autopas::Particle Particle;
/**
 * Short for a FullParticle Cell with the AutoPas Particle
 */
typedef autopas::FullParticleCell<autopas::Particle> FPCell;

/**
 * Short for the AutoPas single center Lennard-Jones molecule
 */
typedef autopas::MoleculeLJ<> Molecule;
/**
 * Short for the Full Particle Cell with the single center Lennard-Jones molecule
 */
typedef autopas::FullParticleCell<Molecule> FMCell;

// M prefix for mocks
/**
 * Short for Mock Functor
 */
typedef MockFunctor<autopas::Particle, autopas::FullParticleCell<autopas::Particle>> MFunctor;
