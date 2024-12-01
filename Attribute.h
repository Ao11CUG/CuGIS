#include <QDialog>
#include <QOBject>
#include <QMenu>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QHeaderView>
#include <QVariant>
#include <QgsMapLayer.h>
#include <QgsVectorLayer.h>
#include <QgsFeatureIterator.h>
#include <QgsSourceFieldsProperties.h>
#include <qgis.h>  
#include <QgsApplication.h>
#include <QDebug>
#include "StyleSheet.h"

class Attribute :public QObject {
public:
	//图层信息
	static void openLayerInfoWindow(QgsMapLayer* layer, QWidget* parent) {
		if (!layer) return;

		// 创建并显示一个简单的新窗口
		QDialog* infoDialog = new QDialog(parent);
		infoDialog->setWindowTitle(QStringLiteral("图层信息"));

		// 获取图层类型
		QString layerType;
		switch (layer->type()) {
		case Qgis::LayerType::Vector:
			layerType = QStringLiteral("矢量");
			break;
		case Qgis::LayerType::Raster:
			layerType = QStringLiteral("栅格");
			break;
		case Qgis::LayerType::Mesh:
			layerType = QStringLiteral("网格");
			break;
		default:
			layerType = QStringLiteral("其他");
			break;
		}

		// 在窗口中显示图层信息
		QVBoxLayout* layout = new QVBoxLayout(infoDialog);
		QLabel* label = new QLabel(QStringLiteral("图层名称: %1\n图层类型: %2")
			.arg(layer->name())
			.arg(layerType),
			infoDialog);
		layout->addWidget(label);
		infoDialog->setLayout(layout);

		infoDialog->resize(300, 200);
		infoDialog->exec(); // 以模态方式显示
	}
	//展示属性表
	void showAttributeTable(QgsVectorLayer* vectorLayer, QWidget* parent) {
		if (!vectorLayer) {
			QMessageBox::warning(parent, QStringLiteral("警告"), QStringLiteral("无效的图层"));
			return;
		}

		// 检查并设置图层为可编辑状态
		if (!vectorLayer->isEditable()) {
			if (!vectorLayer->startEditing()) {
				QMessageBox::critical(parent, QStringLiteral("错误"), QStringLiteral("无法设置图层为可编辑状态"));
				return;
			}
		}

		// 创建属性表窗口
		QDialog* attributeDialog = new QDialog(parent);
		attributeDialog->setWindowTitle(QStringLiteral("属性表 - %1").arg(vectorLayer->name()));
		QVBoxLayout* layout = new QVBoxLayout(attributeDialog);

		// 创建属性表视图
		QTableWidget* tableWidget = new QTableWidget(attributeDialog);
		layout->addWidget(tableWidget);

		// 设置列头
		const auto& fields = vectorLayer->fields();
		int fieldCount = fields.size();
		tableWidget->setColumnCount(fieldCount);

		QStringList headerLabels;
		for (int i = 0; i < fieldCount; ++i) {
			headerLabels << fields[i].name();
		}
		tableWidget->setHorizontalHeaderLabels(headerLabels);

		// 填充表格
		QgsFeatureIterator iterator = vectorLayer->getFeatures();
		QgsFeature feature;
		QVector<QgsFeature> features;

		tableWidget->setRowCount(vectorLayer->featureCount());
		int row = 0;

		while (iterator.nextFeature(feature)) {
			features.push_back(feature);
			tableWidget->insertRow(row);

			for (int col = 0; col < fieldCount; ++col) {
				QVariant value = feature.attribute(col);
				QTableWidgetItem* item = new QTableWidgetItem(value.toString());
				tableWidget->setItem(row, col, item);
			}
			++row;
		}

		// 设置可编辑和触发器
		tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);

		// 标志位：记录是否有修改
		bool tableModified = false;

		// 监听属性表的修改
		connectTableItemChanged(tableWidget, vectorLayer, features, tableModified);

		// 显示窗口
		attributeDialog->resize(800, 600);
		attributeDialog->exec();

		// 如果标志位为 true，提示是否保存更改
		if (tableModified && vectorLayer->isEditable()) {
			if (QMessageBox::question(attributeDialog, QStringLiteral("保存修改"), QStringLiteral("是否保存对图层的更改？")) == QMessageBox::Yes) {
				applyChangesToLayer(vectorLayer);
			}
			else {
				vectorLayer->rollBack();
				qDebug() << "Changes rolled back for layer:" << vectorLayer->name();
			}
		}
	}
	// 修改 connectTableItemChanged 函数，加入标志位参数
	void connectTableItemChanged(QTableWidget* tableWidget, QgsVectorLayer* vectorLayer, QVector<QgsFeature> features, bool& tableModified) {
		QObject::connect(tableWidget, &QTableWidget::itemChanged, [tableWidget, vectorLayer, features, &tableModified](QTableWidgetItem* item) {
			int row = item->row();
			int col = item->column();

			// 获取对应的要素副本
			if (row < 0 || row >= features.size()) return;

			QgsFeature feature = features[row];
			QVariant newValue = item->text();
			QgsField field = vectorLayer->fields().at(col);

			// 数据类型转换
			if (field.type() == QVariant::Int) {
				bool ok;
				int intValue = item->text().toInt(&ok);
				if (!ok) {
					qDebug() << "Field type is int, but conversion failed for value:" << item->text();
					QMessageBox::warning(tableWidget, QStringLiteral("输入错误"), QStringLiteral("字段 %1 需要整数值").arg(field.name()));
					return;
				}
				newValue = intValue;
			}
			else if (field.type() == QVariant::Double) {
				bool ok;
				double doubleValue = item->text().toDouble(&ok);
				if (!ok) {
					qDebug() << "Field type is double, but conversion failed for value:" << item->text();
					QMessageBox::warning(tableWidget, QStringLiteral("输入错误"), QStringLiteral("字段 %1 需要浮点数值").arg(field.name()));
					return;
				}
				newValue = doubleValue;
			}
			else if (field.type() == QVariant::Bool) {
				newValue = (item->text().toLower() == "true" || item->text() == "1");
			}

			// 更新要素属性
			feature.setAttribute(col, newValue);
			if (!vectorLayer->updateFeature(feature)) {
				qDebug() << "Failed to update feature at row:" << row << "column:" << col << "with value:" << newValue;
				QMessageBox::warning(tableWidget, QStringLiteral("更新失败"), QStringLiteral("无法更新要素属性"));
				return;
			}

			qDebug() << "Feature updated successfully at row:" << row << "column:" << col;

			// 标志修改
			tableModified = true;

			// 刷新图层
			vectorLayer->triggerRepaint();
			});
	}
	//应用属性表的修改
	void applyChangesToLayer(QgsVectorLayer* vectorLayer) {
		if (vectorLayer->isEditable()) {
			if (!vectorLayer->commitChanges()) {
				qDebug() << "Failed to commit changes to vector layer:" << vectorLayer->name();
				QMessageBox::warning(nullptr, QStringLiteral("提交失败"), QStringLiteral("无法提交更改"));
				vectorLayer->rollBack();
			}
			else {
				qDebug() << "Changes committed successfully to vector layer:" << vectorLayer->name();
			}
		}
		else {
			qDebug() << "Vector layer is not editable:" << vectorLayer->name();
		}
	}


	//图层属性结构表
	void openAttributeEditor(QgsVectorLayer* temp_layer, QWidget* parent) {
		if (!temp_layer || !temp_layer->isValid() || Qgis::LayerType::Vector != temp_layer->type()) {
			QMessageBox::critical(parent, QStringLiteral("错误"), QStringLiteral("无效的图层或不是矢量图层！"));
			return;
		}

		// 启动编辑模式
		if (!temp_layer->isEditable()) {
			temp_layer->startEditing();
		}

		// 创建窗口
		QDialog dialog(parent);
		dialog.setWindowTitle(QStringLiteral("编辑属性结构 - %1").arg(temp_layer->name()));
		dialog.resize(600, 400);

		QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

		// 创建表格
		QTableWidget* tableWidget = new QTableWidget(&dialog);
		tableWidget->setColumnCount(2);
		tableWidget->setHorizontalHeaderLabels({ QStringLiteral("字段名称"), QStringLiteral("字段类型") });
		populateAttributeTable(temp_layer, tableWidget);
		mainLayout->addWidget(tableWidget);

		// 创建按钮
		QHBoxLayout* buttonLayout = new QHBoxLayout();
		QPushButton* addButton = new QPushButton(QStringLiteral("添加字段"), &dialog);
		style.setConfirmButton(addButton);
		QPushButton* removeButton = new QPushButton(QStringLiteral("删除字段"), &dialog);
		style.setConfirmButton(removeButton);
		QPushButton* applyButton = new QPushButton(QStringLiteral("应用更改"), &dialog);
		style.setConfirmButton(applyButton);
		QPushButton* cancelButton = new QPushButton(QStringLiteral("取消"), &dialog);
		style.setConfirmButton(cancelButton);

		buttonLayout->addWidget(addButton);
		buttonLayout->addWidget(removeButton);
		buttonLayout->addStretch();
		buttonLayout->addWidget(applyButton);
		buttonLayout->addWidget(cancelButton);
		mainLayout->addLayout(buttonLayout);

		// 信号和槽
		QObject::connect(addButton, &QPushButton::clicked, [&]() {
			addFieldToTable(tableWidget);
			});

		QObject::connect(removeButton, &QPushButton::clicked, [&]() {
			removeSelectedFieldFromTable(tableWidget);
			});

		QObject::connect(applyButton, &QPushButton::clicked, [&]() {
			applyAttributeChanges(temp_layer, tableWidget, &dialog);
			});

		QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

		dialog.exec();
	}

	void populateAttributeTable(QgsVectorLayer* layer, QTableWidget* tableWidget) {
		const QgsFields& fields = layer->fields();
		tableWidget->setRowCount(fields.size());

		for (int i = 0; i < fields.size(); ++i) {
			const QgsField& field = fields[i];
			tableWidget->setItem(i, 0, new QTableWidgetItem(field.name()));  // 字段名称
			tableWidget->setItem(i, 1, new QTableWidgetItem(QVariant::typeToName(field.type())));  // 字段类型
		}
	}

	void addFieldToTable(QTableWidget* tableWidget) {
		int row = tableWidget->rowCount();
		tableWidget->insertRow(row);
		tableWidget->setItem(row, 0, new QTableWidgetItem(QStringLiteral("新字段")));  // 默认字段名称
		tableWidget->setItem(row, 1, new QTableWidgetItem("QString"));  // 默认字段类型
	}

	void removeSelectedFieldFromTable(QTableWidget* tableWidget) {
		int currentRow = tableWidget->currentRow();
		if (currentRow >= 0) {
			tableWidget->removeRow(currentRow);
		}
		else {
			QMessageBox::warning(tableWidget->parentWidget(), QStringLiteral("警告"), QStringLiteral("请选择要删除的字段"));
		}
	}

	void applyAttributeChanges(QgsVectorLayer* layer, QTableWidget* tableWidget, QDialog* dialog) {
		if (!layer->isEditable()) {
			QMessageBox::critical(dialog, QStringLiteral("错误"), QStringLiteral("图层未处于可编辑模式！"));
			return;
		}

		const QgsFields& originalFields = layer->fields();
		QList<int> fieldsToRemove;

		// 找出需要删除的字段
		for (int i = 0; i < originalFields.size(); ++i) {
			bool exists = false;
			for (int j = 0; j < tableWidget->rowCount(); ++j) {
				if (tableWidget->item(j, 0)->text() == originalFields[i].name()) {
					exists = true;
					break;
				}
			}
			if (!exists) {
				fieldsToRemove.append(i);
			}
		}

		for (int index : fieldsToRemove) {
			if (!layer->deleteAttribute(index)) {
				QMessageBox::warning(dialog, QStringLiteral("删除失败"), QStringLiteral("无法删除字段 %1").arg(originalFields[index].name()));
			}
		}

		// 添加或更新字段
		for (int i = 0; i < tableWidget->rowCount(); ++i) {
			QString name = tableWidget->item(i, 0)->text();
			QString typeStr = tableWidget->item(i, 1)->text();
			QVariant::Type type = QVariant::nameToType(typeStr.toUtf8().constData());

			int index = layer->fields().indexOf(name);
			if (index == -1) {
				// 添加新字段
				if (!layer->addAttribute(QgsField(name, type))) {
					QMessageBox::warning(dialog, QStringLiteral("添加失败"), QStringLiteral("无法添加字段 %1").arg(name));
				}
			}
		}

		// 提交更改
		if (!layer->commitChanges()) {
			QMessageBox::critical(dialog, QStringLiteral("提交失败"), QStringLiteral("无法提交更改"));
			layer->rollBack();
		}
		else {
			dialog->accept();
		}
	}

private:
	StyleSheet style;
};


