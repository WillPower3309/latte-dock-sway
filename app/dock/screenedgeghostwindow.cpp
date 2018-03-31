/*
*  Copyright 2018 Michail Vourlakos <mvourlakos@gmail.com>
*
*  This file is part of Latte-Dock
*
*  Latte-Dock is free software; you can redistribute it and/or
*  modify it under the terms of the GNU General Public License as
*  published by the Free Software Foundation; either version 2 of
*  the License, or (at your option) any later version.
*
*  Latte-Dock is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "screenedgeghostwindow.h"
#include "../dockcorona.h"
#include "dockview.h"

#include <QDebug>
#include <QSurfaceFormat>
#include <QQuickView>

#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>

#include <KWindowSystem>

#include <NETWM>

namespace Latte {

ScreenEdgeGhostWindow::ScreenEdgeGhostWindow(DockView *view) :
    m_dockView(view)
{
    setColor(QColor(Qt::transparent));
    setDefaultAlphaBuffer(true);

    setFlags(Qt::FramelessWindowHint
             | Qt::WindowStaysOnTopHint
             | Qt::NoDropShadowWindowHint
             | Qt::WindowDoesNotAcceptFocus);

    connect(m_dockView, &DockView::absGeometryChanged, this, &ScreenEdgeGhostWindow::updateGeometry);
    connect(m_dockView, &DockView::screenGeometryChanged, this, &ScreenEdgeGhostWindow::updateGeometry);
    connect(m_dockView, &QQuickView::screenChanged, this, [this]() {
        setScreen(m_dockView->screen());
    });


    if (!KWindowSystem::isPlatformWayland()) {
        connect(this, &QWindow::visibleChanged, this, [&]() {
            //! IMPORTANT!!! ::: This fixes a bug when closing an Activity all docks from all Activities are
            //!  disappearing! With this they reappear!!!
            if (!isVisible()) {
                QTimer::singleShot(100, [this]() {
                    if (!m_inDelete && !isVisible()) {
                        setVisible(true);
                    }
                });

                QTimer::singleShot(1500, [this]() {
                    if (!m_inDelete && !isVisible()) {
                        setVisible(true);
                    }
                });
            } else {
                //! For some reason when the window is hidden in the edge under X11 afterwards
                //! is losing its window flags
                if (!m_inDelete) {
                    KWindowSystem::setType(winId(), NET::Dock);
                    KWindowSystem::setState(winId(), NET::SkipTaskbar | NET::SkipPager);
                    KWindowSystem::setOnAllDesktops(winId(), true);
                }
            }
        });
    }

    setupWaylandIntegration();

    setScreen(m_dockView->screen());
    setVisible(true);
    updateGeometry();
    hideWithMask();
}

ScreenEdgeGhostWindow::~ScreenEdgeGhostWindow()
{
    m_inDelete = true;

    if (m_shellSurface) {
        delete m_shellSurface;
    }
}

int ScreenEdgeGhostWindow::location()
{
    return (int)m_dockView->location();
}

DockView *ScreenEdgeGhostWindow::parentDock()
{
    return m_dockView;
}

KWayland::Client::PlasmaShellSurface *ScreenEdgeGhostWindow::surface()
{
    return m_shellSurface;
}

void ScreenEdgeGhostWindow::updateGeometry()
{
    QRect newGeometry;
    int thickness{1};

    if (m_dockView->location() == Plasma::Types::BottomEdge) {
        newGeometry.setX(m_dockView->absGeometry().left());
        newGeometry.setY(m_dockView->screenGeometry().bottom() - thickness);
    } else if (m_dockView->location() == Plasma::Types::TopEdge) {
        newGeometry.setX(m_dockView->absGeometry().left());
        newGeometry.setY(m_dockView->screenGeometry().top());
    } else if (m_dockView->location() == Plasma::Types::LeftEdge) {
        newGeometry.setX(m_dockView->screenGeometry().left());
        newGeometry.setY(m_dockView->absGeometry().top());
    } else if (m_dockView->location() == Plasma::Types::RightEdge) {
        newGeometry.setX(m_dockView->screenGeometry().right() - thickness);
        newGeometry.setY(m_dockView->absGeometry().top());
    }

    if (m_dockView->formFactor() == Plasma::Types::Horizontal) {
        newGeometry.setWidth(m_dockView->absGeometry().width());
        newGeometry.setHeight(thickness + 1);
    } else {
        newGeometry.setWidth(thickness + 1);
        newGeometry.setHeight(m_dockView->absGeometry().height());
    }

    setMinimumSize(newGeometry.size());
    setMaximumSize(newGeometry.size());
    setPosition(newGeometry.x(), newGeometry.y());

    if (m_shellSurface) {
        m_shellSurface->setPosition(newGeometry.topLeft());
    }
}

void ScreenEdgeGhostWindow::setupWaylandIntegration()
{
    if (m_shellSurface || !KWindowSystem::isPlatformWayland() || !m_dockView || !m_dockView->containment()) {
        // already setup
        return;
    }

    if (DockCorona *c = qobject_cast<DockCorona *>(m_dockView->containment()->corona())) {
        using namespace KWayland::Client;

        PlasmaShell *interface = c->waylandDockCoronaInterface();

        if (!interface) {
            return;
        }

        Surface *s = Surface::fromWindow(this);

        if (!s) {
            return;
        }

        qDebug() << "wayland screen edge ghost window surface was created...";
        m_shellSurface = interface->createSurface(s, this);
        m_shellSurface->setSkipTaskbar(true);
        m_shellSurface->setPanelTakesFocus(false);
        m_shellSurface->setRole(PlasmaShellSurface::Role::Panel);
        m_shellSurface->setPanelBehavior(PlasmaShellSurface::PanelBehavior::AutoHide);
    }
}

bool ScreenEdgeGhostWindow::event(QEvent *e)
{
    if (e->type() == QEvent::Enter || e->type() == QEvent::DragEnter) {
        emit edgeTriggered();
    }

    QQuickView::event(e);
}

void ScreenEdgeGhostWindow::hideWithMask()
{
    QRect maskGeometry;

    if (m_dockView->formFactor() == Plasma::Types::Horizontal) {
        maskGeometry.setX(m_dockView->absGeometry().x());
        maskGeometry.setY(m_dockView->absGeometry().bottom() + 1);
        maskGeometry.setWidth(m_dockView->absGeometry().width());
        maskGeometry.setHeight(1);
    } else {
        maskGeometry.setX(m_dockView->absGeometry().right() + 1);
        maskGeometry.setY(m_dockView->absGeometry().y());
        maskGeometry.setWidth(1);
        maskGeometry.setHeight(m_dockView->absGeometry().height());
    }

    setMask(maskGeometry);
}

void ScreenEdgeGhostWindow::showWithMask()
{
    setMask(QRect());
}

}
