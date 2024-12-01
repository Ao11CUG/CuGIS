#pragma once
#include <QListWidgetItem>
#include <QFileDialog>
#include <QIcon>
#include <QListWidget>
#include <QDialog>
#include <QDialog>
#include<qmessagebox.h>
#include <QFile>
#include <QTextStream>
#include <QDomDocument>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QPixmap>
#include<qtablewidget.h>
#include<QPainter>
#include "ui_SymbolLibManager.h"
#include <QgsSymbol.h>
#include <QgsMapLayer.h>
#include <QXmlStreamReader>
#include <qgsstyle.h>
#include <qgsstylemanagerdialog.h>
#include <qgssymbol.h>
#include <qgsvectorlayer.h>
#include <QDir>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QListWidgetItem>
#include <QPixmap>
#include "StyleSheet.h"

class SymbolLibManager : public QDialog
{
	Q_OBJECT

public:
	SymbolLibManager(QWidget* parent = nullptr);
	~SymbolLibManager();

	void setCurrentLayer(QgsMapLayer* layer);//选择图层


private slots:

	void on_btnImport_clicked();      // 导入符号库
	void on_btnExportSLD_clicked();   // 导出SLD文件
	void on_applyButton_clicked();//符号样式应用
	void onLayerComboBoxIndexChanged(int index);//选择图层

private:
	Ui::SymbolLibManagerClass ui;

	StyleSheet style;
	void setStyle();

	QgsStyle* mStyle = nullptr;

	// 导入XML符号库
	void importSymbolLibrary(const QString& fileName);
	//导入SLD
	void importSLD(const QString& fileName);
	QgsMapLayer* m_curMapLayer; //当前选中的图层
	// 从 SLD 文件的 Rule 元素创建符号
	QgsSymbol* createSymbolFromSLD(const QDomElement& ruleElement);
};
