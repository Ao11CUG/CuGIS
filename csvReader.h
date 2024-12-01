#ifndef CSVREADER_H
#define CSVREADER_H

#include <QObject>
#include <QString>
#include <QFileInfo>
#include <QDir>
#include <qgsMapCanvas.h>

// csvReader �ࣺ���ڽ� CSV �ļ�ת��Ϊ SHP �ļ������ص���ͼ����
class csvReader : public QObject {
	Q_OBJECT

public:
	// ���캯��
	explicit csvReader(QgsMapCanvas* mapCanvas, QObject* parent = nullptr);

	// �� CSV �ļ�
	QgsVectorLayer* openCSVFile();

private:
	// �� CSV �ļ�ת��Ϊ SHP �ļ�
	QgsVectorLayer* convertCSVToShapefile(const QString& csvFilePath, const QString& shpOutputPath);
	// ���� SHP �ļ�����ͼ����
	QgsVectorLayer* loadShapefile(const QString& shpFilePath);
};

#endif // CSVREADER_H


