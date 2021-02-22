#include <QMenu>
#include <QToolButton>

#include "partcolortoolbar.h"
#include "icons.h"
#include "gconfig.h"
#include "config.h"
#include "utils.h"

namespace MusEGui {

PartColorToolbar::PartColorToolbar(QWidget *parent)
    : QToolBar(parent)
{
    setObjectName("Part color toolbar");

    buttonAction = addAction(tr("Part color"));
    if (MusEGlobal::config.useTrackColorForParts)
        buttonAction->setIcon(*tracktypeSVGIcon);
    else
        buttonAction->setIcon(MusECore::colorRect(MusEGlobal::config.partColors[0], 80, 80));

    buttonAction->setData(0);

    colorPopup = new QMenu(this);
    buildMenu();

    buttonAction->setMenu(colorPopup);

    QToolButton *tb = dynamic_cast<QToolButton*>(widgetForAction(buttonAction));
    if (tb)
        tb->setPopupMode(QToolButton::MenuButtonPopup);

    buttonAction->setStatusTip(tr("Select current part color from drop-down menu. Click button to set color to selected parts."));

    connect(colorPopup, &QMenu::triggered, this, &PartColorToolbar::popupActionTriggered);
    connect(buttonAction, &QAction::triggered, [this](){ emit partColorTriggered(buttonAction->data().toInt()); });
}

void PartColorToolbar::popupActionTriggered(QAction *a)
{
    buttonAction->setData(a->data());

    int idx = a->data().toInt();
    if (idx == 0 && MusEGlobal::config.useTrackColorForParts)
        buttonAction->setIcon(*tracktypeSVGIcon);
    else
        buttonAction->setIcon(MusECore::colorRect(MusEGlobal::config.partColors[a->data().toInt()], 80, 80));
}

void PartColorToolbar::buildMenu()
{
    colorPopup->clear();

    for (int i = 0; i < NUM_PARTCOLORS; ++i) {
        QAction *act_color = nullptr;
        if (i == 0 && MusEGlobal::config.useTrackColorForParts)
            act_color = colorPopup->addAction(*tracktypeSVGIcon, tr("Track Color"));
        else
            act_color = colorPopup->addAction(MusECore::colorRect(MusEGlobal::config.partColors[i], 80, 80), MusEGlobal::config.partColorNames[i]);
        act_color->setData(i);

        if (i == 0)
            colorPopup->addSeparator();
    }
}

void PartColorToolbar::configChanged() {
    buildMenu();

    int idx = buttonAction->data().toInt();
    if (idx == 0 && MusEGlobal::config.useTrackColorForParts)
        buttonAction->setIcon(*tracktypeSVGIcon);
    else
        buttonAction->setIcon(MusECore::colorRect(MusEGlobal::config.partColors[idx], 80, 80));
}


} // namespace
