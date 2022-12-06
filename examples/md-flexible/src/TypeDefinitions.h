/**
 * @file Simulation.cpp
 * @author F. Gratl
 * @date 01.03.2021
 */

#pragma once

#include "autopas/molecularDynamics/MoleculeLJ.h"
#include "autopas/molecularDynamics/MultisiteMoleculeLJ.h"
#include "autopas/molecularDynamics/ParticlePropertiesLibrary.h"

/**
 * Precision used for particle representations. If you want to test other precisions change it here.
 */
using FloatPrecision = double;

/**
 * Type of the Particles used in md-flexible.
 * Use the molecule type provided by AutoPas.
 */
#if defined(MD_FLEXIBLE_USE_MULTI_SITE)
  using ParticleType = autopas::MultisiteMoleculeLJ;
#else
  using ParticleType = autopas::MoleculeLJ;
#endif

/**
 * Type of the Particle Properties Library.
 * Set to the same precision as ParticleType.
 */
using ParticlePropertiesLibraryType = ParticlePropertiesLibrary<FloatPrecision, size_t>;
