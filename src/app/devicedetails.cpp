// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "devicedetails.h"
#include "devicemodel.h"

DeviceDetails::DeviceDetails(DeviceModel *model, const QString &key, QObject *parent)
    : QObject(parent)
    , m_model(model)
    , m_key(key)
{
    // A handful of rows: recomputing on every change is cheaper than
    // tracking which row moved.
    connect(model, &QAbstractItemModel::dataChanged, this, &DeviceDetails::changed);
    connect(model, &QAbstractItemModel::rowsInserted, this, &DeviceDetails::changed);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &DeviceDetails::changed);
    connect(model, &QAbstractItemModel::modelReset, this, &DeviceDetails::changed);
}

bool DeviceDetails::present() const
{
    return m_model && m_model->rowOf(m_key) >= 0;
}

QVariantMap DeviceDetails::device() const
{
    QVariantMap values;
    if (!m_model) {
        return values;
    }
    const int row = m_model->rowOf(m_key);
    if (row < 0) {
        return values;
    }
    const QModelIndex index = m_model->index(row);
    const auto roles = m_model->roleNames();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
        values.insert(QString::fromLatin1(it.value()), index.data(it.key()));
    }
    return values;
}
