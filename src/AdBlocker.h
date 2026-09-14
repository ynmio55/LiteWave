#pragma once
#include <QWebEngineUrlRequestInterceptor>

class AdBlocker final : public QWebEngineUrlRequestInterceptor
{
public:
    explicit AdBlocker(QObject *parent = nullptr);
    void interceptRequest(QWebEngineUrlRequestInfo &info) override;
};
