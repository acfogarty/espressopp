// ESPP_CLASS
#ifndef _INTERACTION_VSPHERESELF_HPP
#define _INTERACTION_VSPHERESELF_HPP

#include "Potential.hpp"
#include <cmath>

#ifndef M_PIl
#define M_PIl 3.1415926535897932384626433832795029L
#endif

namespace espresso {
  namespace interaction {

  #define M_4PI (4.0*(M_PIl))
  #define M_4PI3 (M_4PI / 3.0)

    /** This class provides methods to compute forces and energies of
        the VSphereSelf potential.

    */
    class VSphereSelf : public PotentialTemplate< VSphereSelf > {
    private:
      real e1;
      real a1, a16, a16NbNbNb;
      real a2, a22, a2dNb, a22dNb;
      real mth, mfh;
      int Nb, NbNbNb;

    public:
      static void registerPython();

      VSphereSelf()
        : e1(0.0), a1(0.0), a2(0.0), Nb(0) {
        setShift(0.0);
        setCutoff(infinity);
        preset();
      }

      VSphereSelf(real _e1, real _a1, real _a2, int _Nb,
		   real _cutoff, real _shift) 
        : e1(_e1), a1(_a1), a2(_a2), Nb(_Nb) {
        setShift(_shift);
        setCutoff(_cutoff);
        preset();
      }

      VSphereSelf(real _e1, real _a1, real _a2, int _Nb,
		   real _cutoff)
        : e1(_e1), a1(_a1), a2(_a2), Nb(_Nb) {
        autoShift = false;
        setCutoff(_cutoff);
        setAutoShift();
        preset();
      }

      void preset() {
    	mth = - (3.0/2.0);
    	mfh = - (5.0/2.0);
    	a16 = 6*a1;
    	a22 = 2*a2;
    	a22dNb = a22 / Nb;
    	NbNbNb = Nb*Nb*Nb;
    	a16NbNbNb = a16 * NbNbNb;
    	a2dNb = a2 / Nb;
      }

      // Setter and getter
      void sete1(real _e1) {
        e1 = _e1;
        updateAutoShift();
        preset();
      }
      real gete1() const { return e1; }

      void seta1(real _a1) {
        a1 = _a1;
        updateAutoShift();
        preset();
      }
      real geta1() const { return a1; }

      void seta2(real _a2) {
        a2 = _a2;
        updateAutoShift();
        preset();
      }
      real geta2() const { return a2; }

      void setNb(int _Nb) {
        Nb = _Nb;
        updateAutoShift();
        preset();
      }
      real getNb() const { return Nb; }

      real _computeEnergySqrRaw(real distSqr) const {
    	real sigma2 = distSqr;
        real energy;
        energy  =   e1*pow(1.0L*M_4PI3*sigma2, 1.0L*mth)       \
                  + a1*NbNbNb/(sigma2*sigma2*sigma2) \
                  + a22dNb*sigma2;
        return energy;
      }

      bool _computeForceRaw(Real3D& force,
			    const Real3D& dist,
			    real distSqr) const {

    	real sigma  = dist[0];
    	real sigma2 = distSqr;
        force[1] = force[2] = 0.0;
        force[0] =   M_4PI*e1*pow(1.0L*M_4PI3*sigma2, 1.0L*mfh)*sigma \
        		   + a16NbNbNb/(sigma2*sigma2*sigma2*sigma) \
        		   - a22dNb*sigma;
        return true;
      }
    };
  }
}

#endif