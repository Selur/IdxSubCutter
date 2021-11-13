#include <QtCore>
#include <QCoreApplication>
#include <iostream>
#include <QDir>
#include <QFile>
#include <QStringList>
#include "Cutter.h"

int main(int argc, char *argv[])
{
  if (argc != 4) {
    std::cerr << "usage: IdxSubCutter \"Path to the .idx file\" \"cutlist\" \"output folder\""
        << std::endl;
    return -1;
  }
  QString idx = QString(argv[1]);
  if (!idx.endsWith(".idx", Qt::CaseInsensitive)) {
    std::cerr << qPrintable(idx) << "isn't a .idx file!" << std::endl;
    return -1;
  }
  if (!QFile::exists(idx)) {
    std::cerr << "found no file: " << qPrintable(idx) << std::endl;
    return -1;
  }
  QString sub = idx;
  sub = sub.remove(idx.lastIndexOf("."), idx.size());
  sub += ".sub";
  if (!QFile::exists(sub)) {
    std::cerr << "found no file: " << qPrintable(sub) << std::endl;
    return -1;
  }
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
  QStringList cutList = QString(argv[2]).split(",", Qt::SkipEmptyParts);
#else
  QStringList cutList = QString(argv[2]).split(",", QString::SkipEmptyParts);
#endif
  if (cutList.count() < 1) {
    std::cerr << "cut list seems to be empty!" << std::endl;
    return -1;
  }
  QString output = QString(argv[3]);
  QDir folder(QDir::toNativeSeparators(output));
  if (!folder.exists()) {
    std::cerr << "output folder: " << qPrintable(output) << " doesn't exist!" << std::endl;
    return -1;
  }

  QCoreApplication a(argc, argv);
  Cutter cut(&a);
  a.connect(&cut, SIGNAL(closeApplication()), &a, SLOT(quit()), Qt::QueuedConnection);
  cut.start(idx, sub, cutList, output);
  return a.exec();
}
