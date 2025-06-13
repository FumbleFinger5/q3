#include <QApplication>
#include <QTableView>
#include <QSettings>
// This is crucial: undefine the 'signals' macro after Qt headers
// but *before* any GTK/GLib headers that might use 'signals' as a struct member.
#ifdef signals
#undef signals
#endif

#include "pdef.h"
#include "memgive.h"
#include "str.h"
#include "log.h"

#include "myqsettings.h"
#include "window.h"

static int conf_number=0;

void get_conf_number(int argc, char *argv[])
{
int i,c;
for (i=1; i<argc; i++)
   {
   const char *p=argv[i];
   if (SAME2BYTES(p,"-c") && (c=a2i(&p[2],1))!=0 && p[3]==0)
      conf_number=c;
   }
}

#ifdef chatgpt
MyQSettings::MyQSettings(QWidget *parent_window)
{
   wind = parent_window;
   QString configFileName = "SMDB.conf";
   if (conf_number != 0) {
       configFileName = QString("SMDB_%1.conf").arg(conf_number);
   }
   Qs = new QSettings(QCoreApplication::organizationName(), QCoreApplication::applicationName() + "_" + configFileName);
   ((Window*)wind)->proxyView->header()->restoreState(Qs->value("hdr_state").toByteArray());
   ((Window*)wind)->restoreGeometry(Qs->value("geometry").toByteArray());
}

/*MyQSettings::~MyQSettings()
{
Qs->setValue("geometry",((Window*)wind)->saveGeometry());
Qs->setValue("hdr_state",((Window*)wind)->proxyView->header()->saveState());
delete Qs;
}*/

MyQSettings::~MyQSettings() {
   // Create a QSettings instance pointing to the default config file
   QSettings defaultSettings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

   // Save the current settings to the default file
   defaultSettings.setValue("geometry", ((Window*)wind)->saveGeometry());
   defaultSettings.setValue("hdr_state", ((Window*)wind)->proxyView->header()->saveState());

   // Clean up the QSettings instance used during the session
   delete Qs;
}
#endif

MyQSettings::MyQSettings(QWidget *parent_window)
{
    wind = parent_window;

    QString configFileName = QCoreApplication::applicationName(); // Default name

    if (conf_number >= 1 && conf_number <= 9) {
        configFileName += "_" + QString::number(conf_number); // Append _n
    }

    Qs = new QSettings("Softworks", configFileName); // Use the potentially modified name

    // Check if the file exists before attempting to restore, preventing crashes
    if (QFile::exists(Qs->fileName())) {
        ((Window*)wind)->proxyView->header()->restoreState(Qs->value("hdr_state").toByteArray());
        ((Window*)wind)->restoreGeometry(Qs->value("geometry").toByteArray());
    } else {
        // Handle the case where the file doesn't exist. You might want to initialize 
        // the window to some default state or display a message to the user.
        qDebug() << "Config file not found:" << Qs->fileName();
        // Example: Set initial window geometry or header state
        // ((Window*)wind)->setGeometry(QRect(100, 100, 800, 600)); // Example
    }
}

MyQSettings::~MyQSettings()
{
    QSettings saveQs("Softworks", QCoreApplication::applicationName()); // Always save to default

    saveQs.setValue("geometry", ((Window*)wind)->saveGeometry());
    saveQs.setValue("hdr_state", ((Window*)wind)->proxyView->header()->saveState());

    // No need to delete Qs here. QSettings handles its own cleanup.
}

