#ifndef CSVREADER_H
#define CSVREADER_H

#include <QObject>
#include <QString>
#include <QFileInfo>
#include <QDir>
#include <qgsMapCanvas.h>

// csvReader 类：用于将 CSV 文件转换为 SHP 文件并加载到地图画布
class csvReader : public QObject {
	Q_OBJECT

public:
	// 构造函数
	explicit csvReader(QgsMapCanvas* mapCanvas, QObject* parent = nullptr);

	// 打开 CSV 文件
	QgsVectorLayer* openCSVFile();

private:
	// 将 CSV 文件转换为 SHP 文件
	QgsVectorLayer* convertCSVToShapefile(const QString& csvFilePath, const QString& shpOutputPath);
	// 加载 SHP 文件到地图画布
	QgsVectorLayer* loadShapefile(const QString& shpFilePath);
};

#endif // CSVREADER_H


