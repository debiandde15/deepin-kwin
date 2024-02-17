/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2017 Martin Flöser <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "kwin_wayland_test.h"
#include "abstract_client.h"
#include "abstract_output.h"
#include "cursor.h"
#include "keyboard_input.h"
#include "platform.h"
#include "pointer_input.h"
#include "screens.h"
#include "useractions.h"
#include "wayland_server.h"
#include "workspace.h"

#include <DWayland/Client/compositor.h>
#include <DWayland/Client/keyboard.h>
#include <DWayland/Client/pointer.h>
#include <DWayland/Client/seat.h>
#include <DWayland/Client/shm_pool.h>
#include <DWayland/Client/surface.h>
#include <DWayland/Client/touch.h>

#include <linux/input.h>

using namespace KWin;
using namespace KWayland::Client;

static const QString s_socketName = QStringLiteral("wayland_test_kwin_dont_crash_useractions_menu-0");

class TestDontCrashUseractionsMenu : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testShowHideShowUseractionsMenu();
};

void TestDontCrashUseractionsMenu::initTestCase()
{
    qRegisterMetaType<KWin::AbstractClient*>();
    QSignalSpy applicationStartedSpy(kwinApp(), &Application::started);
    QVERIFY(applicationStartedSpy.isValid());
    kwinApp()->platform()->setInitialWindowSize(QSize(1280, 1024));
    QVERIFY(waylandServer()->init(s_socketName));
    QMetaObject::invokeMethod(kwinApp()->platform(), "setVirtualOutputs", Qt::DirectConnection, Q_ARG(int, 2));

    // force style to breeze as that's the one which triggered the crash
    QVERIFY(kwinApp()->setStyle(QStringLiteral("breeze")));

    kwinApp()->start();
    QVERIFY(applicationStartedSpy.wait());
    const auto outputs = kwinApp()->platform()->enabledOutputs();
    QCOMPARE(outputs.count(), 2);
    QCOMPARE(outputs[0]->geometry(), QRect(0, 0, 1280, 1024));
    QCOMPARE(outputs[1]->geometry(), QRect(1280, 0, 1280, 1024));
    Test::initWaylandWorkspace();
}

void TestDontCrashUseractionsMenu::init()
{
    QVERIFY(Test::setupWaylandConnection());

    workspace()->setActiveOutput(QPoint(640, 512));
    KWin::Cursors::self()->mouse()->setPos(QPoint(640, 512));
}

void TestDontCrashUseractionsMenu::cleanup()
{
    Test::destroyWaylandConnection();
}

void TestDontCrashUseractionsMenu::testShowHideShowUseractionsMenu()
{
    // this test creates the condition of BUG 382063
    QScopedPointer<KWayland::Client::Surface> surface1(Test::createSurface());
    QScopedPointer<Test::XdgToplevel> shellSurface1(Test::createXdgToplevelSurface(surface1.data()));
    auto client = Test::renderAndWaitForShown(surface1.data(), QSize(100, 50), Qt::blue);
    QVERIFY(client);

    workspace()->showWindowMenu(QRect(), client);
    auto userActionsMenu = workspace()->userActionsMenu();
    QTRY_VERIFY(userActionsMenu->isShown());
    QVERIFY(userActionsMenu->hasClient());

    kwinApp()->platform()->keyboardKeyPressed(KEY_ESC, 0);
    kwinApp()->platform()->keyboardKeyReleased(KEY_ESC, 1);
    QTRY_VERIFY(!userActionsMenu->isShown());
    QVERIFY(!userActionsMenu->hasClient());

    // and show again, this triggers BUG 382063
    workspace()->showWindowMenu(QRect(), client);
    QTRY_VERIFY(userActionsMenu->isShown());
    QVERIFY(userActionsMenu->hasClient());
}

WAYLANDTEST_MAIN(TestDontCrashUseractionsMenu)
#include "dont_crash_useractions_menu.moc"
