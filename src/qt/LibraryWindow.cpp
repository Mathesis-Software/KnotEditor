/*
 * Copyright (c) 1995-2021, Nikolay Pultsin <geometer@geometer.name>
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cmath>

#include <rapidjson/document.h>

#include <QtCore/QDirIterator>
#include <QtCore/QResource>
#include <QtCore/QSettings>
#include <QtGui/QAction>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QVBoxLayout>

#include "Application.h"
#include "LibraryWindow.h"
#include "NetworkManager.h"
#include "../ke/Diagram.h"
#include "../ke/Knot.h"
#include "../ke/Util_rapidjson.h"

namespace KE::Qt {

namespace {

class KnotPreview {

private:
	const ThreeD::Knot &knot;

public:
	KnotPreview(const ThreeD::Knot &knot) : knot(knot) {
	}

	void paint(QPixmap &pixmap) {
		QPainter painter;
		painter.begin(&pixmap);

		double minX = std::numeric_limits<double>::max();
		double minY = std::numeric_limits<double>::max();
		double maxX = std::numeric_limits<double>::min();
		double maxY = std::numeric_limits<double>::min();
		const auto snapshot = this->knot.snapshot();
		for (std::size_t i = 0; i < snapshot.size(); i += 1) {
			const auto &point = snapshot[i];
			minX = std::min(minX, point.x);
			minY = std::min(minY, point.y);
			maxX = std::max(maxX, point.x);
			maxY = std::max(maxY, point.y);
		}

		const double scale = 0.8 * pixmap.width() / std::max(maxX - minX, maxY - minY);
		const double deltaX = (pixmap.width() - scale * (minX + maxX)) / 2;
		const double deltaY = (pixmap.height() - scale * (minY + maxY)) / 2;

		typedef std::pair<ThreeD::Point,ThreeD::Point> Pair;
		std::vector<Pair> edges;
		for (std::size_t i = 0; i < snapshot.size(); i += 1) {
			edges.push_back(std::make_pair(snapshot[i], snapshot[snapshot.next(i)]));
		}
		std::sort(edges.begin(), edges.end(), [](const Pair &p0, const Pair &p1){ return p0.first.z < p1.first.z; });
		for (const auto &pair : edges) {
			QPointF start(pair.first.x * scale + deltaX, pixmap.height() - pair.first.y * scale - deltaY);
			QPointF end(pair.second.x * scale + deltaX, pixmap.height() / pixmap.devicePixelRatio() - pair.second.y * scale - deltaY);
			painter.setPen(QPen(::Qt::white, 24, ::Qt::SolidLine, ::Qt::FlatCap));
			painter.drawLine(start, end);
			painter.setPen(QPen(::Qt::gray, 8, ::Qt::SolidLine, ::Qt::FlatCap));
			painter.drawLine(start, end);
		}

		painter.end();
	}
};

class DiagramPreview {

private:
	const TwoD::Diagram &diagram;

public:
	DiagramPreview(const TwoD::Diagram &diagram) : diagram(diagram) {
	}

	void paint(QPixmap &pixmap) {
		QPainter painter;
		painter.begin(&pixmap);

		float minX = std::numeric_limits<float>::max();
		float minY = std::numeric_limits<float>::max();
		float maxX = std::numeric_limits<float>::min();
		float maxY = std::numeric_limits<float>::min();
		for (const auto &vertex : this->diagram.vertices()) {
			const auto coords = vertex->coords();
			minX = std::min(minX, coords.x);
			minY = std::min(minY, coords.y);
			maxX = std::max(maxX, coords.x);
			maxY = std::max(maxY, coords.y);
		}

		const float scale = 0.8 * pixmap.width() / pixmap.devicePixelRatio() / std::max(maxX - minX, maxY - minY);
		const float deltaX = (pixmap.width() / pixmap.devicePixelRatio() - scale * (minX + maxX)) / 2;
		const float deltaY = (pixmap.width() / pixmap.devicePixelRatio() - scale * (minY + maxY)) / 2;

		painter.setPen(QPen(::Qt::black, 4 * scale));
		painter.setBrush(::Qt::black);

		for (const auto &edge : this->diagram.edges()) {
			drawEdge(painter, edge, scale, deltaX, deltaY);
		}
		const auto radius = 6 * scale;
		for (const auto &vertex : this->diagram.vertices()) {
			const auto coords = vertex->coords();
			const QPointF center(scale * coords.x + deltaX, scale * coords.y + deltaY);
			painter.drawEllipse(center, radius, radius);
		}

		painter.end();
	}

private:
	void drawEdge(QPainter &painter, const TwoD::Diagram::Edge &edge, float scale, float dX, float dY) {
		const auto start = edge.start->coords();
		const auto end = edge.end->coords();
		float deltaX = end.x - start.x;
		float deltaY = end.y - start.y;
		float hyp = hypotf(deltaX, deltaY);

		deltaX = 20 * deltaX / hyp;
		deltaY = 20 * deltaY / hyp;

		float x0 = start.x,
					y0 = start.y,
					x1, y1;

		for (const auto &crs : this->diagram.underCrossings(edge)) {
			auto coords = crs.coords();
			if (!coords) {
				continue;
			}
			x1 = coords->x - deltaX;
			y1 = coords->y - deltaY;

			if ((x1 - x0) * deltaX + (y1 - y0) * deltaY > 0) {
				painter.drawLine(QPointF(x0 * scale + dX, y0 * scale + dY), QPointF(x1 * scale + dX, y1 * scale + dY));
			}

			x0 = coords->x + deltaX;
			y0 = coords->y + deltaY;
		}

		x1 = end.x;
		y1 = end.y;

		if ((x1 - x0) * deltaX + (y1 - y0) * deltaY > 0) {
			painter.drawLine(QPointF(x0 * scale + dX, y0 * scale + dY), QPointF(x1 * scale + dX, y1 * scale + dY));
		}
	}
};

class DataItem : public QListWidgetItem {

public:
	virtual void open() const = 0;

protected:
	void init(const rapidjson::Document &doc) {
		const std::string type = KE::Util::rapidjson::getString(doc, "type");
		std::string name;
		QPixmap pixmap(400, 400);
		pixmap.fill(::Qt::white);
		if (type == "link") {
			const ThreeD::Knot knot(doc);
			this->setText(knot.caption.c_str());
			KnotPreview(knot).paint(pixmap);
		} else if (type == "diagram") {
			const TwoD::Diagram diagram(doc);
			this->setText(diagram.caption.c_str());
			DiagramPreview(diagram).paint(pixmap);
		} else {
			throw std::runtime_error("The data do not represent a knot nor a diagram");
		}

		this->setTextAlignment(::Qt::AlignCenter);
		this->setSizeHint(QSize(120, 145));
		QIcon icon;
		icon.addPixmap(pixmap, QIcon::Normal);
		icon.addPixmap(pixmap, QIcon::Selected);
		this->setIcon(icon);
	}
};

class FileDataItem : public DataItem {

private:
	const QString path;
	const int index;

public:
	FileDataItem(const QString &path, int index) : path(path), index(index) {
		QResource resource(this->path);
		rapidjson::Document doc;
		doc.Parse(reinterpret_cast<const char*>(resource.data()), resource.size());
		this->init(doc);
	}

	bool operator < (const QListWidgetItem &other) const override {
		const FileDataItem &data = dynamic_cast<const FileDataItem&>(other);
		return this->index < data.index || (this->index == data.index && this->path < data.path);
	}

private:
	void open() const override {
		dynamic_cast<Application*>(qApp)->openFile(this->path);
	}
};

class DataDataItem : public DataItem {

private:
	const QByteArray data;

public:
	DataDataItem(const QByteArray &data) : data(data) {
		this->init(this->document());
	}

private:
	rapidjson::Document document() const {
		rapidjson::Document doc;
		doc.Parse(reinterpret_cast<const char*>(this->data.data()), this->data.size());
		return doc;
	}

	void open() const override {
		dynamic_cast<Application*>(qApp)->openDocument(this->document(), QString());
	}
};

class LibraryListWidget : public QListWidget {

public:
	LibraryListWidget() {
		this->setMouseTracking(true);
		this->setViewMode(QListWidget::IconMode);
		this->setResizeMode(QListWidget::Adjust);
		this->setContentsMargins(0, 0, 0, 0);
		this->setIconSize(QSize(100, 100));
		this->setSpacing(5);
		this->setUniformItemSizes(true);
		this->setStyleSheet("QListWidget{background:#d8d8d8;} QListWidget::item{background:white;border:1px solid #c0c0c0;color:#808080;} QListWidget::item::selected{border:2px solid #404040;}");

		QObject::connect(this, &QListWidget::itemEntered, [](QListWidgetItem *item) {
			item->setSelected(true);
		});
		QObject::connect(this, &QListWidget::itemClicked, [](QListWidgetItem *item) {
			dynamic_cast<const DataItem*>(item)->open();
		});
	}

private:
	void leaveEvent(QEvent*) override {
		const auto selected = this->selectedItems();
		for (const auto &item : selected) {
			item->setSelected(false);
		}
	}

	void keyPressEvent(QKeyEvent *event) override {
		if (event->key() != ::Qt::Key_Enter && event->key() != ::Qt::Key_Return) {
			QListWidget::keyPressEvent(event);
			return;
		}

		const auto selected = this->selectedItems();
		if (selected.size() != 1) {
			return;
		}
		dynamic_cast<const DataItem*>(selected[0])->open();
	}
};

}

LibraryWindow::LibraryWindow() {
	this->setCentralWidget(new QWidget);
	auto vlayout = new QVBoxLayout(this->centralWidget());
	vlayout->setSpacing(0);
	vlayout->setContentsMargins(0, 0, 0, 0);
	auto top = new QHBoxLayout;
	top->setContentsMargins(0, 6, 0, 0);
	vlayout->addLayout(top);

	auto tabs = new QTabBar;
	tabs->setDocumentMode(true);
	auto diagrams = createList(".dgr");
	auto knots = createList(".knt");
	auto searchResults = new LibraryListWidget();
	tabs->addTab("Diagrams");
	tabs->addTab("Knots");
	const auto fakeTabIndex = tabs->addTab(QString());
	tabs->setTabVisible(fakeTabIndex, false);
	top->addWidget(tabs);
	top->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
	auto searchLine = new QLineEdit;
	searchLine->setMinimumHeight(24);
	searchLine->setMaximumWidth(200);
	searchLine->setAttribute(::Qt::WA_MacShowFocusRect, 0);
	searchLine->addAction(QIcon(":images/search.svg"), QLineEdit::LeadingPosition);
	auto clearAction = searchLine->addAction(QIcon(":images/clear.svg"), QLineEdit::TrailingPosition);
	QObject::connect(clearAction, &QAction::triggered, [=] {
		searchLine->setText(QString());
		if (tabs->currentIndex() == fakeTabIndex) {
			QSettings settings;
			settings.beginGroup("Window:" + this->identifier());
			const auto index = settings.value("currentTabIndex").toInt();
			tabs->setCurrentIndex(index);
			emit tabs->tabBarClicked(index);
			settings.endGroup();
		}
		clearAction->setVisible(false);
	});
	clearAction->setVisible(false);
	QObject::connect(searchLine, &QLineEdit::textChanged, [=](const QString &text) {
		clearAction->setVisible(tabs->currentIndex() == fakeTabIndex || !text.isEmpty());
	});
	QObject::connect(searchLine, &QLineEdit::returnPressed, [=]() {
		const auto pattern = searchLine->text();
		// TODO: validate input
		if (!pattern.isEmpty()) {
			auto manager = new NetworkManager(this);
			// TODO: show some waiting indicator
			manager->searchDiagram(pattern, [searchResults] (const QByteArray &data) {
				searchResults->clear();
				try {
					searchResults->addItem(new DataDataItem(data));
				} catch (const std::runtime_error &e) {
					// TODO: show error message
				}
			});
			tabs->setCurrentIndex(fakeTabIndex);
			diagrams->setVisible(false);
			knots->setVisible(false);
			searchResults->setVisible(true);
		}
	});
	top->addWidget(searchLine);
	top->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Minimum));
	vlayout->addWidget(diagrams);
	vlayout->addWidget(knots);
	vlayout->addWidget(searchResults);

	{
		QSettings settings;
		settings.beginGroup("Window:" + this->identifier());
		tabs->setCurrentIndex(settings.value("currentTabIndex").toInt());
		settings.endGroup();
	}
	QObject::connect(tabs, &QTabBar::tabBarClicked, [=](int index) {
		diagrams->setVisible(index == 0);
		knots->setVisible(index == 1);
		if (index == 0) {
			diagrams->setFocus();
		} else if (index == 1) {
			knots->setFocus();
		}
		searchResults->setVisible(false);
		QSettings settings;
		settings.beginGroup("Window:" + this->identifier());
		settings.setValue("currentTabIndex", index);
		settings.endGroup();
		settings.sync();
	});
	emit tabs->tabBarClicked(tabs->currentIndex());

	setWindowTitle("Knot Library");
	this->resize(780, 500);

	this->createFileMenu();
	this->restoreParameters();
}

QWidget *LibraryWindow::createList(const QString &suffix) {
	auto list = new LibraryListWidget();
	for (QDirIterator iter(":data", QDirIterator::Subdirectories); iter.hasNext(); ) {
		const auto fileName = iter.next();
		if (!fileName.endsWith(suffix)) {
			continue;
		}

		QRegularExpression re("^(\\d+)_(\\d+)\\....$");
		QRegularExpressionMatch match = re.match(iter.fileName());
		int index = std::numeric_limits<int>::max();
		if (match.hasMatch()) {
			index = 1000 * match.captured(1).toInt() + 1 * match.captured(2).toInt();
		}
		list->addItem(new FileDataItem(iter.filePath(), index));
	}
	list->sortItems();
	return list;
}

}
