#pragma once
#include <QWebEngineUrlRequestInterceptor>

class AdBlocker final : public QWebEngineUrlRequestInterceptor
{
public:
    explicit AdBlocker(QObject *parent = nullptr);
    void setEnabled(bool enabled);
    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

private:
    bool enabled_ = true;
};
