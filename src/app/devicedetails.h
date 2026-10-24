// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QPointer>
#include <QVariantMap>

class DeviceModel;

// One device of the model for the device page, kept current: the roles of
// its row by role name, for as long as the row exists.
class DeviceDetails : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool present READ present NOTIFY changed)
    Q_PROPERTY(QVariantMap device READ device NOTIFY changed)

public:
    DeviceDetails(DeviceModel *model, const QString &key, QObject *parent = nullptr);

    bool present() const;
    QVariantMap device() const;

Q_SIGNALS:
    void changed();

private:
    QPointer<DeviceModel> m_model;
    QString m_key;
};
