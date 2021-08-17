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

#include <QtWidgets/QAction>
#include <QtWidgets/QStatusBar>

#include "knotWindow_math.h"
#include "KnotWidget.h"

void knotWindow::smooth() {
  if (!this->smoothingThread.isRunning()) {
    this->startSmooth(0, 20);
	}
}

void knotWindow::stop() {
  if (this->smoothingThread.isRunning()) {
		this->smoothingThread.requestInterruption();
    statusBar()->showMessage("Smoothing complete", 3000);
		this->updateActions();
  }
}

void knotWindow::doSmooth() {
  if (!continuousSmoothing) {
    if (redrawAfter > smoothSteps)
      redrawAfter = smoothSteps;
    smoothSteps -= redrawAfter;
    if (!smoothSteps)
      stop();
  }

	for (int i = 0; i < redrawAfter; ++i) {
		this->knot.decreaseEnergy();
	}
}

void knotWindow::startSmooth(int st, int ra, bool cont) {
  smoothSteps = st;
  redrawAfter = ra;
  continuousSmoothing = cont;

	this->smoothingThread.start();
  statusBar()->showMessage("Smoothing…");
	this->updateActions();
}

SmoothingThread::SmoothingThread(knotWindow &window) : window(window) {
	connect(this, &SmoothingThread::knotChanged, [&window] { window.knotWidget()->onKnotChanged(); });
	connect(this, &SmoothingThread::finished, &window, &knotWindow::updateActions);
}

void SmoothingThread::run() {
	this->setPriority(LowPriority);
	while (true) {
		if (this->isInterruptionRequested()) {
			this->quit();
			break;
		}
		this->msleep(20);
		this->window.doSmooth();
		emit knotChanged();
	}
}
