#include <QtWidgets/qmenubar.h>

#include "knotWindow.h"

void knotWindow::initMenu (void)
{
  mathMenu = menuBar () -> addMenu ("&Math");
  mathMenu -> addAction ( "&View parameters", this, SLOT (math()) );
  mathMenu -> addSeparator ();
  math_decreaseEnergy = mathMenu -> addAction ( "Decrease &energy...", this, SLOT (decreaseEnergy()) );
  math_decreaseEnergy -> setEnabled (!smoothing);
  math_stop = mathMenu -> addAction ( "&Stop", this, SLOT (stop()) );
  math_stop -> setEnabled (smoothing);
  mathMenu -> addSeparator ();
  mathMenu -> addAction ( "Number of &points...", this, SLOT (setNum()) );
  mathMenu -> addAction ( "&Length...", this, SLOT (setLength()) );
  
  viewMenu = menuBar () -> addMenu ("&View");
  view_showKnot = viewMenu -> addAction ( "Show &knot", this, SLOT (switchShowKnot()) );
  view_showKnot -> setChecked (kSurf -> isVisible ());  
  view_showSeifertSurface = viewMenu -> addAction ( "Show &Seifert surface", this, SLOT (switchShowSeifert()) );
  view_showSeifertSurface -> setChecked (sSurf -> isVisible ());  
  //viewMenu -> setCheckable (true);
  
  QMenu *optionsMenu = menuBar () -> addMenu ("&Options");
  optionsMenu -> addAction ( "&Thickness...", this, SLOT (setThickness()) );
  optionsMenu -> addSeparator ();
  optionsMenu -> addAction ( "Back&ground color...", this, SLOT (setBgColor()) );
  optionsMenu -> addAction ( "&Knot color...", this, SLOT (setKnotColor()) );
  optionsMenu -> addAction ( "Seifert surface &front color...", this, SLOT (setSeifertFrontColor()) );
  optionsMenu -> addAction ( "Seifert surface &back color...", this, SLOT (setSeifertBackColor()) );

  addToolBarSeparator ();
  addToolBarButton ("start.xpm", "Start smoothing", SLOT (smooth()));
  addToolBarButton ("stop.xpm", "Interrupt smoothing", SLOT (stop()));
  addToolBarSeparator ();
  addToolBarButton ("math.xpm", "Show parameters", SLOT (math()));
  addToolBarSeparator ();
  addToolBarButton ("plus.xpm", "Shift Seifert surface along gradient",
             SLOT (bp_plus()));
  addToolBarButton ("minus.xpm", "Shift Seifert surface along gradient",
             SLOT (bp_minus()));
}
