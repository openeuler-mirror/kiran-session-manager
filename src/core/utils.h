/**
 * Copyright (c) 2020 ~ 2021 KylinSec Co., Ltd.
 * kiran-session-manager is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 *
 * Author:     tangjie02 <tangjie02@kylinos.com.cn>
 */

#pragma once

#include <QSharedPointer>

namespace Kiran
{
class Utils
{
public:
    Utils();
    virtual ~Utils(){};

    static QSharedPointer<Utils> getDefault();

    // 生成随机的startup id
    QString generateStartupID();
    // 设置自定义的自启动目录，覆盖默认加载目录
    void setAutostartDirs(const QString &autostartDirs);
    // 获取自动启动程序的目录列表
    QStringList getAutostartDirs();
    // 设置环境变量
    void setEnv(const QString &name, const QString &value);
    void setEnvs(const QMap<QString, QString> &envs);
    // 生成cookie
    uint32_t generateCookie();

private:
    static QSharedPointer<Utils> m_instance;

    QStringList m_customAutostartDirs;
    bool m_useCustomAutostartDirs;
};
}  // namespace Kiran