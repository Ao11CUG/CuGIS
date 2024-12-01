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
			"    background-color: #4CAF50;"     // 背景色
			"    color: white;"                    // 文本颜色
			"    border: none;"                    // 去掉边框
			"    padding: 10px;"                   // 内边距
			"    border-radius: 5px;"              // 圆角
			"    font-size: 25px;"                 // 字体大小
			"}"
			"QPushButton:hover {"
			"    background-color: #45a049;"      // 悬停时背景色
			"}"
			"QPushButton:pressed {"
			"    background-color: #3e8e41;"      // 按下时背景色
			"}");
	}

	void setDialogConfirmButton(QDialogButtonBox* buttonBox) {
		buttonBox->setStyleSheet(
			"QPushButton {"
			"    background-color: #4CAF50;"     // 背景色
			"    color: white;"                    // 文本颜色
			"    border: none;"                    // 去掉边框
			"    padding: 10px;"                   // 内边距
			"    border-radius: 5px;"              // 圆角
			"    font-size: 25px;"                 // 字体大小
			"}"
			"QPushButton:hover {"
			"    background-color: #45a049;"      // 悬停时背景色
			"}"
			"QPushButton:pressed {"
			"    background-color: #3e8e41;"      // 按下时背景色
			"}");
	}

	void setCommonButton(QPushButton* button) {
		button->setStyleSheet(
			"QPushButton {"
			"    padding: 10px;"                   // 内边距
			"    border-radius: 5px;"              // 圆角
			"    font-size: 25px;"                 // 字体大小
			"}"
			"QPushButton:hover {"
			"    background-color: #C0C0C0;"      // 悬停时背景色
			"}"
			"QPushButton:pressed {"
			"    background-color: #A0A0A0;"      // 按下时背景色
			"}");
	}

	void setCalculatorButton(QPushButton* button) {
		button->setStyleSheet(
			"QPushButton {"
			"    padding: 10px;"                   // 内边距
			"    border: 2px solid #4A4A4A;"       // 明确的边界，2像素宽，颜色为深灰色
			"    border-radius: 5px;"              // 圆角
			"    font-size: 25px;"                 // 字体大小
			"    background-color: #E8E8E8;"       // 默认背景色
			"}"
			"QPushButton:hover {"
			"    background-color: #C0C0C0;"      // 悬停时背景色
			"}"
			"QPushButton:pressed {"
			"    background-color: #A0A0A0;"      // 按下时背景色
			"}");
	}

	void setLineEdit(QLineEdit* lineEdit) {
		lineEdit->setStyleSheet(
			"QLineEdit {"
			"    padding: 10px;"                   // 内边距
			"    border-radius: 5px;"              // 圆角
			"    font-size: 25px;"                 // 字体大小
			"}"
		);
	}

	void setComboBox(QComboBox* box) {
		box->setStyleSheet(
			"QComboBox {"
			"    padding: 10px;"                   // 内边距
			"    border-radius: 5px;"              // 圆角
			"    font-size: 25px;"                 // 字体大小
			"}"
		);
	}

	void setSpinBox(QSpinBox* box) {
		box->setStyleSheet(
			"QSpinBox {"
			"    padding: 10px;"                   // 内边距
			"    border-radius: 5px;"              // 圆角
			"    font-size: 25px;"                 // 字体大小
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