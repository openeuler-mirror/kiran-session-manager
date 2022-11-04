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

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QTranslator>
#include "config.h"
#include "lib/base/base.h"
#include "src/ui/exit-query-window.h"

using namespace Kiran;

#define DEFAULT_STYLE_FILE ":/styles/kiran-session-window.qss"

int main(int argc, char *argv[])
{
    auto argv0 = QFileInfo(argv[0]);
    auto programName = argv0.baseName();

    if (klog_qt5_init(QString(), "kylinsec-session", PROJECT_NAME, programName) < 0)
    {
        fprintf(stderr, "Failed to init kiran-log.");
    }

    QApplication app(argc, argv);
    app.setApplicationName(programName);
    app.setApplicationVersion(PROJECT_VERSION);

    QTranslator translator;

    if (!translator.load(QLocale(), qAppName(), ".", KSM_INSTALL_TRANSLATIONDIR, ".qm"))
    {
        KLOG_WARNING() << "Load translator failed!";
    }
    else
    {
        app.installTranslator(&translator);
    }

    ///加载样式表
    QFile file(DEFAULT_STYLE_FILE);
    if (file.open(QIODevice::ReadOnly))
    {
        app.setStyleSheet(file.readAll());
    }
    else
    {
        KLOG_WARNING("Failed to load style sheet.");
    }

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption(QStringList({"p", "power-action"}),
                                        app.translate("main", "Power action."),
                                        app.translate("main", "power action"),
                                        QString("%1").arg(POWER_ACTION_NONE)));
    parser.process(app);

    auto exitQueryWindow = new ExitQueryWindow(parser.value("power-action").toInt());
    exitQueryWindow->show();
    return app.exec();
}
