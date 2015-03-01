/*
 * Cutter.cpp
 *
 *  Created on: 27.02.2015
 *      Author: Selur
 */

#include "Cutter.h"
#include <QFileInfo>
#include <QTextCodec>
#include <QTextStream>
#include <QDir>
#include <QFile>
#include <QTime>
#include <iostream>

Cutter::Cutter(QObject *parent)
    : QObject(parent), m_pathToIdxFile(QString()), m_pathToSubFile(QString()),
        m_cutList(QStringList()), m_outputFolder(QString())
{
  this->setObjectName("IdxSubCutter");
}

Cutter::~Cutter()
{
}

QString Cutter::readAll(const QString& fileName, const QString& type)
{
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly)) {
    return QString();
  }
  QTextStream stream(&file);
  if (type == "auto") {
    stream.autoDetectUnicode();
  } else {
    QTextCodec* codec = QTextCodec::codecForName(type.toLocal8Bit());
    if (codec != nullptr) {
      stream.setCodec(codec);
    } else {
      stream.setCodec(type.toUtf8());
    }
  }
  QString input = stream.readAll();
  file.close();
  return input;
}

QString Cutter::getWholeFilename(const QString& fileName)
{
  QFileInfo info(fileName);
  if (info.isDir()) {
    return QString();
  }
  QString output = QDir::toNativeSeparators(fileName);
  int index = output.lastIndexOf(QDir::separator());
  if (output.endsWith(QDir::separator())) {
    return QString();
  }
  if (index != -1) {
    output = output.remove(0, index + 1);
  }
  return QDir::toNativeSeparators(output);
}

double timeToSeconds(QTime time)
{
  return ((time.hour() * 60.0 + time.minute()) * 60.0 + time.second()) + (time.msec() / 1000.0);
}

double timeStringToDouble(const QString& timeString)
{
  return timeToSeconds(QTime::fromString(timeString, "hh:mm:ss:zzz"));
}

int hexToDecimal(const QString& filePos)
{
  QString str = "0x" + filePos;
  bool ok;
  int ret = str.toInt(&ok, 16);
  //std::cerr << "converted " << qPrintable(str) << " to " << ret << std::endl;
  return ret;
}

qint64 rawByteSize(const QString& pathToFile)
{
  if (pathToFile.isEmpty()) {
    return 0;
  }
  QFile file;
  file.setFileName(pathToFile);
  if (!file.exists()) {
    return 0;
  }
  qint64 ret = (qint64) (file.size());
  return ret; //in Byte
}

QStringList adjustFilePosToByteAndSetSize(const QString& idxContent, const QString& pathToSub)
{
  QString line, tmp;
  QStringList lines = idxContent.split("\n"), ret;
  qint64 subFileSizeByte = rawByteSize(pathToSub);
  int startPos, endPos, index;
  QString currentStart;
  for (int i = 0, c = lines.count(); i < c; ++i) {
    line = lines.at(i).trimmed();
    if (!line.startsWith("timestamp")) {
      ret << line;
      continue;
    }
    tmp = line; // timestamp: 00:00:13:600, filepos: 000000000
    index = tmp.lastIndexOf(":");
    tmp = tmp.remove(0, index + 1).trimmed();
    startPos = hexToDecimal(tmp);
    currentStart = line;
    currentStart = currentStart.remove(index + 1, currentStart.size());
    currentStart += " " + QString::number(startPos) + ", size: ";
    if (i + 1 < c) {
      tmp = lines.at(i + 1).trimmed();
      index = tmp.lastIndexOf(":");
      tmp = tmp.remove(0, index + 1).trimmed();
      endPos = hexToDecimal(tmp);
    } else {
      endPos = subFileSizeByte;
    }
    tmp = currentStart + QString::number(endPos - startPos);
    ret << tmp;
  }
  return ret;
}

bool keepEntry(const QString& from, const QString& to, const QString& timeStamp)
{
  double dFrom = timeStringToDouble(from);
  double dTo = timeStringToDouble(to);
  double dTimeStamp = timeStringToDouble(timeStamp);
  return dTimeStamp >= dFrom && dTimeStamp < dTo;
}

bool keepLine(const QString& line, const QStringList& cutList)
{
  QStringList parts;
  QString timeStamp;
  foreach(QString cut, cutList)
  {
    parts = cut.split("-");
    timeStamp = line;
    timeStamp = timeStamp.remove(0, 10);
    timeStamp = timeStamp.remove(timeStamp.indexOf(","), timeStamp.size()).trimmed();
    if (keepEntry(parts[0].trimmed(), parts[1].trimmed(), timeStamp)) {
      return true;
    }
  }
  return false;
}

/**
 * let those lines which are not needed start with a '#'-symbol
 */
QStringList markNotNeeded(const QStringList& lines, const QStringList& cutList)
{
  if (cutList.isEmpty()) {
    return lines;
  }
  QStringList ret;
  QString line;
  for (int i = 0, c = lines.count(); i < c; ++i) {
    line = lines.at(i).trimmed();
    if (!line.startsWith("timestamp:")) {
      ret << line;
      continue;
    }
    if (!keepLine(line, cutList)) {
      ret << "#" + line;
      continue;
    }
    ret << line;
  }
  return ret;
}

void addToByteArray(const int filePos, const int size, QByteArray& outputFileContent,
    const QByteArray& inputFileContent)
{
  //std::cerr << "addToByteArray, filePos: " << filePos << ", size: " << size << std::endl;
  outputFileContent.append(inputFileContent.mid(filePos, size));
}

void fillSubOutput(const QByteArray &subInputContent, QByteArray& subOutputContent,
    const QStringList markedLines)
{
  QString line, startPos, size;
  for (int i = 0, c = markedLines.count(); i < c; ++i) {
    line = markedLines.at(i);
    if (!line.startsWith("timestamp")) {
      continue;
    }
    //timestamp: 00:00:17:520, filepos: 4096, size: 6144
    startPos = line;
    startPos = startPos.remove(0, startPos.indexOf("filepos:") + 8);
    startPos = startPos.remove(startPos.indexOf(","), startPos.size()).trimmed();
    size = line;
    size = size.remove(0, size.lastIndexOf(":") + 1).trimmed();
    //std::cerr << "addToByteArray, filePos: " << qPrintable(startPos) << ", size: " << qPrintable(size) << std::endl;
    addToByteArray(startPos.toInt(), size.toInt(), subOutputContent, subInputContent);
  }
}

QString secondsToHMSZZZ(double seconds)
{
  if (seconds == 0) {
    return "00:00:00:000";
  }
  seconds = seconds + 0.0001; // to compensate rounding errors
  double milliSeconds = seconds * 1000.0;
  QString time = QString();
  int hrs = milliSeconds / 3600000; //Stunden
  time += QString((hrs < 10) ? "0" : QString()) + QString::number(hrs);
  int min = 0;
  milliSeconds = milliSeconds - 3600000 * hrs;
  min = milliSeconds / 60000; //Minuten
  time += ":" + QString((min < 10) ? "0" : QString()) + QString::number(min);
  milliSeconds = milliSeconds - 60000 * min;
  time += ":";
  int sec = milliSeconds / 1000;
  time += QString((sec < 10) ? "0" : QString());
  time += QString::number(sec);
  milliSeconds = milliSeconds - 1000 * sec;
  if (milliSeconds < 1) {
    time += ":000";
    return time;
  }
  time += ":";
  if (milliSeconds < 10) {
    time += "00";
  } else if (milliSeconds < 100) {
    time += "0";
  }
  time += QString::number(int(milliSeconds));
  return time;
}

QString intToHex(qint64 filePos)
{
  QString ret = "000000000";
  if (filePos == 0) {
    return ret;
  }
  ret.setNum(filePos, 16);
  while (ret.length() != 9) {
    ret = "0" + ret;
  }
  return ret;
}

QStringList adjustFilePositions(const QStringList& lines)
{
  QStringList ret;
  QString line, tmp;
  qint64 nextFilePosition = 0;
  int size;
  for (int i = 0, c = lines.count(); i < c; ++i) {
    line = lines.at(i);
    if (!line.startsWith("timestamp:")) {
      ret << line;
      continue;
    }
    //timestamp: 00:00:17:520, filepos: 4096, size: 6144
    tmp = line;
    tmp = tmp.remove(0, line.lastIndexOf(":") + 1).trimmed(); // 6144
    size = tmp.toInt();
    line = line.remove(line.indexOf(", filepos: ") + 11, line.size()); // timestamp: 00:00:17:520, filepos:
    line += intToHex(nextFilePosition);
    nextFilePosition += size;
    ret << line;
  }
  return ret;
}

double getStartOfCurrentCutSection(const QString& line, const QStringList& cutList,
    double& sumOfLengthOfAllPreviousCutSections)
{
  QStringList parts;
  QString timeStamp;
  sumOfLengthOfAllPreviousCutSections = 0;
  QString cutStart, cutEnd;
  foreach(QString cut, cutList)
  {
    parts = cut.split("-");
    timeStamp = line;
    timeStamp = timeStamp.remove(0, 10);
    timeStamp = timeStamp.remove(timeStamp.indexOf(","), timeStamp.size()).trimmed();
    cutStart = parts[0].trimmed();
    cutEnd = parts[1].trimmed();
    //std::cerr << qPrintable(QObject::tr("Cut %1-%2 vs. timestamp: %3").arg(cutStart).arg(cutEnd).arg(timeStamp))<< std::endl;
    if (keepEntry(cutStart, cutEnd, timeStamp)) {
      double start = timeStringToDouble(cutStart);
      //std::cerr << " -> startOfCurrentSection: " << start << std::endl;
      return start;
    }
    double dCutEnd = timeStringToDouble(cutEnd);
    double dCutStart = timeStringToDouble(cutStart);
    //std::cerr << qPrintable(QObject::tr("Cut %1-%2 + sumOfLengthOfAllPreviousCutSections: %3").arg(dCutEnd).arg(dCutStart).arg(sumOfLengthOfAllPreviousCutSections)) << std::endl;
    sumOfLengthOfAllPreviousCutSections += dCutEnd - dCutStart;
    //std::cerr << " -> sumOfLengthOfAllPreviousCutSections: " << sumOfLengthOfAllPreviousCutSections << std::endl;
  }
  return 0;
}

QStringList adjustFileTimeStamps(const QStringList& lines, const QStringList& cutList)
{
  QStringList ret;
  QString line, tmp;
  double timeStamp;
  double startOfCurrentCutSection;
  double sumOfLengthOfAllPreviousCutSections = 0;
  for (int i = 0, c = lines.count(); i < c; ++i) {
    line = lines.at(i);
    if (line.startsWith("#time")) {
      continue;
    }
    if (!line.startsWith("timestamp:")) {
      ret << line;
      continue;
    }
    tmp = line;
    tmp = tmp.remove(0, 10);
    tmp = tmp.remove(tmp.indexOf(","), tmp.size()).trimmed();
    startOfCurrentCutSection = getStartOfCurrentCutSection(line, cutList,
        sumOfLengthOfAllPreviousCutSections);
    timeStamp = timeStringToDouble(tmp) - startOfCurrentCutSection
        + sumOfLengthOfAllPreviousCutSections;
    line = line.replace(tmp, secondsToHMSZZZ(timeStamp));
    ret << line;
  }
  return ret;
}

QString Cutter::adjustIdxContent(const QString& idxContent, const QString& pathToSub,
    const QStringList& cutList)
{
//timestamp: 00:00:14:479, filepos: 000000000
//timestamp: 00:00:17:520, filepos: 000001000
//timestamp: 00:00:21:200, filepos: 000002800
  std::cerr << qPrintable(tr("Adjusting file position to byte and calculating sizes."))
      << std::endl;
  QStringList lines = adjustFilePosToByteAndSetSize(idxContent, pathToSub);
//timestamp: 00:00:14:479, filepos: 0, size: 4095
//timestamp: 00:00:17:520, filepos: 4096, size: 6144
//timestamp: 00:00:21:200, filepos: 10240, size: ...
  std::cerr << qPrintable(tr("Marking unneeded lines,..")) << std::endl;
  QStringList marked = markNotNeeded(lines, cutList);
  QFile subFile(pathToSub);
  if (!subFile.open(QFile::ReadOnly)) {
    std::cerr << qPrintable(tr("Couldn't open .sub file: %1").arg(pathToSub)) << std::endl;
    return QString();
  }
  QByteArray subInputContent = subFile.readAll();
  subFile.close();
  std::cerr << qPrintable(tr("Collecting content for output .sub")) << std::endl;
  QByteArray subOutputContent;
  fillSubOutput(subInputContent, subOutputContent, marked);
  QString outputSubFile = pathToSub;
  outputSubFile = outputSubFile.insert(outputSubFile.lastIndexOf("."), "_cut");
  QFile file(outputSubFile);
  if (!file.open(QIODevice::WriteOnly)) {
    std::cerr << qPrintable(tr("Couldn't write .sub output to: %1").arg(outputSubFile))
        << std::endl;
    return QString();
  }
  file.write(subOutputContent);
  file.close();
  std::cerr << qPrintable(tr("Created .sub output to: %1").arg(outputSubFile)) << std::endl;
  std::cerr << qPrintable(tr("Adjusting file positions,...")) << std::endl;
  QStringList newLines = adjustFilePositions(marked);
  std::cerr << qPrintable(tr("Adjusting timestamps,...")) << std::endl;
  newLines = adjustFileTimeStamps(newLines, cutList);
  return newLines.join("\n");
}

void Cutter::start(const QString& pathToIdxFile, const QString& pathToSubFile,
    const QStringList& cutList, const QString& outputFolder)
{
  std::cerr << qPrintable(tr("Loading content of %1.").arg(pathToIdxFile)) << std::endl;
  QString idxContent = this->readAll(pathToIdxFile);
  QString outputIdx = this->getWholeFilename(pathToIdxFile);
  outputIdx = outputIdx.insert(outputIdx.lastIndexOf("."), "_cut");
  outputIdx = QDir::toNativeSeparators(outputFolder + QDir::separator() + outputIdx);
  std::cerr << qPrintable(tr("Target output file name %1.").arg(outputIdx)) << std::endl;
  QString outputIdxContent = this->adjustIdxContent(idxContent, pathToSubFile, cutList);
  if (outputIdxContent.isEmpty()) {
    emit closeApplication();
    return;
  }
  QFile file(outputIdx);
  if (!file.open(QIODevice::WriteOnly)) {
    std::cerr << qPrintable(tr("Couldn't write to %1.").arg(outputIdx)) << std::endl;
    emit closeApplication();
    return;
  }
  file.write(outputIdxContent.toUtf8());
  file.close();
  std::cerr << qPrintable(tr("Created %1.").arg(outputIdx)) << std::endl;
  emit closeApplication();
}
