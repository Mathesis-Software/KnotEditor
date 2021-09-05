/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at

 *   http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 * Author: Nikolay Pultsin <geometer@geometer.name>
 */

#include <fstream>

#include <QtCore/QResource>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtGui/QPainter>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProxyStyle>

#include <rapidjson/istreamwrapper.h>

#include "DiagramWindow.h"
#include "FileIconProvider.h"
#include "KnotWindow.h"
#include "KnotEditorApplication.h"
#include "LibraryWindow.h"
#include "StartWindow.h"
#include "../ke/Util_rapidjson.h"

namespace {

class ProxyStyle : public QProxyStyle {

QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap, const QStyleOption *option) const override {
	if (iconMode == QIcon::Disabled) {
		QPixmap copy(pixmap);
		QPainter painter(&copy);
		painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
		painter.fillRect(copy.rect(), 0xc0c0c0);
		painter.end();
		return copy;
	}
	return QProxyStyle::generatedIconPixmap(iconMode, pixmap, option);
}

};

}

namespace KE::Qt {

KnotEditorApplication::KnotEditorApplication(int &argc, char **argv) : QApplication(argc, argv) {
	this->setFont(QFont("Helvetica", 10));
	this->setStyle(new ProxyStyle);

	QObject::connect(this, &QApplication::aboutToQuit, [] { qDebug() << "about to quit"; });

	QPixmap pixmap(":images/trefoil.png");
	pixmap.setDevicePixelRatio(this->devicePixelRatio());
	this->setWindowIcon(pixmap);

	int count = 0;
	for (int i = 1; i < argc; ++i) {
		if (auto window = KnotEditorApplication::openFile(argv[i])) {
			window->raise();
			count += 1;
		}
	}
	if (count == 0) {
		QSettings settings;
		for (const auto &name : settings.value("OpenWindows").toStringList()) {
			if (name == "::LIBRARY::") {
				KnotEditorApplication::library()->raise();
				count += 1;
			} else if (auto window = KnotEditorApplication::openFile(name)) {
				window->raise();
				count += 1;
			}
		}
	}

	if (count == 0) {
		(new StartWindow())->show();
	}
}

void KnotEditorApplication::exitApplication() {
	QStringList ids;
	for (auto widget : QApplication::topLevelWidgets()) {
		if (auto window = dynamic_cast<BaseWindow*>(widget)) {
			if (window->close()) {
				const auto id = window->identifier();
				if (!id.isNull()) {
					ids.append(id);
				}
			} else {
				return;
			}
		}
	}

	QSettings settings;
	settings.setValue("OpenWindows", ids);
	settings.sync();

	qApp->quit();
}

QWidget *KnotEditorApplication::library() {
	for (auto widget : QApplication::topLevelWidgets()) {
		if (auto window = dynamic_cast<LibraryWindow*>(widget)) {
			window->showNormal();
			window->raise();
			return window;
		}
	}

	auto window = new LibraryWindow();
	window->show();
	return window;
}

QWidget *KnotEditorApplication::newDiagram() {
	auto window = new DiagramWindow();
	window->show();
	return window;
}

namespace {

QString getOpenFileNameEx() {
	QSettings settings;
	QString dir = settings.value("CustomFilesFolder").toString();
	if (dir.isEmpty()) {
		dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
	}
	QFileDialog dialog(nullptr, "Open file", dir);
	dialog.setSupportedSchemes(QStringList(QStringLiteral("file")));
	dialog.setIconProvider(FileIconProvider::instance());
	dialog.setNameFilters({
		"Knot Editor files (*.knt *.dgr)",
		"Knot files only (*.knt)",
		"Diagram files only (*.dgr)",
		"Any files (*)"
	});
	if (dialog.exec() == QDialog::Accepted) {
		settings.setValue("CustomFilesFolder", dialog.directory().path());
		settings.sync();
		return dialog.selectedUrls().value(0).toLocalFile();
	}
	return QString();
}

}

QWidget *KnotEditorApplication::openFile() {
	return openFile(getOpenFileNameEx());
}

QWidget *KnotEditorApplication::openFile(const QString &filename) {
	if (filename.isEmpty()) {
		return nullptr;
	}

	try {
		rapidjson::Document doc;

		if (filename.startsWith(":")) {
			QResource resource(filename);
			if (!resource.isValid() || resource.data() == nullptr || resource.size() == 0) {
				throw std::runtime_error("Cannot read the resource content");
			}
			doc.Parse(reinterpret_cast<const char*>(resource.data()), resource.size());
		} else {
			std::ifstream is(filename.toStdString());
			if (!is) {
				throw std::runtime_error("Cannot read the file content");
			}
			rapidjson::IStreamWrapper wrapper(is);
			doc.ParseStream(wrapper);
			is.close();
		}

		Window *window = nullptr;
		if (doc.IsNull()) {
			throw std::runtime_error("The file is not in JSON format");
		} else if (doc.IsObject() && Util::rapidjson::getString(doc, "type") == "diagram") {
			window = new DiagramWindow(doc, filename);
		} else if (doc.IsObject() && Util::rapidjson::getString(doc, "type") == "link") {
			window = new KnotWindow(doc, filename);
		} else {
			throw std::runtime_error("The file does not represent a knot nor a diagram");
		}

		window->show();
		return window;
	} catch (const std::runtime_error &e) {
		QMessageBox::critical(nullptr, "File opening error", QString("\n") + e.what() + "\n");
		return nullptr;
	}
}

}
