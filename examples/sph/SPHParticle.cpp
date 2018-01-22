//
// Created by seckler on 19.01.18.
//

#include "SPHParticle.h"
#include "autopas.h"

using namespace autopas;

void SPHParticle::addAcc(const std::array<double, 3>& acc) {
    _acc = arrayMath::add(_acc, acc);
}

void SPHParticle::subAcc(const std::array<double, 3>& acc) {
    _acc = arrayMath::sub(_acc, acc);
}