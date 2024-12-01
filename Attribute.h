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
	//ͼ����Ϣ
	static void openLayerInfoWindow(QgsMapLayer* layer, QWidget* parent) {
		if (!layer) return;

		// ��������ʾһ���򵥵��´���
		QDialog* infoDialog = new QDialog(parent);
		infoDialog->setWindowTitle(QStringLiteral("ͼ����Ϣ"));

		// ��ȡͼ������
		QString layerType;
		switch (layer->type()) {
		case Qgis::LayerType::Vector:
			layerType = QStringLiteral("ʸ��");
			break;
		case Qgis::LayerType::Raster:
			layerType = QStringLiteral("դ��");
			break;
		case Qgis::LayerType::Mesh:
			layerType = QStringLiteral("����");
			break;
		default:
			layerType = QStringLiteral("����");
			break;
		}

		// �ڴ�������ʾͼ����Ϣ
		QVBoxLayout* layout = new QVBoxLayout(infoDialog);
		QLabel* label = new QLabel(QStringLiteral("ͼ������: %1\nͼ������: %2")
			.arg(layer->name())
			.arg(layerType),
			infoDialog);
		layout->addWidget(label);
		infoDialog->setLayout(layout);

		infoDialog->resize(300, 200);
		infoDialog->exec(); // ��ģ̬��ʽ��ʾ
	}
	//չʾ���Ա�
	void showAttributeTable(QgsVectorLayer* vectorLayer, QWidget* parent) {
		if (!vectorLayer) {
			QMessageBox::warning(parent, QStringLiteral("����"), QStringLiteral("��Ч��ͼ��"));
			return;
		}

		// ��鲢����ͼ��Ϊ�ɱ༭״̬
		if (!vectorLayer->isEditable()) {
			if (!vectorLayer->startEditing()) {
				QMessageBox::critical(parent, QStringLiteral("����"), QStringLiteral("�޷�����ͼ��Ϊ�ɱ༭״̬"));
				return;
			}
		}

		// �������Ա���
		QDialog* attributeDialog = new QDialog(parent);
		attributeDialog->setWindowTitle(QStringLiteral("���Ա� - %1").arg(vectorLayer->name()));
		QVBoxLayout* layout = new QVBoxLayout(attributeDialog);

		// �������Ա���ͼ
		QTableWidget* tableWidget = new QTableWidget(attributeDialog);
		layout->addWidget(tableWidget);

		// ������ͷ
		const auto& fields = vectorLayer->fields();
		int fieldCount = fields.size();
		tableWidget->setColumnCount(fieldCount);

		QStringList headerLabels;
		for (int i = 0; i < fieldCount; ++i) {
			headerLabels << fields[i].name();
		}
		tableWidget->setHorizontalHeaderLabels(headerLabels);

		// �����
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

		// ���ÿɱ༭�ʹ�����
		tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);

		// ��־λ����¼�Ƿ����޸�
		bool tableModified = false;

		// �������Ա���޸�
		connectTableItemChanged(tableWidget, vectorLayer, features, tableModified);

		// ��ʾ����
		attributeDialog->resize(800, 600);
		attributeDialog->exec();

		// �����־λΪ true����ʾ�Ƿ񱣴����
		if (tableModified && vectorLayer->isEditable()) {
			if (QMessageBox::question(attributeDialog, QStringLiteral("�����޸�"), QStringLiteral("�Ƿ񱣴��ͼ��ĸ��ģ�")) == QMessageBox::Yes) {
				applyChangesToLayer(vectorLayer);
			}
			else {
				vectorLayer->rollBack();
				qDebug() << "Changes rolled back for layer:" << vectorLayer->name();
			}
		}
	}
	// �޸� connectTableItemChanged �����������־λ����
	void connectTableItemChanged(QTableWidget* tableWidget, QgsVectorLayer* vectorLayer, QVector<QgsFeature> features, bool& tableModified) {
		QObject::connect(tableWidget, &QTableWidget::itemChanged, [tableWidget, vectorLayer, features, &tableModified](QTableWidgetItem* item) {
			int row = item->row();
			int col = item->column();

			// ��ȡ��Ӧ��Ҫ�ظ���
			if (row < 0 || row >= features.size()) return;

			QgsFeature feature = features[row];
			QVariant newValue = item->text();
			QgsField field = vectorLayer->fields().at(col);

			// ��������ת��
			if (field.type() == QVariant::Int) {
				bool ok;
				int intValue = item->text().toInt(&ok);
				if (!ok) {
					qDebug() << "Field type is int, but conversion failed for value:" << item->text();
					QMessageBox::warning(tableWidget, QStringLiteral("�������"), QStringLiteral("�ֶ� %1 ��Ҫ����ֵ").arg(field.name()));
					return;
				}
				newValue = intValue;
			}
			else if (field.type() == QVariant::Double) {
				bool ok;
				double doubleValue = item->text().toDouble(&ok);
				if (!ok) {
					qDebug() << "Field type is double, but conversion failed for value:" << item->text();
					QMessageBox::warning(tableWidget, QStringLiteral("�������"), QStringLiteral("�ֶ� %1 ��Ҫ������ֵ").arg(field.name()));
					return;
				}
				newValue = doubleValue;
			}
			else if (field.type() == QVariant::Bool) {
				newValue = (item->text().toLower() == "true" || item->text() == "1");
			}

			// ����Ҫ������
			feature.setAttribute(col, newValue);
			if (!vectorLayer->updateFeature(feature)) {
				qDebug() << "Failed to update feature at row:" << row << "column:" << col << "with value:" << newValue;
				QMessageBox::warning(tableWidget, QStringLiteral("����ʧ��"), QStringLiteral("�޷�����Ҫ������"));
				return;
			}

			qDebug() << "Feature updated successfully at row:" << row << "column:" << col;

			// ��־�޸�
			tableModified = true;

			// ˢ��ͼ��
			vectorLayer->triggerRepaint();
			});
	}
	//Ӧ�����Ա���޸�
	void applyChangesToLayer(QgsVectorLayer* vectorLayer) {
		if (vectorLayer->isEditable()) {
			if (!vectorLayer->commitChanges()) {
				qDebug() << "Failed to commit changes to vector layer:" << vectorLayer->name();
				QMessageBox::warning(nullptr, QStringLiteral("�ύʧ��"), QStringLiteral("�޷��ύ����"));
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


	//ͼ�����Խṹ��
	void openAttributeEditor(QgsVectorLayer* temp_layer, QWidget* parent) {
		if (!temp_layer || !temp_layer->isValid() || Qgis::LayerType::Vector != temp_layer->type()) {
			QMessageBox::critical(parent, QStringLiteral("����"), QStringLiteral("��Ч��ͼ�����ʸ��ͼ�㣡"));
			return;
		}

		// �����༭ģʽ
		if (!temp_layer->isEditable()) {
			temp_layer->startEditing();
		}

		// ��������
		QDialog dialog(parent);
		dialog.setWindowTitle(QStringLiteral("�༭���Խṹ - %1").arg(temp_layer->name()));
		dialog.resize(600, 400);

		QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

		// �������
		QTableWidget* tableWidget = new QTableWidget(&dialog);
		tableWidget->setColumnCount(2);
		tableWidget->setHorizontalHeaderLabels({ QStringLiteral("�ֶ�����"), QStringLiteral("�ֶ�����") });
		populateAttributeTable(temp_layer, tableWidget);
		mainLayout->addWidget(tableWidget);

		// ������ť
		QHBoxLayout* buttonLayout = new QHBoxLayout();
		QPushButton* addButton = new QPushButton(QStringLiteral("����ֶ�"), &dialog);
		style.setConfirmButton(addButton);
		QPushButton* removeButton = new QPushButton(QStringLiteral("ɾ���ֶ�"), &dialog);
		style.setConfirmButton(removeButton);
		QPushButton* applyButton = new QPushButton(QStringLiteral("Ӧ�ø���"), &dialog);
		style.setConfirmButton(applyButton);
		QPushButton* cancelButton = new QPushButton(QStringLiteral("ȡ��"), &dialog);
		style.setConfirmButton(cancelButton);

		buttonLayout->addWidget(addButton);
		buttonLayout->addWidget(removeButton);
		buttonLayout->addStretch();
		buttonLayout->addWidget(applyButton);
		buttonLayout->addWidget(cancelButton);
		mainLayout->addLayout(buttonLayout);

		// �źźͲ�
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
			tableWidget->setItem(i, 0, new QTableWidgetItem(field.name()));  // �ֶ�����
			tableWidget->setItem(i, 1, new QTableWidgetItem(QVariant::typeToName(field.type())));  // �ֶ�����
		}
	}

	void addFieldToTable(QTableWidget* tableWidget) {
		int row = tableWidget->rowCount();
		tableWidget->insertRow(row);
		tableWidget->setItem(row, 0, new QTableWidgetItem(QStringLiteral("���ֶ�")));  // Ĭ���ֶ�����
		tableWidget->setItem(row, 1, new QTableWidgetItem("QString"));  // Ĭ���ֶ�����
	}

	void removeSelectedFieldFromTable(QTableWidget* tableWidget) {
		int currentRow = tableWidget->currentRow();
		if (currentRow >= 0) {
			tableWidget->removeRow(currentRow);
		}
		else {
			QMessageBox::warning(tableWidget->parentWidget(), QStringLiteral("����"), QStringLiteral("��ѡ��Ҫɾ�����ֶ�"));
		}
	}

	void applyAttributeChanges(QgsVectorLayer* layer, QTableWidget* tableWidget, QDialog* dialog) {
		if (!layer->isEditable()) {
			QMessageBox::critical(dialog, QStringLiteral("����"), QStringLiteral("ͼ��δ���ڿɱ༭ģʽ��"));
			return;
		}

		const QgsFields& originalFields = layer->fields();
		QList<int> fieldsToRemove;

		// �ҳ���Ҫɾ�����ֶ�
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
				QMessageBox::warning(dialog, QStringLiteral("ɾ��ʧ��"), QStringLiteral("�޷�ɾ���ֶ� %1").arg(originalFields[index].name()));
			}
		}

		// ��ӻ�����ֶ�
		for (int i = 0; i < tableWidget->rowCount(); ++i) {
			QString name = tableWidget->item(i, 0)->text();
			QString typeStr = tableWidget->item(i, 1)->text();
			QVariant::Type type = QVariant::nameToType(typeStr.toUtf8().constData());

			int index = layer->fields().indexOf(name);
			if (index == -1) {
				// ������ֶ�
				if (!layer->addAttribute(QgsField(name, type))) {
					QMessageBox::warning(dialog, QStringLiteral("���ʧ��"), QStringLiteral("�޷�����ֶ� %1").arg(name));
				}
			}
		}

		// �ύ����
		if (!layer->commitChanges()) {
			QMessageBox::critical(dialog, QStringLiteral("�ύʧ��"), QStringLiteral("�޷��ύ����"));
			layer->rollBack();
		}
		else {
			dialog->accept();
		}
	}

private:
	StyleSheet style;
};


