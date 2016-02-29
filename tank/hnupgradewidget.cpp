#include "hnupgradewidget.h"
#include "ui_hnupgradewidget.h"
#include "qtankdefine.h"
#include <QTimer>
#include "qtankclient.h"
#include "qtankdefine.h"
#include "qtanklinux.h"

class QBackupThread : public QThread
{
public:
    explicit QBackupThread(QObject* parent = 0);

    void setLabel(QLabel* lb)
    {
        label = lb;
    }

    void setProgress(HNProgressBar* wid)
    {
        prog = wid;
    }

    // QThread interface
protected:
    void run();
private:
    HNProgressBar* prog;
    QLabel* label;
};

QBackupThread::QBackupThread(QObject *parent) : QThread(parent)
{

}

void QBackupThread::run()
{
    QMetaObject::invokeMethod(label, "setText", Qt::QueuedConnection, Q_ARG(QString, tr("Progressing...")));
    QMetaObject::invokeMethod(prog, "setValue", Qt::QueuedConnection, Q_ARG(int, 12));
    system("rm -f ./backup/backup.tar.gz");
    QMetaObject::invokeMethod(prog, "setValue", Qt::QueuedConnection, Q_ARG(int, 40));
#ifdef __MIPS_LINUX__
    system("tar czvf ./backup/backup.tar.gz /DWINFile/tank");
#else
    system("tar czvf ./backup/backup.tar.gz tank");
#endif
    QMetaObject::invokeMethod(prog, "setValue", Qt::QueuedConnection, Q_ARG(int, 100));
    QMetaObject::invokeMethod(label, "setText", Qt::QueuedConnection, Q_ARG(QString, tr("Success")));
}


class QRestoreThread : public QThread
{
public:
    explicit QRestoreThread(QObject* parent = 0);

    void setLabel(QLabel* lb)
    {
        label = lb;
    }

    void setProgress(HNProgressBar* wid)
    {
        prog = wid;
    }

    void setTimer(QTimer* t)
    {
        timer = t;
    }

    // QThread interface
protected:
    void run();
private:
    HNProgressBar* prog;
    QLabel* label;
    QTimer* timer;
};


QRestoreThread::QRestoreThread(QObject *parent)
{

}

void QRestoreThread::run()
{
    QMetaObject::invokeMethod(label, "setText", Qt::QueuedConnection, Q_ARG(QString, tr("Progressing...")));
    QMetaObject::invokeMethod(prog, "setValue", Qt::QueuedConnection, Q_ARG(int, 40));
#ifdef __MIPS_LINUX__
    system("tar xzvf ./upgrade/upgrade.tar.gz -C /");
#else
    system("mkdir tmp");
    system("tar xzvf ./upgrade/upgrade.tar.gz -C tmp");
#endif
    QMetaObject::invokeMethod(prog, "setValue", Qt::QueuedConnection, Q_ARG(int, 100));
    QMetaObject::invokeMethod(label, "setText", Qt::QueuedConnection, Q_ARG(QString, tr("Success")));
    QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection, Q_ARG(int, 1000));
}

HNUpgradeWidget::HNUpgradeWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HNUpgradeWidget)
{
    ui->setupUi(this);

    ui->widgetUpgrade->setFixedWidth(240);
    ui->widgetUpgrade->setRange(0, 100);
    ui->widgetUpgrade->setValue(0);
    ui->widgetDownload->setFixedWidth(240);
    ui->widgetDownload->setRange(0, 100);
    ui->widgetDownload->setValue(0);
    ui->widgetBackup->setFixedWidth(240);
    ui->widgetBackup->setRange(0, 100);
    ui->widgetBackup->setValue(0);

    m_cli = HNSingleClient(this);
    connect(m_cli, SIGNAL(signalUpdateProgress(int)), ui->widgetDownload, SLOT(setValue(int)));
    connect(m_cli, SIGNAL(signalDownSucc()), this, SLOT(downOK()));

    connect(this, SIGNAL(sigBackup()), this, SLOT(backup()), Qt::QueuedConnection);
    connect(this, SIGNAL(sigDownload()), this, SLOT(download()), Qt::QueuedConnection);
    connect(this, SIGNAL(sigRestore()), this, SLOT(restore()), Qt::QueuedConnection);

    timer = new QTimer(this);
    timer->setSingleShot(false);
    connect(timer, SIGNAL(timeout()), this, SLOT(restart()));

    ui->lbBack->setFixedWidth(100);
    ui->lbDown->setFixedWidth(100);
    ui->lbUpgrade->setFixedWidth(100);

    QBackupThread* t = new QBackupThread(this);
    t->setLabel(ui->lbBack);
    t->setProgress(ui->widgetBackup);
    disconnect(this, SIGNAL(sigBackup()), this, SLOT(backup()));
    connect(this, SIGNAL(sigBackup()), t, SLOT(start()), Qt::QueuedConnection);
    QRestoreThread* tt = new QRestoreThread(this);
    tt->setLabel(ui->lbUpgrade);
    tt->setProgress(ui->widgetUpgrade);
    tt->setTimer(timer);
    disconnect(this, SIGNAL(sigRestore()), this, SLOT(restore()));
    connect(this, SIGNAL(sigRestore()), tt, SLOT(start()), Qt::QueuedConnection);
}

HNUpgradeWidget::~HNUpgradeWidget()
{
    delete ui;
}

void HNUpgradeWidget::initAll()
{
    ui->lbTip->setText(tr("Please don't close this computer! Upgrading..."));
    emit sigBackup();
    emit sigDownload();
}

void HNUpgradeWidget::download()
{
    //开始下载过程
    ui->lbDown->setText(tr("Progressing..."));
    system("rm -f ./upgrade/upgrade.tar.gz");
    m_cli->sendDownDevFiles("./upgrade", "356", "upgrade.tar.gz");
}

void HNUpgradeWidget::downOK()
{
    disconnect(m_cli, SIGNAL(signalUpdateProgress(int)), ui->widgetDownload, SLOT(setValue(int)));
    disconnect(m_cli, SIGNAL(signalDownSucc()), this, SLOT(downOK()));
    ui->lbDown->setText(tr("Success"));
    emit sigRestore();
}

void HNUpgradeWidget::backup()
{
    //开始备份
    ui->lbBack->setText(tr("Progressing..."));
    ui->widgetBackup->setValue(24);
    system("rm -f ./backup/backup.tar.gz");
#ifdef __MIPS_LINUX__
    system("tar czvf ./backup/backup.tar.gz /DWINFile/tank");
#else
    system("tar czvf ./backup/backup.tar.gz tank");
#endif
    ui->widgetBackup->setValue(100);
    ui->lbBack->setText(tr("Success"));
}

void HNUpgradeWidget::restore()
{
    ui->lbUpgrade->setText(tr("Progressing..."));
    ui->widgetUpgrade->setValue(24);
#ifdef __MIPS_LINUX__
    system("tar xzvf ./upgrade/upgrade.tar.gz -C /");
#else
    system("mkdir tmp");
    system("tar xzvf ./upgrade/upgrade.tar.gz -C tmp");
#endif
    ui->widgetUpgrade->setValue(60);
    system("rm -f ./upgrade/upgrade.tar.gz");
    ui->widgetUpgrade->setValue(100);
    ui->lbUpgrade->setText(tr("Success"));
    timer->start(1000);
}

void HNUpgradeWidget::restart()
{
    static int i = 6;
    if(i == 0) {
        //system("reboot");
        timer->stop();
        return;
    }
    i--;
    ui->lbTip->setText(tr("Upgrade success, Restarting... %1").arg(i));
}
