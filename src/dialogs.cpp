#include "dialogs.h"

#include "config.h"

#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QToolTip>
#include <QVBoxLayout>

void DismissibleDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Return
        || event->key() == Qt::Key_Enter) {
        accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

void applyCloseButtonWidth(QPushButton *button, const Config &config)
{
    button->setMinimumWidth(static_cast<int>(
        button->sizeHint().width()
        * config.number(QStringLiteral("settings_close_button_width_multiplier"))));
}

void showAboutDialog(QWidget *parent)
{
    const Config &config = Config::instance();

    DismissibleDialog dialog(parent);
    dialog.setWindowTitle(config.string(QStringLiteral("about_dialog_title")));
    auto *layout = new QVBoxLayout(&dialog);

    auto *icon = new QLabel(&dialog);
    const int iconSize = static_cast<int>(config.number(QStringLiteral("about_icon_size")));
    icon->setPixmap(QPixmap(QStringLiteral(":/img/Icon.png"))
                        .scaled(iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    auto *name = new QLabel(config.appName(), &dialog);
    QFont nameFont = name->font();
    double aboutNameMultiplier = config.number(QStringLiteral("about_name_font_multiplier"));
#ifdef Q_OS_ANDROID
    aboutNameMultiplier *= 3.0;
#endif
    nameFont.setPointSizeF(nameFont.pointSizeF() * aboutNameMultiplier);
    nameFont.setBold(true);
    name->setFont(nameFont);

    auto *version = new QLabel(QStringLiteral("Version ") + config.appVersion(), &dialog);

    name->setAlignment(Qt::AlignLeft);
    version->setAlignment(Qt::AlignLeft);
    icon->setAlignment(Qt::AlignHCenter);
    auto *headerText = new QVBoxLayout();
    headerText->addWidget(name);
    headerText->addWidget(version);
    auto *header = new QHBoxLayout();
    header->addWidget(icon, 0, Qt::AlignVCenter | Qt::AlignLeft);
    header->addLayout(headerText, 1);
    layout->addLayout(header);

    // Render the author handle inside the copyright line as a link to the
    // project's author page.
    QString copyright = config.copyright().toHtmlEscaped();
    QString handle = config.authorHandle();
    if (handle.startsWith(QLatin1Char('@'))) {
        handle.remove(0, 1);
    }
    if (!handle.isEmpty()) {
        const int handleIndex = copyright.indexOf(handle.toHtmlEscaped());
        if (handleIndex >= 0) {
            const QString anchor = QStringLiteral("<a href=\"%1\">%2</a>")
                                       .arg(config.authorUrl().toHtmlEscaped(),
                                            handle.toHtmlEscaped());
            copyright.replace(handleIndex, handle.toHtmlEscaped().size(), anchor);
        }
    }
    auto *copyrightLabel = new QLabel(copyright, &dialog);
    copyrightLabel->setOpenExternalLinks(true);
    copyrightLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    copyrightLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(copyrightLabel);
    auto *link = new QLabel(
        QStringLiteral("Uses Qt, licensed under "
                       "<a href=\"https://www.qt.io/licensing/open-source-lgpl-obligations\">LGPLv3</a>"),
        &dialog);
    link->setOpenExternalLinks(true);
    link->setTextInteractionFlags(Qt::TextBrowserInteraction);
    link->setAlignment(Qt::AlignCenter);
    layout->addWidget(link, 0, Qt::AlignHCenter);
    layout->addSpacing(QFontMetrics(dialog.font()).height());

    auto *donationTitle = new QLabel(QStringLiteral("Please Donate!"), &dialog);
    QFont donationTitleFont = donationTitle->font();
    donationTitleFont.setBold(true);
    donationTitle->setFont(donationTitleFont);
    layout->addWidget(donationTitle);
    auto *donationDescription = new QLabel(
        QStringLiteral("We truly appreciate and thank you for any donation you can give to our project."),
        &dialog);
    donationDescription->setWordWrap(true);
    layout->addWidget(donationDescription);
    auto *donationImage = new QLabel(&dialog);
    const int donationSize = static_cast<int>(config.number(QStringLiteral("donation_image_size")));
    donationImage->setPixmap(QPixmap(QStringLiteral(":/assets/ethereum-donation.png"))
                                 .scaled(donationSize, donationSize, Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation));
    donationImage->setAlignment(Qt::AlignCenter);
    layout->addWidget(donationImage);
    const QString donationAddress = QStringLiteral("0x6E6465394D14975956cd1BD37ab4E35F2C60300E");
    auto *donationAddressLabel = new QLabel(
        QStringLiteral("Ethereum %1").arg(donationAddress), &dialog);
    layout->addWidget(donationAddressLabel);
    auto *copyDonationButton = new QPushButton(QStringLiteral("Copy Ethereum address"), &dialog);
    copyDonationButton->setFixedWidth(qRound(donationSize * 1.05));
    QObject::connect(copyDonationButton, &QPushButton::clicked, &dialog,
                     [&dialog, donationAddress]() {
                         QApplication::clipboard()->setText(donationAddress);
                         QToolTip::showText(QCursor::pos(), QStringLiteral("Address copied"), &dialog);
                     });

    auto *closeButton = new QPushButton(config.string(QStringLiteral("close")), &dialog);
    applyCloseButtonWidth(closeButton, config);
    QObject::connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addSpacing(QFontMetrics(dialog.font()).height());
    auto *bottomRow = new QHBoxLayout();
    bottomRow->addStretch(1);
    bottomRow->addWidget(copyDonationButton);
    bottomRow->addStretch(1);
    bottomRow->addWidget(closeButton);
    layout->addLayout(bottomRow);

    dialog.exec();
}
