/*
  Copyright (C) 2012,2013
      Max Planck Institute for Polymer Research
  Copyright (C) 2008,2009,2010,2011
      Max-Planck-Institute for Polymer Research & Fraunhofer SCAI
  
  This file is part of ESPResSo++.
  
  ESPResSo++ is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  ESPResSo++ is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>. 
*/

// ESPP_CLASS 
#ifndef _INTERACTION_VERLETLISTGMHADRESSINTERACTIONTEMPLATE_HPP
#define _INTERACTION_VERLETLISTGMHADRESSINTERACTIONTEMPLATE_HPP

//#include <typeinfo>

#include "System.hpp"
#include "bc/BC.hpp"
#include "iterator/CellListIterator.hpp"

#include "types.hpp"
#include "Interaction.hpp"
#include "Real3D.hpp"
#include "Tensor.hpp"
#include "Particle.hpp"
#include "VerletListAdress.hpp"
#include "FixedTupleListAdress.hpp"
#include "esutil/Array2D.hpp"
#include "SystemAccess.hpp"

namespace espressopp {
  namespace interaction {
    template < typename _PotentialAT, typename _PotentialCG >
    class VerletListGMHadressInteractionTemplate: public Interaction {
    
    protected:
      typedef _PotentialAT PotentialAT;
      typedef _PotentialCG PotentialCG;
    
    public:
      VerletListGMHadressInteractionTemplate
      (shared_ptr<VerletListAdress> _verletList, shared_ptr<FixedTupleListAdress> _fixedtupleList)
                : verletList(_verletList), fixedtupleList(_fixedtupleList) {

          potentialArrayAT = esutil::Array2D<PotentialAT, esutil::enlarge>(0, 0, PotentialAT());
          potentialArrayCG = esutil::Array2D<PotentialCG, esutil::enlarge>(0, 0, PotentialCG());

          // AdResS stuff
          dhy = verletList->getHy();
          pidhy2 = M_PI/(dhy * 2.0);
          dex = verletList->getEx();
          dex2 = dex * dex;
          dexdhy = dex + verletList->getHy();
          dexdhy2 = dexdhy * dexdhy;
          
          ntypes = 0;
          kbT = 0.0;
          kbT_is_calculated = false;
      }
                
      void
      setVerletList(shared_ptr < VerletListAdress > _verletList) {
        verletList = _verletList;
      }

      shared_ptr<VerletListAdress> getVerletList() {
        return verletList;
      }

      void
      setFixedTupleList(shared_ptr<FixedTupleListAdress> _fixedtupleList) {
          fixedtupleList = _fixedtupleList;
      }

      void
      setPotentialAT(int type1, int type2, const PotentialAT &potential) {
          // typeX+1 because i<ntypes
          ntypes = std::max(ntypes, std::max(type1+1, type2+1));
        
          potentialArrayAT.at(type1, type2) = potential;
          if (type1 != type2) { // add potential in the other direction
             potentialArrayAT.at(type2, type1) = potential;
          }
      }

      void
      setPotentialCG(int type1, int type2, const PotentialCG &potential) {
          // typeX+1 because i<ntypes
          ntypes = std::max(ntypes, std::max(type1+1, type2+1));
        
         potentialArrayCG.at(type1, type2) = potential;
         if (type1 != type2) { // add potential in the other direction
             potentialArrayCG.at(type2, type1) = potential;
         }
      }

      PotentialAT &getPotentialAT(int type1, int type2) {
        return potentialArrayAT.at(type1, type2);
      }

      PotentialCG &getPotentialCG(int type1, int type2) {
        return potentialArrayCG.at(type1, type2);
      }

      virtual void addForces();
      virtual real computeEnergy();
      virtual real computeEnergyAA();
      virtual real computeEnergyCG();
      virtual void computeVirialX(std::vector<real> &p_xx_total, int bins); 
      virtual real computeVirial();
      virtual void computeVirialTensor(Tensor& w);
      virtual void computeVirialTensor(Tensor& w, real z);
      virtual void computeVirialTensor(Tensor *w, int n);
      virtual real getMaxCutoff();
      virtual int bondType() { return Nonbonded; }
      
    protected: 
      int ntypes;
      shared_ptr<VerletListAdress> verletList;
      shared_ptr<FixedTupleListAdress> fixedtupleList;
      esutil::Array2D<PotentialAT, esutil::enlarge> potentialArrayAT;
      esutil::Array2D<PotentialCG, esutil::enlarge> potentialArrayCG;

      // AdResS stuff
      real pidhy2; // pi / (dhy * 2)
      real dexdhy; // dex + dhy
      real dexdhy2; // dexdhy^2
      real dex;
      real dhy;
      real dex2; // dex^2
      //std::map<Particle*, real> weights;
      std::map<Particle*, real> energyCG;   // V_CG in hybrid region for GM-AdResS
      std::map<Particle*, real> energyAT;   // V_AT in hybrid region for GM-AdResS
      std::set<Particle*> adrZone;  // Virtual particles in AdResS zone (HY and AT region)
      std::set<Particle*> cgZone;

      // Needed for GM-H-AdResS
      real kbT;
      bool kbT_is_calculated;
      void compute_kbT();
      void getPairEnergies(Particle &p1, Particle &p2, real eAT, real eCG);
      void computeParticleEnergies(); // TODO JD instead of calcParticleEnergies() ??
      void inline clearParticleEnergies() {
          energyAT.clear();
          energyCG.clear();
      }

      // AdResS Weighting function
      /*real weight(real distanceSqr){
          if (dex2 > distanceSqr) return 1.0;
          else if (dexdhy2 < distanceSqr) return 0.0;
          else {
              real argument = sqrt(distanceSqr) - dex;
              return 1.0-(30.0/(pow(dhy, 5.0)))*(1.0/5.0*pow(argument, 5.0)-dhy/2.0*pow(argument, 4.0)+1.0/3.0*pow(argument, 3.0)*dhy*dhy);
              //return pow(cos(pidhy2 * argument),2.0); // for cosine squared weighting function
          }
      }
      real weightderivative(real distance){
          real argument = distance - dex;
          return -(30.0/(pow(dhy, 5.0)))*(pow(argument, 4.0)-2.0*dhy*pow(argument, 3.0)+argument*argument*dhy*dhy);
          //return -pidhy2 * 2.0 * cos(pidhy2*argument) * sin(pidhy2*argument); // for cosine squared weighting function
      }*/

      // Compute center of mass and set the weights for virtual particles in AdResS zone (HY and AT region).
      //void makeWeights(){
      /*    
          std::set<Particle*> cgZone = verletList->getCGZone();
          for (std::set<Particle*>::iterator it=cgZone.begin();
              it != cgZone.end(); ++it) {

              Particle &vp = **it;
              
              FixedTupleListAdress::iterator it3;
              it3 = fixedtupleList->find(&vp);

              if (it3 != fixedtupleList->end()) {

                  std::vector<Particle*> atList;
                  atList = it3->second;

                  // Compute center of mass
                  Real3D cmp(0.0, 0.0, 0.0); // center of mass position
                  Real3D cmv(0.0, 0.0, 0.0); // center of mass velocity
                  //real M = vp.getMass(); // sum of mass of AT particles
                  for (std::vector<Particle*>::iterator it2 = atList.begin();
                                       it2 != atList.end(); ++it2) {
                      Particle &at = **it2;
                      //Real3D d1 = at.position() - vp.position();
                      //Real3D d1;
                      //verletList->getSystem()->bc->getMinimumImageVectorBox(d1, at.position(), vp.position());
                      //cmp += at.mass() * d1;

                      cmp += at.mass() * at.position();
                      cmv += at.mass() * at.velocity();
                  }
                  cmp /= vp.getMass();
                  cmv /= vp.getMass();
                  //cmp += vp.position(); // cmp is a relative position
                  //std::cout << " cmp M: "  << M << "\n\n";
                  //std::cout << "  moving VP to " << cmp << ", velocitiy is " << cmv << "\n";

                  // update (overwrite) the position and velocity of the VP
                  vp.position() = cmp;
                  vp.velocity() = cmv;

                  // calculate distance to nearest adress particle or center
                  std::vector<Real3D*>::iterator it2 = verletList->getAdrPositions().begin();
                  Real3D pa = **it2; // position of adress particle
                  Real3D d1(0.0, 0.0, 0.0);          
                  //Real3D d1 = vp.position() - pa;                                                      // X SPLIT VS SPHERE CHANGE
                  //real d1 = vp.position()[0] - pa[0];                                                // X SPLIT VS SPHERE CHANGE
                  verletList->getSystem()->bc->getMinimumImageVector(d1, vp.position(), pa);
                  //real min1sq = d1.sqr();  // set min1sq before loop                                   // X SPLIT VS SPHERE CHANGE
                  real min1sq = d1[0]*d1[0];   // set min1sq before loop                                   // X SPLIT VS SPHERE CHANGE
                  ++it2;
                  for (; it2 != verletList->getAdrPositions().end(); ++it2) {
                       pa = **it2;
                       //d1 = vp.position() - pa;                                                          // X SPLIT VS SPHERE CHANGE
                       //d1 = vp.position()[0] - pa[0];                                                  // X SPLIT VS SPHERE CHANGE
                       verletList->getSystem()->bc->getMinimumImageVector(d1, vp.position(), pa);
                       //real distsq1 = d1.sqr();                                                          // X SPLIT VS SPHERE CHANGE
                       real distsq1 = d1[0]*d1[0];                                                           // X SPLIT VS SPHERE CHANGE
                       //std::cout << pa << " " << sqrt(distsq1) << "\n";
                       if (distsq1 < min1sq) min1sq = distsq1;
                  }
                  
                  real w = weight(min1sq);                  
                  vp.lambda() = w;  
                  
                  if(w!=0.0) std::cout << "lambda in CG region not equal to 0.0. ID: " << vp.id() << " Position: " << vp.position() << "\n";
                  //weights.insert(std::make_pair(&vp, w));
                  
                  real wDeriv = weightderivative(sqrt(min1sq));
                  vp.lambdaDeriv() = wDeriv;
                  
              }
              else { // this should not happen
                  std::cout << " VP particle " << vp.id() << "-" << vp.ghost() << " not found in tuples ";
                  std::cout << " (" << vp.position() << ")\n";
                  exit(1);
                  return;
              }    
             //vp.lambda() = 0.0;
             //weights.insert(std::make_pair(&vp, 0.0));
          }
          
          adrZone = verletList->getAdrZone();
          for (std::set<Particle*>::iterator it=adrZone.begin();
                  it != adrZone.end(); ++it) {

              Particle &vp = **it;

              FixedTupleListAdress::iterator it3;
              it3 = fixedtupleList->find(&vp);

              if (it3 != fixedtupleList->end()) {

                  std::vector<Particle*> atList;
                  atList = it3->second;

                  // Compute center of mass
                  Real3D cmp(0.0, 0.0, 0.0); // center of mass position
                  Real3D cmv(0.0, 0.0, 0.0); // center of mass velocity
                  //real M = vp.getMass(); // sum of mass of AT particles
                  for (std::vector<Particle*>::iterator it2 = atList.begin();
                                       it2 != atList.end(); ++it2) {
                      Particle &at = **it2;
                      //Real3D d1 = at.position() - vp.position();
                      //Real3D d1;
                      //verletList->getSystem()->bc->getMinimumImageVectorBox(d1, at.position(), vp.position());
                      //cmp += at.mass() * d1;

                      cmp += at.mass() * at.position();
                      cmv += at.mass() * at.velocity();
                  }
                  cmp /= vp.getMass();
                  cmv /= vp.getMass();
                  //cmp += vp.position(); // cmp is a relative position
                  //std::cout << " cmp M: "  << M << "\n\n";
                  //std::cout << "  moving VP to " << cmp << ", velocitiy is " << cmv << "\n";

                  // update (overwrite) the position and velocity of the VP
                  vp.position() = cmp;
                  vp.velocity() = cmv;

                  // calculate distance to nearest adress particle or center
                  std::vector<Real3D*>::iterator it2 = verletList->getAdrPositions().begin();
                  Real3D pa = **it2; // position of adress particle
                  Real3D d1(0.0, 0.0, 0.0);
                  //Real3D d1 = vp.position() - pa;                                                      // X SPLIT VS SPHERE CHANGE
                  verletList->getSystem()->bc->getMinimumImageVector(d1, vp.position(), pa);
                  //real d1 = vp.position()[0] - pa[0];                                                // X SPLIT VS SPHERE CHANGE
                  //real min1sq = d1.sqr();  // set min1sq before loop                                   // X SPLIT VS SPHERE CHANGE
                  //real min1sq = d1*d1;   // set min1sq before loop                                   // X SPLIT VS SPHERE CHANGE
                  real min1sq = d1[0]*d1[0];   // set min1sq before loop                                   // X SPLIT VS SPHERE CHANGE
                  ++it2;
                  for (; it2 != verletList->getAdrPositions().end(); ++it2) {
                       pa = **it2;
                       //d1 = vp.position() - pa;                                                          // X SPLIT VS SPHERE CHANGE
                       //d1 = vp.position()[0] - pa[0];
                       verletList->getSystem()->bc->getMinimumImageVector(d1, vp.position(), pa);        // X SPLIT VS SPHERE CHANGE
                       //real distsq1 = d1.sqr();                                                          // X SPLIT VS SPHERE CHANGE
                       real distsq1 = d1[0]*d1[0];                                                           // X SPLIT VS SPHERE CHANGE
                       //std::cout << pa << " " << sqrt(distsq1) << "\n";
                       if (distsq1 < min1sq) min1sq = distsq1;
                  }
                  
                  real w = weight(min1sq);                  
                  vp.lambda() = w;                  
                  //weights.insert(std::make_pair(&vp, w));
                  
                  real wDeriv = weightderivative(sqrt(min1sq));
                  vp.lambdaDeriv() = wDeriv;
                  
              }
              else { // this should not happen
                  std::cout << " VP particle " << vp.id() << "-" << vp.ghost() << " not found in tuples ";
                  std::cout << " (" << vp.position() << ")\n";
                  exit(1);
                  return;
              }
          }*/
        //}
    };

    //////////////////////////////////////////////////
    // INLINE IMPLEMENTATION
    //////////////////////////////////////////////////
    template < typename _PotentialAT, typename _PotentialCG > inline void
    VerletListGMHadressInteractionTemplate < _PotentialAT, _PotentialCG >::
    addForces() {
      LOG4ESPP_INFO(theLogger, "add forces computed by the Verlet List");
      
      std::set<Particle*> cgZone = verletList->getCGZone();
      std::set<Particle*> adrZone = verletList->getAdrZone();

      // For GM-H-AdResS
      if (kbT_is_calculated) {} else {compute_kbT();}
      computeParticleEnergies(); // TODO JD instead of calcParticleEnergies() ??

      // Pairs not inside the AdResS Zone (CG region)
      // REMOVE FOR IDEAL GAS
      for (PairList::Iterator it(verletList->getPairs()); it.isValid(); ++it) {

        Particle &p1 = *it->first;
        Particle &p2 = *it->second;
        int type1 = p1.type();
        int type2 = p2.type();

        const PotentialCG &potentialCG = getPotentialCG(type1, type2);

        Real3D force(0.0, 0.0, 0.0);

        // CG forces
        if(potentialCG._computeForce(force, p1, p2)) {
          p1.force() += force;
          p2.force() -= force;
        }
      }
      // REMOVE FOR IDEAL GAS
      
      // Compute forces (AT and VP) of Pairs inside AdResS zone
      for (PairList::Iterator it(verletList->getAdrPairs()); it.isValid(); ++it) {
         real w1, w2;
         // these are the two VP interacting
         Particle &p1 = *it->first;
         Particle &p2 = *it->second;

         w1 = p1.lambda();               
         w2 = p2.lambda();
        
         real w12 = (w1 + w2)/2.0;  // H-AdResS

         // GM-H-AdResS
         real eAT1 = energyAT[&p1];
         real eAT2 = energyAT[&p2];
         real eCG1 = energyCG[&p1];
         real eCG2 = energyCG[&p2];
         real gAT1 = w1 * exp(-eAT1 / kbT) / (w1 * exp(-eAT1 / kbT) + (1 - w1) * exp(-eCG1 / kbT));
         real gAT2 = w2 * exp(-eAT2 / kbT) / (w2 * exp(-eAT2 / kbT) + (1 - w2) * exp(-eCG2 / kbT));
         real gAT_avg = (gAT1 + gAT2) / 2.0;

         // REMOVE FOR IDEAL GAS
         // force between VP particles
         int type1 = p1.type();
         int type2 = p2.type();
         const PotentialCG &potentialCG = getPotentialCG(type1, type2);
         Real3D forcevp(0.0, 0.0, 0.0);
                if (w12 != 1.0) { // calculate VP force if both VP are outside AT region (CG-HY, HY-HY)
                    if (potentialCG._computeForce(forcevp, p1, p2)) {
                        forcevp *= (1.0 - gAT_avg);
                        p1.force() += forcevp;
                        p2.force() -= forcevp;
                        
                        // TEST ITERATIVE PRESSURE FEC!
                        //if (w12 != 0.0) {
                            //p1.drift() += forcevp[0];
                            //p2.drift() -= forcevp[0];
                        //}
                        // TEST ITERATIVE PRESSURE FEC!                        
                        
                    }
                }
         // REMOVE FOR IDEAL GAS
         
         
         
         // force between AT particles
         if (w12 != 0.0) { // calculate AT force if both VP are outside CG region (HY-HY, HY-AT, AT-AT)
             FixedTupleListAdress::iterator it3;
             FixedTupleListAdress::iterator it4;
             it3 = fixedtupleList->find(&p1);
             it4 = fixedtupleList->find(&p2);

             if (it3 != fixedtupleList->end() && it4 != fixedtupleList->end()) {

                 std::vector<Particle*> atList1;
                 std::vector<Particle*> atList2;
                 atList1 = it3->second;
                 atList2 = it4->second;

                 //std::cout << "AT forces ...\n";
                 for (std::vector<Particle*>::iterator itv = atList1.begin();
                         itv != atList1.end(); ++itv) {

                     Particle &p3 = **itv;

                     for (std::vector<Particle*>::iterator itv2 = atList2.begin();
                                          itv2 != atList2.end(); ++itv2) {

                         Particle &p4 = **itv2;

                         // AT forces
                         const PotentialAT &potentialAT = getPotentialAT(p3.type(), p4.type());
                         Real3D force(0.0, 0.0, 0.0);
                         if(potentialAT._computeForce(force, p3, p4)) {
                             force *= gAT_avg;
                             p3.force() += force;
                             p4.force() -= force;
                             
                            // TEST ITERATIVE PRESSURE FEC!
                            //if (w12 != 1.0) {
                                //p1.drift() += force[0];
                                //p2.drift() -= force[0];
                            //}
                            // TEST ITERATIVE PRESSURE FEC!
                         }                       
                     }

                 }
             }
             else { // this should not happen
                 std::cout << " one of the VP particles not found in tuples: " << p1.id() << "-" <<
                         p1.ghost() << ", " << p2.id() << "-" << p2.ghost();
                 std::cout << " (" << p1.position() << ") (" << p2.position() << ")\n";
                 exit(1);
                 return;
             }
         }
      }
      
      // GM-H-AdResS - Drift Term
      // Iterate over all particles in the hybrid region and calculate drift force
      for (std::set<Particle*>::iterator it=adrZone.begin();
        it != adrZone.end(); ++it) {   // Iterate over all particles
          Particle &vp = **it;
          real w = vp.lambda();
          real eAT = energyAT.find(&vp)->second;
          real eCG = energyCG.find(&vp)->second;
          real gAT = w * exp(-eAT / kbT) / (w * exp(-eAT / kbT) + (1 - w) * exp(-eCG / kbT));
          //real w = weights.find(&vp)->second;
                  
          //if(w!=1.0 && w!=0.0){   //   only chose those in the hybrid region
          if(w<0.9999999 && w>0.0000001){   //   only chose those in the hybrid region
              // Calculate unit vector for the drift force
              std::vector<Real3D*>::iterator it2 = verletList->getAdrPositions().begin();
              Real3D pa = **it2; // position of adress particle
              Real3D mindriftforce(0.0, 0.0, 0.0);
              //Real3D mindriftforce = vp.position() - pa;                                                           // X SPLIT VS SPHERE CHANGE
              //real mindriftforce = vp.position()[0] - pa[0];                                                         // X SPLIT VS SPHERE CHANGE
              verletList->getSystem()->bc->getMinimumImageVector(mindriftforce, vp.position(), pa);
              real min1sq = mindriftforce[0]*mindriftforce[0]; // mindriftforce.sqr(); // set min1sq before loop
              ++it2;
              for (; it2 != verletList->getAdrPositions().end(); ++it2) {
                   pa = **it2;
                   Real3D driftforce(0.0, 0.0, 0.0);
                   //Real3D driftforce = vp.position() - pa;                                                          // X SPLIT VS SPHERE CHANGE
                   //real driftforce = vp.position()[0] - pa[0];                                                    // X SPLIT VS SPHERE CHANGE
                   verletList->getSystem()->bc->getMinimumImageVector(driftforce, vp.position(), pa);
                   //real distsq1 = driftforce.sqr();//driftforce*driftforce; //driftforce.sqr();                     // X SPLIT VS SPHERE CHANGE
                   real distsq1 = driftforce[0]*driftforce[0];                                                          // X SPLIT VS SPHERE CHANGE
                   //std::cout << pa << " " << sqrt(distsq1) << "\n";
                   if (distsq1 < min1sq) {
                        min1sq = distsq1;
                        mindriftforce = driftforce;
                   }
              }
              min1sq = sqrt(min1sq);   // distance to nearest adress particle or center
              real mindriftforceX = (1.0/min1sq)*mindriftforce[0];  // normalized driftforce vector
              //mindriftforce *= weightderivative(min1sq);  // multiplication with derivative of the weighting function
              
              // Calculate the drift force given that direction.
              mindriftforceX *= 0.5;
              mindriftforceX *= kbT * (gAT / w - (1 - gAT) / (1 - w)) ;   // get the energy differences which were calculated previously and put in drift force
              
              vp.drift() += mindriftforceX ;//* vp.lambdaDeriv();    // USE ONLY LIKE THAT, IF DOING ITERATIVE FEC INCLUDING ITERATIVE PRESSURE FEC               
              
              mindriftforceX *= vp.lambdaDeriv();  // multiplication with derivative of the weighting function
              //vp.force() += mindriftforce;   // add drift force to virtual particles                                                                    // X SPLIT VS SPHERE CHANGE
              Real3D driftforceadd(mindriftforceX,0.0,0.0);                                                                                            // X SPLIT VS SPHERE CHANGE
              //Real3D driftforceadd(0.0,0.0,0.0);   
              vp.force() += driftforceadd;             // Goes in, if one wants to apply the "normal" drift force - also improve using [0] ...           // X SPLIT VS SPHERE CHANGE
              //std::cout << "Added Drift Force: " << driftforceadd << " for particle at pos(x).: " << vp.position()[0] << "\n";
              
          }   
      }
      clearParticleEnergies();
    }
    
    // Energy calculation does currently only work if integrator.run( ) (also with 0) and decompose have been executed before. This is due to the initialization of the tuples.
    template < typename _PotentialAT, typename _PotentialCG >
    inline real
    VerletListGMHadressInteractionTemplate < _PotentialAT, _PotentialCG >::
    computeEnergy() {
      LOG4ESPP_INFO(theLogger, "compute energy of the Verlet list pairs");

      // For GM-H-AdResS
      if (kbT_is_calculated) {} else {compute_kbT();}
      computeParticleEnergies();

      // REMOVE FOR IDEAL GAS 
      real e = 0.0;        
      for (PairList::Iterator it(verletList->getPairs()); 
           it.isValid(); ++it) {
          Particle &p1 = *it->first;
          Particle &p2 = *it->second;
          int type1 = p1.type();
          int type2 = p2.type();
          const PotentialCG &potential = getPotentialCG(type1, type2);
          e += potential._computeEnergy(p1, p2);
      }
      // REMOVE FOR IDEAL GAS
      
      // Iterate over all particles in the hybrid region and calculate drift force
      for (std::set<Particle*>::iterator it=adrZone.begin();
           it != adrZone.end(); ++it) {   // Iterate over all particles
          Particle &vp = **it;
          real w = vp.lambda();
          real eAT = energyAT.find(&vp)->second;
          real eCG = energyCG.find(&vp)->second;
          e += -kbT * log(w * exp(-eAT / kbT) + (1 - w) * exp(-eCG / kbT));
      }
      clearParticleEnergies();
      real esum;
      boost::mpi::all_reduce(*getVerletList()->getSystem()->comm, e, esum, std::plus<real>());
      return esum;      
    }
    
    
    template < typename _PotentialAT, typename _PotentialCG >
    inline real
    VerletListGMHadressInteractionTemplate < _PotentialAT, _PotentialCG >::
    computeEnergyAA() {
      LOG4ESPP_INFO(theLogger, "compute total AA energy of the Verlet list pairs");
      
      real e = 0.0;        
      for (PairList::Iterator it(verletList->getPairs()); 
           it.isValid(); ++it) {
          Particle &p1 = *it->first;
          Particle &p2 = *it->second;
          FixedTupleListAdress::iterator it3;
          FixedTupleListAdress::iterator it4;
          it3 = fixedtupleList->find(&p1);
          it4 = fixedtupleList->find(&p2);

          if (it3 != fixedtupleList->end() && it4 != fixedtupleList->end()) {
              std::vector<Particle*> atList1;
              std::vector<Particle*> atList2;
              atList1 = it3->second;
              atList2 = it4->second;

              for (std::vector<Particle*>::iterator itv = atList1.begin();
                      itv != atList1.end(); ++itv) {

                  Particle &p3 = **itv;
                  for (std::vector<Particle*>::iterator itv2 = atList2.begin();
                                       itv2 != atList2.end(); ++itv2) {
                      Particle &p4 = **itv2;

                      // AT energies
                      const PotentialAT &potentialAT = getPotentialAT(p3.type(), p4.type());
                      e += potentialAT._computeEnergy(p3, p4);

                  }                  
              }              
          }
      }

      for (PairList::Iterator it(verletList->getAdrPairs()); 
           it.isValid(); ++it) {
          Particle &p1 = *it->first;
          Particle &p2 = *it->second;                
          FixedTupleListAdress::iterator it3;
          FixedTupleListAdress::iterator it4;
          it3 = fixedtupleList->find(&p1);
          it4 = fixedtupleList->find(&p2);

          if (it3 != fixedtupleList->end() && it4 != fixedtupleList->end()) {
              std::vector<Particle*> atList1;
              std::vector<Particle*> atList2;
              atList1 = it3->second;
              atList2 = it4->second;

              for (std::vector<Particle*>::iterator itv = atList1.begin();
                      itv != atList1.end(); ++itv) {

                  Particle &p3 = **itv;
                  for (std::vector<Particle*>::iterator itv2 = atList2.begin();
                                       itv2 != atList2.end(); ++itv2) {
                      Particle &p4 = **itv2;

                      // AT energies
                      const PotentialAT &potentialAT = getPotentialAT(p3.type(), p4.type());
                      e += potentialAT._computeEnergy(p3, p4);

                  }                  
              }              
          }          
      }
       
      real esum;
      boost::mpi::all_reduce(*getVerletList()->getSystem()->comm, e, esum, std::plus<real>());
      return esum;      
    }
   
    
    template < typename _PotentialAT, typename _PotentialCG >
    inline real
    VerletListGMHadressInteractionTemplate < _PotentialAT, _PotentialCG >::
    computeEnergyCG() {
      LOG4ESPP_INFO(theLogger, "compute total CG energy of the Verlet list pairs");
      
      real e = 0.0;        
      for (PairList::Iterator it(verletList->getPairs()); 
           it.isValid(); ++it) {
          Particle &p1 = *it->first;
          Particle &p2 = *it->second;
          int type1 = p1.type();
          int type2 = p2.type();
          const PotentialCG &potential = getPotentialCG(type1, type2);
          e += potential._computeEnergy(p1, p2);
      }

      for (PairList::Iterator it(verletList->getAdrPairs()); 
           it.isValid(); ++it) {
          Particle &p1 = *it->first;
          Particle &p2 = *it->second;      
          int type1 = p1.type();
          int type2 = p2.type();
          const PotentialCG &potentialCG = getPotentialCG(type1, type2);
          e += potentialCG._computeEnergy(p1, p2);
      }
       
      real esum;
      boost::mpi::all_reduce(*getVerletList()->getSystem()->comm, e, esum, std::plus<real>());
      return esum;      
    }
    

    template < typename _PotentialAT, typename _PotentialCG > inline void
    VerletListGMHadressInteractionTemplate < _PotentialAT, _PotentialCG >::
    computeVirialX(std::vector<real> &p_xx_total, int bins) {
      LOG4ESPP_INFO(theLogger, "compute virial p_xx of the pressure tensor slabwise");
      
      // For GM-H-AdResS
      if (kbT_is_calculated) {} else {compute_kbT();}
      computeParticleEnergies();
      
      std::set<Particle*> cgZone = verletList->getCGZone();
      for (std::set<Particle*>::iterator it=cgZone.begin();
              it != cgZone.end(); ++it) {

          Particle &vp = **it;

          FixedTupleListAdress::iterator it3;
          it3 = fixedtupleList->find(&vp);

          if (it3 != fixedtupleList->end()) {

              std::vector<Particle*> atList;
              atList = it3->second;

              // compute center of mass
              Real3D cmp(0.0, 0.0, 0.0); // center of mass position
              //Real3D cmv(0.0, 0.0, 0.0); // center of mass velocity
              //real M = vp.getMass(); // sum of mass of AT particles
              for (std::vector<Particle*>::iterator it2 = atList.begin();
                                   it2 != atList.end(); ++it2) {
                  Particle *at = *it2;
                  //Real3D d1 = at.position() - vp.position();
                  //Real3D d1;
                  //verletList->getSystem()->bc->getMinimumImageVectorBox(d1, at.position(), vp.position());
                  //cmp += at.mass() * d1;

                  cmp += at->mass() * at->position();
                  //cmv += at.mass() * at.velocity();
              }
              cmp /= vp.getMass();
              //cmv /= vp.getMass();
              //cmp += vp.position(); // cmp is a relative position
              //std::cout << " cmp M: "  << M << "\n\n";
              //std::cout << "  moving VP to " << cmp << ", velocitiy is " << cmv << "\n";

              // update (overwrite) the position and velocity of the VP
              vp.position() = cmp;
              //vp.velocity() = cmv;

          }
          else { // this should not happen
              std::cout << " VP particle " << vp.id() << "-" << vp.ghost() << " not found in tuples ";
              std::cout << " (" << vp.position() << ")\n";
              exit(1);
              return;
          }
      }
      
      std::set<Particle*> adrZone = verletList->getAdrZone();
      for (std::set<Particle*>::iterator it=adrZone.begin();
              it != adrZone.end(); ++it) {

          Particle &vp = **it;

          FixedTupleListAdress::iterator it3;
          it3 = fixedtupleList->find(&vp);

          if (it3 != fixedtupleList->end()) {

              std::vector<Particle*> atList;
              atList = it3->second;

              // compute center of mass
              Real3D cmp(0.0, 0.0, 0.0); // center of mass position
              //Real3D cmv(0.0, 0.0, 0.0); // center of mass velocity
              //real M = vp.getMass(); // sum of mass of AT particles
              for (std::vector<Particle*>::iterator it2 = atList.begin();
                                   it2 != atList.end(); ++it2) {
                  Particle &at = **it2;
                  //Real3D d1 = at.position() - vp.position();
                  //Real3D d1;
                  //verletList->getSystem()->bc->getMinimumImageVectorBox(d1, at.position(), vp.position());
                  //cmp += at.mass() * d1;

                  cmp += at.mass() * at.position();
                  //cmv += at.mass() * at.velocity();
              }
              cmp /= vp.getMass();
              //cmv /= vp.getMass();
              //cmp += vp.position(); // cmp is a relative position
              //std::cout << " cmp M: "  << M << "\n\n";
              //std::cout << "  moving VP to " << cmp << ", velocitiy is " << cmv << "\n";

              // update (overwrite) the position and velocity of the VP
              vp.position() = cmp;
              //vp.velocity() = cmv;

          }
          else { // this should not happen
              std::cout << " VP particle " << vp.id() << "-" << vp.ghost() << " not found in tuples ";
              std::cout << " (" << vp.position() << ")\n";
              exit(1);
              return;
          }
      }
      
      int i = 0;
      int bin1 = 0;
      int bin2 = 0;
      
      System& system = verletList->getSystemRef();
      Real3D Li = system.bc->getBoxL();
      real Delta_x = Li[0] / (real)bins;
      real Volume = Li[1] * Li[2] * Delta_x;
      size_t size = bins;
      std::vector <real> p_xx_local(size);      

      for (i = 0; i < bins; ++i)
        {
          p_xx_local[i] = 0.0;
        }
      
      for (PairList::Iterator it(verletList->getPairs());                
           it.isValid(); ++it) {                                         
        Particle &p1 = *it->first;                                       
        Particle &p2 = *it->second;                                      
        int type1 = p1.type();                                           
        int type2 = p2.type();
        const PotentialCG &potential = getPotentialCG(type1, type2);

        Real3D force(0.0, 0.0, 0.0);
        if(potential._computeForce(force, p1, p2)) {
          Real3D dist = p1.position() - p2.position();
          real vir_temp = 0.5 * dist[0] * force[0];
          
          if (p1.position()[0] > Li[0])
          {
              real p1_wrap = p1.position()[0] - Li[0];
              bin1 = floor (p1_wrap / Delta_x);    
          }         
          else if (p1.position()[0] < 0.0)
          {
              real p1_wrap = p1.position()[0] + Li[0];
              bin1 = floor (p1_wrap / Delta_x);    
          }
          else
          {
              bin1 = floor (p1.position()[0] / Delta_x);          
          }
          
          if (p2.position()[0] > Li[0])
          {
              real p2_wrap = p2.position()[0] - Li[0];
              bin2 = floor (p2_wrap / Delta_x);     
          }         
          else if (p2.position()[0] < 0.0)
          {
              real p2_wrap = p2.position()[0] + Li[0];
              bin2 = floor (p2_wrap / Delta_x);     
          }
          else
          {
              bin2 = floor (p2.position()[0] / Delta_x);          
          }             
         
          if (bin1 >= p_xx_local.size() || bin2 >= p_xx_local.size()){
              std::cout << "p_xx_local.size() " << p_xx_local.size() << "\n";
              std::cout << "bin1 " << bin1 << " bin2 " << bin2 << "\n";
              std::cout << "p1.position()[0] " << p1.position()[0] << " p2.position()[0]" << p2.position()[0] << "\n";
              std::cout << "FATAL ERROR: computeVirialX error" << "\n";
              exit(0);
          }
          
          p_xx_local.at(bin1) += vir_temp;
          p_xx_local.at(bin2) += vir_temp;
        }
      }
      
      //if (KTI == false) {
      //makeWeights();
      //}
      
      for (PairList::Iterator it(verletList->getAdrPairs()); it.isValid(); ++it) {
         real w1, w2;
         // these are the two VP interacting
         Particle &p1 = *it->first;
         Particle &p2 = *it->second;
     
         Real3D dist = p1.position() - p2.position();
         w1 = p1.lambda();         
         w2 = p2.lambda();
         real w12 = (w1 + w2)/2.0;  // H-AdResS

          // GM-H-AdResS
         real eAT1 = energyAT[&p1];
         real eAT2 = energyAT[&p2];
         real eCG1 = energyCG[&p1];
         real eCG2 = energyCG[&p2];
         real gAT1 = w1 * exp(-eAT1 / kbT) / (w1 * exp(-eAT1 / kbT) + (1 - w1) * exp(-eCG1 / kbT));
         real gAT2 = w2 * exp(-eAT2 / kbT) / (w2 * exp(-eAT2 / kbT) + (1 - w2) * exp(-eCG2 / kbT));
         real gAT_avg = (gAT1 + gAT2) / 2.0;

         if (p1.position()[0] > Li[0])
         {
             real p1_wrap = p1.position()[0] - Li[0];
             bin1 = floor (p1_wrap / Delta_x);    
         }         
         else if (p1.position()[0] < 0.0)
         {
             real p1_wrap = p1.position()[0] + Li[0];
             bin1 = floor (p1_wrap / Delta_x);    
         }
         else
         {
             bin1 = floor (p1.position()[0] / Delta_x);          
         }
 
         if (p2.position()[0] > Li[0])
         {
             real p2_wrap = p2.position()[0] - Li[0];
             bin2 = floor (p2_wrap / Delta_x);     
         }         
         else if (p2.position()[0] < 0.0)
         {
             real p2_wrap = p2.position()[0] + Li[0];
             bin2 = floor (p2_wrap / Delta_x);     
         }
         else
         {
             bin2 = floor (p2.position()[0] / Delta_x);          
         }
         
         // force between VP particles
         int type1 = p1.type();
         int type2 = p2.type();
         const PotentialCG &potentialCG = getPotentialCG(type1, type2);
         Real3D forcevp(0.0, 0.0, 0.0);
                if (w12 != 1.0) { // calculate VP force if both VP are outside AT region (CG-HY, HY-HY)
                    if (potentialCG._computeForce(forcevp, p1, p2)) {
                          forcevp *= (1.0 - gAT_avg);
                          real vir_temp = 0.5 * dist[0] * forcevp[0];                          
                          p_xx_local.at(bin1) += vir_temp;
                          p_xx_local.at(bin2) += vir_temp;  

                    }
                }
       
         // force between AT particles
         if (w12 != 0.0) { // calculate AT force if both VP are outside CG region (HY-HY, HY-AT, AT-AT)
             FixedTupleListAdress::iterator it3;
             FixedTupleListAdress::iterator it4;
             it3 = fixedtupleList->find(&p1);
             it4 = fixedtupleList->find(&p2);

             //std::cout << "Interaction " << p1.id() << " - " << p2.id() << "\n";
             if (it3 != fixedtupleList->end() && it4 != fixedtupleList->end()) {

                 std::vector<Particle*> atList1;
                 std::vector<Particle*> atList2;
                 atList1 = it3->second;
                 atList2 = it4->second;
                 
                 Real3D force_temp(0.0, 0.0, 0.0);
                 
                 for (std::vector<Particle*>::iterator itv = atList1.begin();
                         itv != atList1.end(); ++itv) {

                     Particle &p3 = **itv;

                     for (std::vector<Particle*>::iterator itv2 = atList2.begin();
                                          itv2 != atList2.end(); ++itv2) {

                         Particle &p4 = **itv2;

                         // AT forces
                         const PotentialAT &potentialAT = getPotentialAT(p3.type(), p4.type());
                         Real3D force(0.0, 0.0, 0.0);
                         if(potentialAT._computeForce(force, p3, p4)) {
                             force_temp += force; 
                         }
                     }
                 }
                 
                 force_temp *= gAT_avg;
                 real vir_temp = 0.5 * dist[0] * force_temp[0];                
                 p_xx_local.at(bin1) += vir_temp;
                 p_xx_local.at(bin2) += vir_temp;                                                                          
             }
             else { // this should not happen
                 std::cout << " one of the VP particles not found in tuples: " << p1.id() << "-" <<
                         p1.ghost() << ", " << p2.id() << "-" << p2.ghost();
                 std::cout << " (" << p1.position() << ") (" << p2.position() << ")\n";
                 exit(1);
                 return;
             }
         }
      }

      std::vector <real> p_xx_sum(size);
      for (i = 0; i < bins; ++i)
      {
          p_xx_sum.at(i) = 0.0;         
          boost::mpi::all_reduce(*mpiWorld, p_xx_local.at(i), p_xx_sum.at(i), std::plus<real>());
      }
      std::transform(p_xx_sum.begin(), p_xx_sum.end(), p_xx_sum.begin(),std::bind2nd(std::divides<real>(),Volume)); 
      for (i = 0; i < bins; ++i)
      {
          p_xx_total.at(i) += p_xx_sum.at(i);
      }
      clearParticleEnergies();
    }



    template < typename _PotentialAT, typename _PotentialCG > inline real
    VerletListGMHadressInteractionTemplate < _PotentialAT, _PotentialCG >::
    computeVirial() {
      LOG4ESPP_INFO(theLogger, "compute the virial for the Verlet List");
      
      /*std::set<Particle*> cgZone = verletList->getCGZone();
      for (std::set<Particle*>::iterator it=cgZone.begin();
              it != cgZone.end(); ++it) {

          Particle &vp = **it;

          FixedTupleListAdress::iterator it3;
          it3 = fixedtupleList->find(&vp);

          if (it3 != fixedtupleList->end()) {

              std::vector<Particle*> atList;
              atList = it3->second;

              // compute center of mass
              Real3D cmp(0.0, 0.0, 0.0); // center of mass position
              //Real3D cmv(0.0, 0.0, 0.0); // center of mass velocity
              //real M = vp.getMass(); // sum of mass of AT particles
              for (std::vector<Particle*>::iterator it2 = atList.begin();
                                   it2 != atList.end(); ++it2) {
                  Particle *at = *it2;
                  //Real3D d1 = at.position() - vp.position();
                  //Real3D d1;
                  //verletList->getSystem()->bc->getMinimumImageVectorBox(d1, at.position(), vp.position());
                  //cmp += at.mass() * d1;

                  cmp += at->mass() * at->position();
                  //cmv += at.mass() * at.velocity();
              }
              cmp /= vp.getMass();
              //cmv /= vp.getMass();
              //cmp += vp.position(); // cmp is a relative position
              //std::cout << " cmp M: "  << M << "\n\n";
              //std::cout << "  moving VP to " << cmp << ", velocitiy is " << cmv << "\n";

              // update (overwrite) the position and velocity of the VP
              vp.position() = cmp;
              //vp.velocity() = cmv;

          }
          else { // this should not happen
              std::cout << " VP particle " << vp.id() << "-" << vp.ghost() << " not found in tuples ";
              std::cout << " (" << vp.position() << ")\n";
              exit(1);
              return 0.0;
          }
      }
      
      std::set<Particle*> adrZone = verletList->getAdrZone();
      for (std::set<Particle*>::iterator it=adrZone.begin();
              it != adrZone.end(); ++it) {

          Particle &vp = **it;

          FixedTupleListAdress::iterator it3;
          it3 = fixedtupleList->find(&vp);

          if (it3 != fixedtupleList->end()) {

              std::vector<Particle*> atList;
              atList = it3->second;

              // compute center of mass
              Real3D cmp(0.0, 0.0, 0.0); // center of mass position
              //Real3D cmv(0.0, 0.0, 0.0); // center of mass velocity
              //real M = vp.getMass(); // sum of mass of AT particles
              for (std::vector<Particle*>::iterator it2 = atList.begin();
                                   it2 != atList.end(); ++it2) {
                  Particle &at = **it2;
                  //Real3D d1 = at.position() - vp.position();
                  //Real3D d1;
                  //verletList->getSystem()->bc->getMinimumImageVectorBox(d1, at.position(), vp.position());
                  //cmp += at.mass() * d1;

                  cmp += at.mass() * at.position();
                  //cmv += at.mass() * at.velocity();
              }
              cmp /= vp.getMass();
              //cmv /= vp.getMass();
              //cmp += vp.position(); // cmp is a relative position
              //std::cout << " cmp M: "  << M << "\n\n";
              //std::cout << "  moving VP to " << cmp << ", velocitiy is " << cmv << "\n";

              // update (overwrite) the position and velocity of the VP
              vp.position() = cmp;
              //vp.velocity() = cmv;

          }
          else { // this should not happen
              std::cout << " VP particle " << vp.id() << "-" << vp.ghost() << " not found in tuples ";
              std::cout << " (" << vp.position() << ")\n";
              exit(1);
              return 0.0;
          }
      }*/
      //const bc::BC& bc = *getSystemRef().bc;
      
      // For GM-H-AdResS
      if (kbT_is_calculated) {} else {compute_kbT();}
      computeParticleEnergies(); // TODO JD instead of calcParticleEnergies() ?? 

      real w = 0.0; 
 
      for (PairList::Iterator it(verletList->getPairs());                
           it.isValid(); ++it) {
        /*std::cout << "Should not happen!\n";
        exit(1);
        return 0.0;*/
        Particle &p1 = *it->first;                                       
        Particle &p2 = *it->second;                                      
        int type1 = p1.type();                                           
        int type2 = p2.type();
        const PotentialCG &potential = getPotentialCG(type1, type2);

        Real3D force(0.0, 0.0, 0.0);
        if(potential._computeForce(force, p1, p2)) {
          Real3D dist = p1.position() - p2.position();
          //Real3D dist;
          //bc.getMinimumImageVectorBox(dist, p1.position(), p2.position());
          
          w += dist * force;
        }
      }
      
      //if (KTI == false) {
      //makeWeights();
      //}
      
      for (PairList::Iterator it(verletList->getAdrPairs()); it.isValid(); ++it) {
         real w1, w2;
         // these are the two VP interacting
         Particle &p1 = *it->first;
         Particle &p2 = *it->second;     

         w1 = p1.lambda();         
         w2 = p2.lambda();
         real w12 = (w1 + w2)/2.0;  // H-AdResS
         
         // GM-H-AdResS
         real eAT1 = energyAT[&p1];
         real eAT2 = energyAT[&p2];
         real eCG1 = energyCG[&p1];
         real eCG2 = energyCG[&p2];
         real gAT1 = w1 * exp(-eAT1 / kbT) / (w1 * exp(-eAT1 / kbT) + (1 - w1) * exp(-eCG1 / kbT));
         real gAT2 = w2 * exp(-eAT2 / kbT) / (w2 * exp(-eAT2 / kbT) + (1 - w2) * exp(-eCG2 / kbT));
         real gAT_avg = (gAT1 + gAT2) / 2.0;

         // force between VP particles
         int type1 = p1.type();
         int type2 = p2.type();
         const PotentialCG &potentialCG = getPotentialCG(type1, type2);
         Real3D forcevp(0.0, 0.0, 0.0);
                if (w12 != 1.0) { // calculate VP force if both VP are outside AT region (CG-HY, HY-HY)
                    /*std::cout << "Should not happen!\n";
                    exit(1);
                    return 0.0;*/
                    if (potentialCG._computeForce(forcevp, p1, p2)) {
                          forcevp *= (1.0 - gAT_avg);
                          Real3D dist = p1.position() - p2.position();
                          //Real3D dist;
                          //bc.getMinimumImageVectorBox(dist, p1.position(), p2.position());
                          w += dist * forcevp;  

                    }
                }
       
         // force between AT particles
         if (w12 != 0.0) { // calculate AT force if both VP are outside CG region (HY-HY, HY-AT, AT-AT)
             FixedTupleListAdress::iterator it3;
             FixedTupleListAdress::iterator it4;
             it3 = fixedtupleList->find(&p1);
             it4 = fixedtupleList->find(&p2);

             //std::cout << "Interaction " << p1.id() << " - " << p2.id() << "\n";
             if (it3 != fixedtupleList->end() && it4 != fixedtupleList->end()) {

                 std::vector<Particle*> atList1;
                 std::vector<Particle*> atList2;
                 atList1 = it3->second;
                 atList2 = it4->second;                
                 
                 for (std::vector<Particle*>::iterator itv = atList1.begin();
                         itv != atList1.end(); ++itv) {

                     Particle &p3 = **itv;

                     for (std::vector<Particle*>::iterator itv2 = atList2.begin();
                                          itv2 != atList2.end(); ++itv2) {

                         Particle &p4 = **itv2;

                         // AT forces
                         const PotentialAT &potentialAT = getPotentialAT(p3.type(), p4.type());
                         Real3D forceat(0.0, 0.0, 0.0);
                         if(potentialAT._computeForce(forceat, p3, p4)) {
                         forceat *= gAT_avg;
                         Real3D dist = p3.position() - p4.position();
                         //Real3D dist;
                         //bc.getMinimumImageVectorBox(dist, p3.position(), p3.position());
                         /*if (dist * forceat > 400.0){
                                std::cout << "IDs: id_p3=" << p3.id() << " and id_p4=" << p4.id() << "   #####   ";                             
                                std::cout << "Types: type_p3=" << p3.type() << " and type_p4=" << p4.type() << "   #####   ";   
                                std::cout << "dist: " << sqrt(dist*dist) << "   #####   ";
                                std::cout << "force: " << forceat << "   #####   ";
                                std::cout << "product: " << dist * forceat << "\n";
                                if (sqrt(dist*dist) > 1.0){
                                        std::cout << "DIST TOO BIG!\n";
                                        exit(1);
                                        return 0.0;
                                }
                         }*/
                         
                         w += dist * forceat; 
                         }
                     }
                 }
                 
             }
             else { // this should not happen
                 std::cout << " one of the VP particles not found in tuples: " << p1.id() << "-" <<
                         p1.ghost() << ", " << p2.id() << "-" << p2.ghost();
                 std::cout << " (" << p1.position() << ") (" << p2.position() << ")\n";
                 exit(1);
                 return 0.0;
             }
         }
      }      
      clearParticleEnergies();
      real wsum;
      wsum = 0.0;
      boost::mpi::all_reduce(*mpiWorld, w, wsum, std::plus<real>());
      return wsum;      
      
      /*real w = 0.0;           OLD STUFF
      for (PairList::Iterator it(verletList->getPairs());                
           it.isValid(); ++it) {                                         
        Particle &p1 = *it->first;                                       
        Particle &p2 = *it->second;                                      
        int type1 = p1.type();                                           
        int type2 = p2.type();
        const PotentialCG &potential = getPotentialCG(type1, type2);

        Real3D force(0.0, 0.0, 0.0);
        if(potential._computeForce(force, p1, p2)) {
          Real3D dist = p1.position() - p2.position();
          w = w + dist * force;
        }
      }

      for (PairList::Iterator it(verletList->getAdrPairs());
                 it.isValid(); ++it) {
              Particle &p1 = *it->first;
              Particle &p2 = *it->second;
              int type1 = p1.type();
              int type2 = p2.type();
              const PotentialCG &potential = getPotentialCG(type1, type2);

              Real3D force(0.0, 0.0, 0.0);
              if(potential._computeForce(force, p1, p2)) {
                Real3D dist = p1.position() - p2.position();
                w = w + dist * force;
              }
      }

      real wsum;
      boost::mpi::all_reduce(*mpiWorld, w, wsum, std::plus<real>());
      return wsum;*/
    }

    template < typename _PotentialAT, typename _PotentialCG > inline void
    VerletListGMHadressInteractionTemplate < _PotentialAT, _PotentialCG >::
    computeVirialTensor(Tensor& w) {
      LOG4ESPP_INFO(theLogger, "compute the virial tensor for the Verlet List");

      Tensor wlocal(0.0);
      for (PairList::Iterator it(verletList->getPairs()); it.isValid(); ++it){
        Particle &p1 = *it->first;
        Particle &p2 = *it->second;
        int type1 = p1.type();
        int type2 = p2.type();
        const PotentialCG &potential = getPotentialCG(type1, type2);

        Real3D force(0.0, 0.0, 0.0);
        if(potential._computeForce(force, p1, p2)) {
          Real3D r21 = p1.position() - p2.position();
          wlocal += Tensor(r21, force);
        }
      }

      for (PairList::Iterator it(verletList->getAdrPairs()); it.isValid(); ++it){
              Particle &p1 = *it->first;
              Particle &p2 = *it->second;
              int type1 = p1.type();
              int type2 = p2.type();
              const PotentialCG &potential = getPotentialCG(type1, type2);

              Real3D force(0.0, 0.0, 0.0);
              if(potential._computeForce(force, p1, p2)) {
                Real3D r21 = p1.position() - p2.position();
                wlocal += Tensor(r21, force);
              }
      }

      Tensor wsum(0.0);
      boost::mpi::all_reduce(*mpiWorld, (double*)&wlocal, 6, (double*)&wsum, std::plus<double>());
      w += wsum;
    }
 
    template < typename _PotentialAT, typename _PotentialCG > inline void
    VerletListGMHadressInteractionTemplate < _PotentialAT, _PotentialCG >::
    computeVirialTensor(Tensor& w, real z) {
      LOG4ESPP_INFO(theLogger, "compute the virial tensor for the Verlet List");

      std::cout << "Warning! At the moment IK computeVirialTensor in VerletListHAdress does'n work"<<std::endl;
      
      /*
      Tensor wlocal(0.0);
      for (PairList::Iterator it(verletList->getPairs()); it.isValid(); ++it){
        Particle &p1 = *it->first;
        Particle &p2 = *it->second;
        int type1 = p1.type();
        int type2 = p2.type();
        Real3D p1pos = p1.position();
        Real3D p2pos = p2.position();
        
        if(  (p1pos[0]>xmin && p1pos[0]<xmax && 
              p1pos[1]>ymin && p1pos[1]<ymax && 
              p1pos[2]>zmin && p1pos[2]<zmax) ||
             (p2pos[0]>xmin && p2pos[0]<xmax && 
              p2pos[1]>ymin && p2pos[1]<ymax && 
              p2pos[2]>zmin && p2pos[2]<zmax) ){
          const PotentialCG &potential = getPotentialCG(type1, type2);

          Real3D force(0.0, 0.0, 0.0);
          if(potential._computeForce(force, p1, p2)) {
            Real3D r21 = p1pos - p2pos;
            wlocal += Tensor(r21, force);
          }
        }
      }

      for (PairList::Iterator it(verletList->getAdrPairs()); it.isValid(); ++it){
        Particle &p1 = *it->first;
        Particle &p2 = *it->second;
        int type1 = p1.type();
        int type2 = p2.type();
        Real3D p1pos = p1.position();
        Real3D p2pos = p2.position();
        if(  (p1pos[0]>xmin && p1pos[0]<xmax && 
              p1pos[1]>ymin && p1pos[1]<ymax && 
              p1pos[2]>zmin && p1pos[2]<zmax) ||
             (p2pos[0]>xmin && p2pos[0]<xmax && 
              p2pos[1]>ymin && p2pos[1]<ymax && 
              p2pos[2]>zmin && p2pos[2]<zmax) ){
          const PotentialCG &potential = getPotentialCG(type1, type2);

          Real3D force(0.0, 0.0, 0.0);
          if(potential._computeForce(force, p1, p2)) {
            Real3D r21 = p1pos - p2pos;
            wlocal += Tensor(r21, force);
          }
        }
      }

      Tensor wsum(0.0);
      boost::mpi::all_reduce(*mpiWorld, (double*)&wlocal, 6, (double*)&wsum, std::plus<double>());
      w += wsum;
       */
    }

    
    template < typename _PotentialAT, typename _PotentialCG > inline void
    VerletListGMHadressInteractionTemplate < _PotentialAT, _PotentialCG >::
    computeVirialTensor(Tensor *w, int n) {
      std::cout << "Warning! At the moment IK computeVirialTensor in VerletListHAdress does'n work"<<std::endl;
    }
    
    template < typename _PotentialAT, typename _PotentialCG >
    inline real
    VerletListGMHadressInteractionTemplate< _PotentialAT, _PotentialCG >::
    getMaxCutoff() {
      real cutoff = 0.0;
      for (int i = 0; i < ntypes; i++) {
        for (int j = 0; j < ntypes; j++) {
          cutoff = std::max(cutoff, getPotentialCG(i, j).getCutoff());
        }
      }
      return cutoff;
    }

    // GM-AdResS helper functions
    template < typename _PotentialAT, typename _PotentialCG > void
    VerletListGMHadressInteractionTemplate < _PotentialAT, _PotentialCG >::
    compute_kbT() {
        int myN, systemN;
        real sumT = 0.0;
        real v2sum = 0.0;
        System& system = verletList->getSystemRef();
          
        if (system.storage->getFixedTuples()){  // AdResS - hence, need to distinguish between CG and AT particles.     
            int count = 0;
            shared_ptr<FixedTupleListAdress> fixedtupleList=system.storage->getFixedTuples();
            CellList realCells = system.storage->getRealCells();
            for (iterator::CellListIterator cit(realCells); !cit.isDone(); ++cit) {  // Iterate over all (CG) particles.              
                Particle &vp = *cit;
                FixedTupleListAdress::iterator it2;
                it2 = fixedtupleList->find(&vp);
                
                if (it2 != fixedtupleList->end()) {  // Are there atomistic particles for given CG particle? If yes, use those for calculation.
                    std::vector<Particle*> atList;
                    atList = it2->second;
                    for (std::vector<Particle*>::iterator it3 = atList.begin();
                                         it3 != atList.end(); ++it3) {
                        Particle &at = **it3;
                        Real3D vel = at.velocity();
                        v2sum += at.mass() * (vel * vel);
                        count += 1;
                    }  
                } else {   // If not, use CG particle itself for calculation.
                    Real3D vel = cit->velocity();
                    v2sum += cit->mass() * (vel * vel);
                    count += 1;
                }
            }
          
        myN = count;
        //std::cout << "COUNT " << count << std::endl;
        } else{  // No AdResS - just iterate over all particles          
            CellList realCells = system.storage->getRealCells();
            for (iterator::CellListIterator cit(realCells); !cit.isDone(); ++cit) {
              Real3D vel = cit->velocity();
              v2sum += cit->mass() * (vel * vel);
            }
            
            myN = system.storage->getNRealParticles();
        }
          
        mpi::all_reduce(*getVerletList()->getSystem()->comm, v2sum, sumT, std::plus<real>());
        mpi::all_reduce(*getVerletList()->getSystem()->comm, myN, systemN, std::plus<int>());
          
        //std::cout << "Count in Temperature calculation: " << systemN << std::endl;
        //std::cout << "sumT in Temperature calculation: " << sumT << std::endl;
        kbT = sumT / (3.0 * systemN); //NO CONSTRAINTS, SETTLE BELOW 
        kbT_is_calculated = true;
        //return sumT / (2.0 * systemN);  // SETTLE CONSTAINTS
    }

    template < typename _PotentialAT, typename _PotentialCG > void
    VerletListGMHadressInteractionTemplate < _PotentialAT, _PotentialCG >::
    getPairEnergies(Particle &p1, Particle &p2, real eAT, real eCG) {
        int type1 = p1.type();
        int type2 = p2.type();
        const PotentialCG &potentialCG = getPotentialCG(type1, type2);
        // Calculate eCG by calculating a single pair interaction energy.
        eCG = potentialCG._computeEnergy(p1, p2);
        // Calculate eAT by calculating the pair interaction energies between
        // each pair of atoms in this AdResS particle.
        eAT = 0.0;
        FixedTupleListAdress::iterator it3;
        FixedTupleListAdress::iterator it4;
        it3 = fixedtupleList->find(&p1);
        it4 = fixedtupleList->find(&p2);

        if (it3 != fixedtupleList->end() && it4 != fixedtupleList->end()) {
            std::vector<Particle*> atList1;
            std::vector<Particle*> atList2;
            atList1 = it3->second;
            atList2 = it4->second;
            for (std::vector<Particle*>::iterator itv = atList1.begin();
                    itv != atList1.end(); ++itv) {
                Particle &p3 = **itv;
                for (std::vector<Particle*>::iterator itv2 = atList2.begin();
                                     itv2 != atList2.end(); ++itv2) {
                    Particle &p4 = **itv2;
                    // AT energies
                    const PotentialAT &potentialAT = getPotentialAT(p3.type(), p4.type());
                    eAT += potentialAT._computeEnergy(p3, p4);
                } 
            }                 
        }              
    }

    template < typename _PotentialAT, typename _PotentialCG > void
    VerletListGMHadressInteractionTemplate < _PotentialAT, _PotentialCG >::
    computeParticleEnergies() {
        for (std::set<Particle*>::iterator it=adrZone.begin();
            it != adrZone.end(); ++it) {
            Particle &p = **it;
            energyCG[&p]=0.0;
            energyAT[&p]=0.0;
        }
        // Calculate the energyCG and energyAT arrays for later use
        // Compute forces (AT and VP) of Pairs inside AdResS zone
        for (PairList::Iterator it(verletList->getAdrPairs()); it.isValid(); ++it) {
          real w1, w2;
          // these are the two VP interacting
          Particle &p1 = *it->first;
          Particle &p2 = *it->second;

          w1 = p1.lambda();               
          w2 = p2.lambda();
          real w12 = (w1 + w2)/2.0;  // H-AdResS
         
          // REMOVE FOR IDEAL GAS
          // Energy between VP particles
          int type1 = p1.type();
          int type2 = p2.type();
          const PotentialCG &potentialCG = getPotentialCG(type1, type2);
          Real3D forcevp(0.0, 0.0, 0.0);
          if (w12 != 1.0) {
              // Compute CG energies of particles in the hybrid and store
              if (w12 != 0.0) {   //at least one particle in hybrid region => need to do the energy calculation
                  real energyvp = potentialCG._computeEnergy(p1, p2);
                  if (w1 != 0.0) {   // if particle one is in hybrid region
                      energyCG[&p1] += energyvp;
                  }
                  if (w2 != 0.0) {   // if particle two is in hybrid region
                      energyCG[&p2] += energyvp;
                  }
              }
          }
          // REMOVE FOR IDEAL GAS
        
          // Energy between AT particles
          if (w12 != 0.0) { // calculate AT force if both VP are outside CG region (HY-HY, HY-AT, AT-AT)
              FixedTupleListAdress::iterator it3;
              FixedTupleListAdress::iterator it4;
              it3 = fixedtupleList->find(&p1);
              it4 = fixedtupleList->find(&p2);

              if (it3 != fixedtupleList->end() && it4 != fixedtupleList->end()) {

                  std::vector<Particle*> atList1;
                  std::vector<Particle*> atList2;
                  atList1 = it3->second;
                  atList2 = it4->second;

                  //std::cout << "AT forces ...\n";
                  for (std::vector<Particle*>::iterator itv = atList1.begin();
                          itv != atList1.end(); ++itv) {

                      Particle &p3 = **itv;

                      for (std::vector<Particle*>::iterator itv2 = atList2.begin();
                                            itv2 != atList2.end(); ++itv2) {

                          Particle &p4 = **itv2;

                          // AT energies
                          const PotentialAT &potentialAT = getPotentialAT(p3.type(), p4.type());
                         
                          // Compute AT energies of particles in the hybrid and store
                          if(w12!=1.0){   //at least one particle in hybrid region => need to do the energy calculation
                              real energyat = potentialAT._computeEnergy(p3, p4);   
                              if(w1!=1.0){   // if particle one is in hybrid region
                                     energyAT[&p1] += energyat;
                              }
                              if(w2!=1.0){   // if particle two is in hybrid region
                                     energyAT[&p2] += energyat;
                              }              
                          }                       

                      }

                  }
              } else { // this should not happen
                  std::cout << " one of the VP particles not found in tuples: " << p1.id() << "-" <<
                          p1.ghost() << ", " << p2.id() << "-" << p2.ghost();
                  std::cout << " (" << p1.position() << ") (" << p2.position() << ")\n";
                  exit(1);
                  return;
              }
          }
      }
    }
  }
}
#endif
