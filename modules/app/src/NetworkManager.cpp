/*
 * Copyright (c) 1995-2021, Mathesis Software <mad@mathesis.fun>
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

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkReply>

#include "NetworkManager.h"

namespace KE::Qt {

void NetworkManager::searchDiagram(const QString &code, int page, QObject *context, const std::function<void(int errorCode, const QByteArray &response)> callback) {
	QUrl url("https://knots.mathesis.fun/api/diagram");
	QNetworkRequest request;
	request.setUrl(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	QJsonObject data;
	data["code"] = code;
	data["debug"] = false;
	data["page"] = page;
	auto reply = this->post(request, QJsonDocument(data).toJson());
	QObject::connect(reply, &QNetworkReply::finished, context, [callback, reply]() {
		callback(reply->error(), reply->readAll());
		reply->deleteLater();
	});
}

}
