#pragma once
#include "qpushbutton.h"
#include "qlineedit.h"
#include "qcombobox.h"
#include "qmenubar.h"
#include "qdialogbuttonbox.h"
#include "qlistview.h"
#include "qspinbox.h"

class StyleSheet {
public:
	void setConfirmButton(QPushButton* button) {
		button->setStyleSheet(
			"QPushButton {"
			"    background-color: #4CAF50;"     // ����ɫ
			"    color: white;"                    // �ı���ɫ
			"    border: none;"                    // ȥ���߿�
			"    padding: 10px;"                   // �ڱ߾�
			"    border-radius: 5px;"              // Բ��
			"    font-size: 25px;"                 // �����С
			"}"
			"QPushButton:hover {"
			"    background-color: #45a049;"      // ��ͣʱ����ɫ
			"}"
			"QPushButton:pressed {"
			"    background-color: #3e8e41;"      // ����ʱ����ɫ
			"}");
	}

	void setDialogConfirmButton(QDialogButtonBox* buttonBox) {
		buttonBox->setStyleSheet(
			"QPushButton {"
			"    background-color: #4CAF50;"     // ����ɫ
			"    color: white;"                    // �ı���ɫ
			"    border: none;"                    // ȥ���߿�
			"    padding: 10px;"                   // �ڱ߾�
			"    border-radius: 5px;"              // Բ��
			"    font-size: 25px;"                 // �����С
			"}"
			"QPushButton:hover {"
			"    background-color: #45a049;"      // ��ͣʱ����ɫ
			"}"
			"QPushButton:pressed {"
			"    background-color: #3e8e41;"      // ����ʱ����ɫ
			"}");
	}

	void setCommonButton(QPushButton* button) {
		button->setStyleSheet(
			"QPushButton {"
			"    padding: 10px;"                   // �ڱ߾�
			"    border-radius: 5px;"              // Բ��
			"    font-size: 25px;"                 // �����С
			"}"
			"QPushButton:hover {"
			"    background-color: #C0C0C0;"      // ��ͣʱ����ɫ
			"}"
			"QPushButton:pressed {"
			"    background-color: #A0A0A0;"      // ����ʱ����ɫ
			"}");
	}

	void setCalculatorButton(QPushButton* button) {
		button->setStyleSheet(
			"QPushButton {"
			"    padding: 10px;"                   // �ڱ߾�
			"    border: 2px solid #4A4A4A;"       // ��ȷ�ı߽磬2���ؿ���ɫΪ���ɫ
			"    border-radius: 5px;"              // Բ��
			"    font-size: 25px;"                 // �����С
			"    background-color: #E8E8E8;"       // Ĭ�ϱ���ɫ
			"}"
			"QPushButton:hover {"
			"    background-color: #C0C0C0;"      // ��ͣʱ����ɫ
			"}"
			"QPushButton:pressed {"
			"    background-color: #A0A0A0;"      // ����ʱ����ɫ
			"}");
	}

	void setLineEdit(QLineEdit* lineEdit) {
		lineEdit->setStyleSheet(
			"QLineEdit {"
			"    padding: 10px;"                   // �ڱ߾�
			"    border-radius: 5px;"              // Բ��
			"    font-size: 25px;"                 // �����С
			"}"
		);
	}

	void setComboBox(QComboBox* box) {
		box->setStyleSheet(
			"QComboBox {"
			"    padding: 10px;"                   // �ڱ߾�
			"    border-radius: 5px;"              // Բ��
			"    font-size: 25px;"                 // �����С
			"}"
		);
	}

	void setSpinBox(QSpinBox* box) {
		box->setStyleSheet(
			"QSpinBox {"
			"    padding: 10px;"                   // �ڱ߾�
			"    border-radius: 5px;"              // Բ��
			"    font-size: 25px;"                 // �����С
			"}"
		);
	}

	void setMenuBar(QMenuBar* menuBar) {
		menuBar->setStyleSheet(
			"QMenuBar {"
			"    background-color: #333;"
			"    color: white;"
			"}"
			"QMenuBar::item {"
			"    spacing: 3px;"
			"    padding: 10px;"
			"}"
			"QMenuBar::item:selected {"
			"    background-color: #444;"
			"}");
	}
};