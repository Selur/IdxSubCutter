/*
 * Cutter.h
 *
 *  Created on: 27.02.2015
 *      Author: Selur
 */

#ifndef CUTTER_H_
#define CUTTER_H_
#include <QObject>
#include <QString>
#include <QStringList>

class Cutter : public QObject
{
  Q_OBJECT
  public:
    Cutter(QObject *parent);
    virtual ~Cutter();
    void start(const QString& pathToIdxFile, const QString& pathToSubFile,
        const QStringList& cutList, const QString& outputFolder);

  private:
    QString m_pathToIdxFile;
    QString m_pathToSubFile;
    QStringList m_cutList;
    QString m_outputFolder;
    QString readAll(const QString& fileName, const QString& type = "auto");
    QString getWholeFilename(const QString& fileName);
    QString adjustIdxContent(const QString& idxContent, const QString& pathToSub,
        const QStringList& cutList);
    double timeStringToDouble(const QString& timeString);

  signals:
    void closeApplication();
};

#endif /* CUTTER_H_ */
