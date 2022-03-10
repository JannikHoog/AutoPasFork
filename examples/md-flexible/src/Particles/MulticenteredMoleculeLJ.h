/**
 * @file MulticenteredParticleBase.h
 * @date 14/02/2022
 * @author S. Newcome
 */

#pragma once

#include "autopas/particles/ParticleBase.h"
#include "autopas/particles/OwnershipState.h"
#include "autopas/molecularDynamics/MoleculeLJ.h"

/**
 * Standard multi-centre LJ molecules/
 *
 * The molecule is treated as a single particle for the purposes of cutoffs and containers, with a quaternion for
 * angular direction, a 3D vector-array for angular velocity, and a vectors of site positions relative to the CoM and
 * angular direction.
 *
 */
class MulticenteredMoleculeLJ : public MDLibrary::MoleculeInterface {
  using idType = size_t;

 public:
  MulticenteredMoleculeLJ() = default;

  /**
   * Constructor of the MulticenteredParticle Class
   * @param r Position of the particle.
   * @param v Velocity of the particle.
   * @param q Quaternion defining rotation of particle.
   * @param D Rotational velocity of the particle.
   * @param sites Vector of sites of the particle.
   * @param id Id of the particle.
   */
   MulticenteredMoleculeLJ(std::array<double, 3> r, std::array<double, 3> v, std::array<double, 4> q,
                            std::array<double, 3> angularVel, std::vector<std::array<double,3>> sitePosLJ, unsigned long id)
                            : _r(r), _v(v), _q(q), _angularVel(angularVel), _sitePosLJ(sitePosLJ), _id(id) {}

   /**
    * Destructor of the MulticenteredParticle class.
    */
    virtual ~MulticenteredMoleculeLJ() = default;

   protected:
   /**
    * (Centre of) Particle position as 3D coords
    */
   std::array<double,3> _r;

   /**
    * Velocity of particle.
    */
   std::array<double,3> _v;

   /**
    * Force experienced by particle.
    */
   std::array<double,3> _f;

   /**
    * Rotational direction of particle as quaternion.
    */
   std::array<double,4> _q;

   /**
    * Angular velocity of the particle
    */
   std::array<double,3> _angularVel;

   /**
    * Net torque applied to particle. (+ve = counterclockwise)
    */
   double _t;

   /**
    * Position of Lennard-Jones sites relative to CoM.
    */
   std::vector<std::array<double,3>> _sitePosLJ;

   /**
    * Particle id.
    */
   idType _id{};

   /**
   * Defines the state of the ownership of the particle.
    */
   autopas::OwnershipState _ownershipState{autopas::OwnershipState::owned};

  public:
   /**
   * get the force acting on the particle
   * @return force
    */
   //[[nodiscard]] const std::array<double, 3> &getF() const { return _f; }

   /**
   * Set the force acting on the particle
   * @param f force
    */
   //void setF(const std::array<double, 3> &f) { _f = f; }

   /**
   * Add a partial force to the force acting on the particle
   * @param f partial force to be added
    */
   //void addF(const std::array<double, 3> &f) { _f = autopas::utils::ArrayMath::add(_f, f); }

   /**
   * Substract a partial force from the force acting on the particle
   * @param f partial force to be substracted
    */
   //void subF(const std::array<double, 3> &f) { _f = autopas::utils::ArrayMath::sub(_f, f); }

   /**
   * Get the id of the particle
   * @return id
    */
   //idType getID() const { return _id; }

   /**
   * Set the id of the particle
   * @param id id
    */
   //void setID(idType id) { _id = id; }

   /**
   * Get the position of the particle
   * @return current position
    */
   //[[nodiscard]] const std::array<double, 3> &getR() const { return _r; }

   /**
   * Set the position of the particle
   * @param r new position
    */
   //void setR(const std::array<double, 3> &r) { _r = r; }

   /**
   * Add a distance vector to the position of the particle
   * @param r vector to be added
    */
   //void addR(const std::array<double, 3> &r) { _r = autopas::utils::ArrayMath::add(_r, r); }

   /**
   * Get the velocity of the particle
   * @return current velocity
    */
   //[[nodiscard]] const std::array<double, 3> &getV() const { return _v; }

   /**
   * Set the velocity of the particle
   * @param v new velocity
    */
   //void setV(const std::array<double, 3> &v) { _v = v; }

   /**
   * Add a vector to the current velocity of the particle
   * @param v vector to be added
    */
   //void addV(const std::array<double, 3> &v) { _v = autopas::utils::ArrayMath::add(_v, v); }

   /**
   * Get the quaternion defining rotation
   * @return quaternion defining rotation
    */
   [[nodiscard]] const std::array<double, 4> &getQ() const { return _q; }

   /**
   * Set the quaternion defining rotation
   * @param q quaternion defining rotation
    */
   void setQ(const std::array<double, 4> &q) { _q = q; }

   /**
   * Get the angular velocity
   * @return angular velocity
    */
   [[nodiscard]] const std::array<double, 3> &getAngularVel() const { return _angularVel; }

   /**
   * Set the angular velocity
   * @param angularVelocity
    */
   void setD(const std::array<double, 3> &angularVel) { _angularVel = angularVel; }

   /**
    * Add Lennard-Jones site
    * @param sitePos relative position of LJ site
    */
    void addSiteLJ(const std::array<double,3> &sitePos) {_sitePosLJ.push_back(sitePos); }

    /**
    * Set Lennard-Jones sites
    * @param sitePosLJ relative position of LJ sites
     */
    void setSitesLJ(const std::vector<std::array<double,3>> &sitePosLJ) {_sitePosLJ = sitePosLJ; }

    /**
     * Get Lennard-Jones site relative positions.
     * @return LJ site relative positions
     */
     [[nodiscard]] const std::vector<std::array<double,3>> &getSitesLJ() const { return _sitePosLJ; }



    /**
   * Creates a string containing all data of the particle.
   * @return String representation.
     */
    [[nodiscard]] virtual std::string toString() const {
      std::ostringstream text;
      std::ostringstream lj_str;
      // clang-format off
      text << "Particle"
         << "\nID                 : " << _id
         << "\nPosition           : "
         << autopas::utils::ArrayUtils::to_string(_r)
         << "\nVelocity           : "
         << autopas::utils::ArrayUtils::to_string(_v)
         << "\nForce              : "
         << autopas::utils::ArrayUtils::to_string(_f)
         << "\nQuaternion         : "
         << autopas::utils::ArrayUtils::to_string(_q)
         << "\nRotational Velocity: "
         << autopas::utils::ArrayUtils::to_string(_angularVel)
         << "\nOwnershipState     : "
         << _ownershipState;
      // clang-format on
      return text.str();
    }

    using SoAArraysType =
        typename autopas::utils::SoAType<MulticenteredMoleculeLJ *, idType /*id*/ , double /*x*/,
          double /*y*/, double /*z*/, double /*fx*/, double /*fy*/, double /*fz*/,
          autopas::OwnershipState /*ownershipState*/>::Type;


};

