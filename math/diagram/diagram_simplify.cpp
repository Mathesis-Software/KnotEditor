#include "diagram.h"

namespace KE { namespace TwoD {

bool Diagram::simplify(int depth) {
  if (depth < 1)
    return 0;

  Vertex *v = base;

  bool changed = false;
	for (bool continueFlag = true; continueFlag; ) {
    continueFlag = false;

    do {
      if (this->vertices().size() == 3)
        break;

      bool removeVertexFlag = true;
      Vertex *t = v;
      for (int i = 1; i < depth; i++)
        t = t->prev();
      for (int i = 0; i < 2 * depth; i++) {
				removeVertexFlag &= t->crossings().empty();
        t = t->next();
      }
      if (removeVertexFlag) {
				continueFlag = true;
				changed = true;
        removeVertex(v->next());
      } else {
        v = v->next();
			}
    } while (v != base);
  }

  return changed;
}

}}
