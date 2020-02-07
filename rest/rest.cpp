#include "rest-internal.h"

using namespace std;

extern handlersMap jdwrapper; // TODO: nuke

int main() {
  // handlersMap& endpointsRef{jdwrapper};
  restServer s(jdwrapper, "6666"s);
  while (true) { /*TODO: be nicer here*/
  }
  return 0;
}
