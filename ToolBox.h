#include <QApplication>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <QStringList>
#include <QIcon>
#include "StyleSheet.h"
class ToolBox : public QGroupBox {
    Q_OBJECT

public:
    ToolBox(const QString& title, const QStringList& contents, const QStringList& icons, QWidget* parent = nullptr)
        : QGroupBox(title, parent) {

        // ����Ϊ���۵�
        setCheckable(true);
        setChecked(false);

        QVBoxLayout* layout = new QVBoxLayout(this);

        // ������������
        createToolContent(layout, contents, icons);

        layout->setAlignment(Qt::AlignTop);
        setLayout(layout); // ���ò���
        connect(this, &QGroupBox::toggled, this, &ToolBox::toggleContentVisibility);
    }

    QList<QPushButton*> getButtons() const {
        return buttons; // �ṩ��ȡ��ť�Ľӿ�
    }

private slots:
    void toggleContentVisibility(bool checked) {
        for (auto widget : findChildren<QWidget*>()) {
            if (widget != this) {
                widget->setVisible(checked);
            }
        }
    }

private:
    StyleSheet style;

    QList<QPushButton*> buttons; // �洢��ť

    void createToolContent(QVBoxLayout* layout, const QStringList& contents, const QStringList& icons) {
        // ȷ�����ݺ�ͼ���������ͬ
        if (contents.size() != icons.size()) {
            // ����ƥ�������������׳��쳣���¼����
            return;
        }

        for (int i = 0; i < contents.size(); ++i) {
            // ��� QPushButton
            QPushButton* button = new QPushButton(contents[i], this);

            buttons.append(button); // ��Ӱ�ť���б�

            // ����ͼ��
            button->setIcon(QIcon(icons[i])); // ʹ�� QIcon ����ͼ��
            button->setIconSize(QSize(24, 24)); // ������Ҫ����ͼ���С

            // ͳһ��ʽ
            style.setCommonButton(button);

            layout->addWidget(button);
        }
    }
};
