#define BOOST_TEST_MODULE TestAll
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <set>

#include "bc/PeriodicBC.hpp"
#include "storage/SingleNode.hpp"
#include "../All.hpp"

using namespace espresso;
using namespace espresso::storage;
using namespace espresso::pairs;

struct Fixture {
  static const size_t N = 3;
  static const real size = 1.0;
  
  bc::PeriodicBC::SelfPtr pbc;
  Storage::SelfPtr store;
  All::SelfPtr pairs;

  Fixture() {
    pbc = make_shared< bc::PeriodicBC >(1.0);
    store = make_shared< SingleNode >(pbc);
    store->setIdProperty();
    store->setPositionProperty();
    pairs = make_shared< All >(store);

    createLattice();
  }

  void createLattice() {
    // create a lattice of NxNxN particles
    size_t pid = 0;
    double step = size/(N-1);
    for (size_t i = 0; i < N; i++)
      for (size_t j = 0; j < N; j++)
	for (size_t k = 0; k < N; k++) {
	  ParticleHandle p
	    = store->addParticle(ParticleId(pid));
	  store->getPositionPropertyHandle()[p] = Real3D(i*step, j*step, k*step);
	  pid++;
	}
  }
};

//____________________________________________________________________________//

class MockPairComputer: public Computer {
public:
  typedef std::pair< size_t, size_t > IDPair;

  std::set< IDPair > occupied;

  bool prepareCalled;
  bool finalizeCalled;

  bc::BC *bc;
  IdPropertyHandle id;
  PropertyHandle< Real3D > pos;

  MockPairComputer(): occupied()
  {
    prepareCalled = false;
    finalizeCalled = false;
  }

  void prepare(Storage::SelfPtr storage1, 
               Storage::SelfPtr storage2) {
    prepareCalled = true;
    bc = storage1->getBoundaryConditions().get();
    id = storage1->getIdPropertyHandle();
    pos = storage1->getPositionPropertyHandle();
  }

  void finalize() {
    finalizeCalled = true;
  }
  
  bool apply(const Real3D &dist,
             const ParticleHandle p1,
             const ParticleHandle p2) {
    Real3D pos1 = pos[p1];
    Real3D pos2 = pos[p2];
    Real3D d = bc->getDist(pos1, pos2);
    real diff = (dist - d).sqr();
    BOOST_CHECK_CLOSE(diff, 0.0, 1.e-10f);
    
    size_t id1 = id[p1], id2 = id[p2];
    if (id2 < id1) std::swap(id1, id2);
    IDPair pair(id1, id2);
    BOOST_CHECK_MESSAGE(occupied.find(pair) == occupied.end(),
			"pair doublette: " << id1 << " " << id2);
    
    occupied.insert(pair);
    return true;
  }
};


BOOST_FIXTURE_TEST_CASE(foreachTest, Fixture)
{
  shared_ptr< MockPairComputer > computer = make_shared< MockPairComputer >();
  pairs->foreachPair(*computer);
  size_t np = N*N*N;
  BOOST_CHECK_EQUAL(computer->occupied.size(), (np*(np-1))/2);
  BOOST_CHECK(computer->prepareCalled);
  BOOST_CHECK(computer->finalizeCalled);
}

