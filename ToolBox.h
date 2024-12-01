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

        // 设置为可折叠
        setCheckable(true);
        setChecked(false);

        QVBoxLayout* layout = new QVBoxLayout(this);

        // 创建工具内容
        createToolContent(layout, contents, icons);

        layout->setAlignment(Qt::AlignTop);
        setLayout(layout); // 设置布局
        connect(this, &QGroupBox::toggled, this, &ToolBox::toggleContentVisibility);
    }

    QList<QPushButton*> getButtons() const {
        return buttons; // 提供获取按钮的接口
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

    QList<QPushButton*> buttons; // 存储按钮

    void createToolContent(QVBoxLayout* layout, const QStringList& contents, const QStringList& icons) {
        // 确保内容和图标的数量相同
        if (contents.size() != icons.size()) {
            // 处理不匹配的情况，比如抛出异常或记录错误
            return;
        }

        for (int i = 0; i < contents.size(); ++i) {
            // 添加 QPushButton
            QPushButton* button = new QPushButton(contents[i], this);

            buttons.append(button); // 添加按钮到列表

            // 设置图标
            button->setIcon(QIcon(icons[i])); // 使用 QIcon 加载图标
            button->setIconSize(QSize(24, 24)); // 根据需要调整图标大小

            // 统一样式
            style.setCommonButton(button);

            layout->addWidget(button);
        }
    }
};
