#include "PythonComputer.hpp"
#include <boost/python.hpp>

using namespace boost::python;
using namespace espresso::particles;

void PythonComputer::prepare(const Storage *storage) {
  particleId = storage-> getIdPropertyHandle();
}

void PythonComputer::operator()(const ParticleHandle pref) {
  get_override("__apply__")(particleId[pref]);
}

void PythonComputer::registerPython() {
  class_<PythonComputer, bases<Computer> >
    ("particles_PythonComputer", init<>());
}
