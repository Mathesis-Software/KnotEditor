#include "diagramWindow.h"

void diagramWindow::simplify() {
  if (!this->diagram.isClosed())
    return;

  if (this->diagram.simplify(2)) {
    isSaved = false;
    centralWidget()->repaint();
  }
}
