#include "interaction/Interaction.hpp"

namespace espresso {
  namespace interaction {
    class LennardJones: Interaction {
    private:
      real sigma;
      real epsilon;
      real cutoff;
      real cutoffSqr;
    public:
      LennardJones() {}
      virtual ~LennardJones() {}
      virtual real computeEnergy (real distSqr,
				  const ParticleRef p1,
				  const ParticleRef p2) const 
      {
	real frac2;
	real frac6;
	
	if(dist2 < rc2) {
	  frac2 = 1.0 / dist2;
	  frac6 = frac2 * frac2 * frac2;
	  return 4.0 * 1.0 * (frac6*frac6 - frac6);
	} else {
	  return 0.0;
	}
      }

      virtual Real3D computeForce (real dist2) const;

      virtual real getCutoff() const { return cutoff; }
      virtual real getCutoffSqr() const { return cutoff2; }
      virtual void setCutoff(real _cutoff) { 
	cutoff = _cutoff; 
	cutoffSqr = cutoff*cutoff;
      }

      virtual setEpsilon(real _epsilon) { epsilon = _epsilon; }
      virtual setSigma(real _sigma) { sigma = _sigma; }
    };
  }
}
