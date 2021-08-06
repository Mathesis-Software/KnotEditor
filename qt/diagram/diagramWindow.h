#ifndef __DIAGRAMWINDOW_H__
#define __DIAGRAMWINDOW_H__

#include <QtWidgets/QMenu>
#include <QtWidgets/QToolButton>

#include "../abstractWindow/abstractWindow.h"
#include "../../math/diagram/diagram.h"

class diagramWindow : public abstractWindow {

Q_OBJECT

private:
	enum editorMode {
		DRAW_NEW_DIAGRAM = 0,
		ADD_POINT = 1,
		MOVE_POINT = 2,
		REMOVE_POINT = 3,
		CHANGE_CROSS = 4,
		MOVE_DIAGRAM = 5,
		editorModeNumber = 6
	};

public:
	KE::TwoD::Diagram diagram;

private:
	QMenu *actionsMenu;
	QAction *actions_convert;
	QAction *actions_simplify;
	QAction *actions_clear;

	QToolButton **actions;

	editorMode mode;

	void init();

	const char *mask() {return "*.dgr";};

	void printIt(QPrinter*);
	void saveIt(std::ostream&);
	
private slots:
	void convert();
	void clear();
	void setmode(int);
	void simplify();

public:
	diagramWindow(const rapidjson::Document &doc);
	diagramWindow();
	~diagramWindow();

	bool isEmpty() { return this->diagram.vertices().empty(); }

	friend class DiagramWidget;
};

class DiagramWidget : public QWidget {

	Q_OBJECT

private:
	diagramWindow *Parent;

	void drawVertex(QPainter&, const KE::TwoD::Diagram::Vertex &vertex);
	void drawEdge(QPainter&, const KE::TwoD::Diagram::Edge &edge);

	void paintEvent(QPaintEvent*);
	void mousePressEvent(QMouseEvent*);
	void mouseReleaseEvent(QMouseEvent*);
	void mouseMoveEvent(QMouseEvent*);

public:
	DiagramWidget(diagramWindow *p);

	void drawIt(QPainter &painter);
};

#endif /* __DIAGRAMWINDOW_H__ */
