//
// Created by seckler on 22.01.18.
//

#include "SPHCalcHydroForceFunctor.h"
#include "SPHKernels.h"

using namespace autopas::sph;

void SPHCalcHydroForceFunctor::AoSFunctor(SPHParticle &i, SPHParticle &j) {
  const std::array<double, 3> dr = arrayMath::sub(i.getR(), j.getR());
  // const PS::F64vec dr = ep_i[i].pos - ep_j[j].pos;

  const std::array<double, 3> dv = arrayMath::sub(i.getV(), j.getV());
  // const PS::F64vec dv = ep_i[i].vel - ep_j[j].vel;

  double dvdr = arrayMath::dot(dv, dr);
  const double w_ij = (dvdr < 0) ? dvdr / sqrt(arrayMath::dot(dr, dr)) : 0;
  // const PS::F64 w_ij = (dv * dr < 0) ? dv * dr / sqrt(dr * dr) : 0;

  const double v_sig = i.getSoundSpeed() + j.getSoundSpeed() - 3.0 * w_ij;
  // const PS::F64 v_sig = ep_i[i].snds + ep_j[j].snds - 3.0 * w_ij;

  i.checkAndSetVSigMax(v_sig);
  j.checkAndSetVSigMax(v_sig);  // Newton 3
  // v_sig_max = std::max(v_sig_max, v_sig);

  const double AV =
      -0.5 * v_sig * w_ij / (0.5 * (i.getDensity() + j.getDensity()));
  // const PS::F64 AV = - 0.5 * v_sig * w_ij / (0.5 * (ep_i[i].dens +
  // ep_j[j].dens));

  const std::array<double, 3> gradW_ij = arrayMath::mulScalar(
      arrayMath::add(SPHKernels::gradW(dr, i.getSmoothingLength()),
                     SPHKernels::gradW(dr, j.getSmoothingLength())),
      0.5);
  // const PS::F64vec gradW_ij = 0.5 * (gradW(dr, ep_i[i].smth) + gradW(dr,
  // ep_j[j].smth));

  double scale = (i.getPressure() / (i.getDensity() * i.getDensity()) +
                  j.getPressure() / (j.getDensity() * j.getDensity()) + AV);
  i.subAcceleration(arrayMath::mulScalar(gradW_ij, scale * j.getMass()));
  // hydro[i].acc     -= ep_j[j].mass * (ep_i[i].pres / (ep_i[i].dens *
  // ep_i[i].dens) + ep_j[j].pres / (ep_j[j].dens * ep_j[j].dens) + AV) *
  // gradW_ij;

  j.addAcceleration(arrayMath::mulScalar(gradW_ij, scale * i.getMass()));
  // Newton3, gradW_ij = -gradW_ji

  double scale2i =
      j.getMass() *
      (i.getPressure() / (i.getDensity() * i.getDensity()) + 0.5 * AV);
  i.addEngDot(arrayMath::dot(gradW_ij, dv) * scale2i);
  // hydro[i].eng_dot += ep_j[j].mass * (ep_i[i].pres / (ep_i[i].dens *
  // ep_i[i].dens) + 0.5 * AV) * dv * gradW_ij;

  double scale2j =
      i.getMass() *
          (j.getPressure() / (j.getDensity() * j.getDensity()) + 0.5 * AV);
  j.addEngDot(arrayMath::dot(gradW_ij, dv) * scale2j);
  // Newton 3
}

unsigned long SPHCalcHydroForceFunctor::getNumFlopsPerKernelCall() {
  /// @todo: correct
  return 1ul;
}