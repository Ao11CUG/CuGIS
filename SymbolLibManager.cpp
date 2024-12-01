#include "SymbolLibManager.h"
#include<qprogressdialog.h>
#include <QgsPreviewEffect.h>
#include<qgssinglesymbolrenderer.h>
#include<qgslinesymbol.h>
#include<qgslinesymbol.h>
#include<qgsfillsymbol.h>
#include<qgsmarkersymbol.h>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include<qgsproject.h>
#include<qgsmaptool.h>
#include<qgsmaptoolidentify.h>

SymbolLibManager::SymbolLibManager(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    setWindowIcon(QIcon(":/MainInterface/res/symbol.png"));
    setWindowTitle(QStringLiteral("符号管理器"));

    setStyle();

    ui.listWidgetSymbols->setSelectionMode(QAbstractItemView::MultiSelection);

    // 初始化 mStyle
    mStyle = QgsStyle::defaultStyle();  // 使用默认样式
     // 连接 LayerComboBox 的信号槽，选中变化时更新 m_curMapLayer
    connect(ui.LayerComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &SymbolLibManager::onLayerComboBoxIndexChanged);
}

SymbolLibManager::~SymbolLibManager()
{}

void SymbolLibManager::setStyle() {
    style.setConfirmButton(ui.applyButton);

    style.setConfirmButton(ui.btnImport);

    style.setConfirmButton(ui.btnExportSLD);

    style.setComboBox(ui.LayerComboBox);
}

//导入按钮
void SymbolLibManager::on_btnImport_clicked()
{
    // 选择要导入的文件，支持多种格式
    QStringList fileNames = QFileDialog::getOpenFileNames(this, QStringLiteral("导入符号库或样式文件"), "",
        QStringLiteral("符号库文件 (*.xml);;SLD样式文件 (*.sld)"));

    if (fileNames.isEmpty()) {
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // 根据文件类型选择导入方式
    for (const QString& fileName : fileNames) {
        QFileInfo fileInfo(fileName);
        QString fileExtension = fileInfo.suffix().toLower();  // 获取文件扩展名

        if (fileExtension == "xml") {
            // 导入符号库
            importSymbolLibrary(fileName);
        }
        else if (fileExtension == "sld") {
            // 导入SLD样式文件
            importSLD(fileName);
        }
        else {
            QMessageBox::warning(this, QStringLiteral("文件格式不支持"), QStringLiteral("当前只支持导入 .xml 或 .sld 文件"));
        }
    }

    QApplication::restoreOverrideCursor();
}

// 导入 XML 格式文件
void SymbolLibManager::importSymbolLibrary(const QString& fileName)
{
    bool status = mStyle->importXml(fileName);
    if (!status) {
        QFileInfo file(fileName);
        QMessageBox::warning(this, QStringLiteral("导入符号库"), QStringLiteral("导入符号库 '%1' 失败！").arg(file.baseName()));
        return;
    }

    // 获取所有符号名称
    QStringList symbolNames = mStyle->allNames(QgsStyle::StyleEntity::SymbolEntity);
    for (const QString& symbolName : symbolNames) {
        // 根据符号名称获取符号
        QgsSymbol* symbol = mStyle->symbol(symbolName);
        if (symbol) {
            // 获取符号的预览图像
            QSize iconSize(32, 32); // 设置预览图像的大小
            QImage previewImage = symbol->asImage(iconSize);

            // 如果符号存在，生成标签（名称）
            QString symbolLabel = symbolName; // 默认使用符号名称作为标签

            // 创建 QListWidgetItem
            QListWidgetItem* item = new QListWidgetItem(symbolLabel);

            // 存储符号对象和预览图像
            item->setData(Qt::UserRole, QVariant::fromValue((void*)symbol)); // 存储符号对象
            item->setData(Qt::DecorationRole, previewImage); // 设置符号的预览图像

            // 将项添加到符号列表中
            ui.listWidgetSymbols->addItem(item);
        }
    }
}

// 导入 SLD 样式文件
void SymbolLibManager::importSLD(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("导入失败"), QStringLiteral("无法打开 SLD 文件 '%1'").arg(fileName));
        return;
    }

    // 解析 SLD 文件
    QDomDocument doc;
    if (!doc.setContent(&file)) {
        QMessageBox::warning(this, QStringLiteral("导入失败"), QStringLiteral("无法解析 SLD 文件"));
        return;
    }

    file.close();

    // 获取 SLD 文件中的规则并导入符号
    QDomElement sld = doc.documentElement();
    QDomNodeList namedLayers = sld.elementsByTagName("NamedLayer");

    for (int i = 0; i < namedLayers.count(); ++i) {
        QDomElement namedLayer = namedLayers.at(i).toElement();
        QDomNodeList userStyles = namedLayer.elementsByTagName("UserStyle");

        for (int j = 0; j < userStyles.count(); ++j) {
            QDomElement userStyle = userStyles.at(j).toElement();

            // 从 UserStyle 中获取规则（Rule）
            QDomNodeList rules = userStyle.elementsByTagName("Rule");
            for (int k = 0; k < rules.count(); ++k) {
                QDomElement rule = rules.at(k).toElement();

                // 从 Rule 中获取名称
                QString ruleName = rule.attribute("Name");
                if (ruleName.isEmpty()) {
                    // 如果没有 Name 属性，可以使用默认名称
                    ruleName = QString("Rule %1").arg(k);  // 使用规则的索引作为名称
                }

                // 创建符号并从 SLD 解析符号样式
                QgsSymbol* symbol = createSymbolFromSLD(rule);
                if (symbol) {
                    // 获取符号的预览图像
                    QSize iconSize(32, 32); // 设置预览图像的大小
                    QImage previewImage = symbol->asImage(iconSize);

                    // 创建 QListWidgetItem 并设置符号名称
                    QListWidgetItem* item = new QListWidgetItem(ruleName);

                    // 存储符号对象和预览图像
                    item->setData(Qt::UserRole, QVariant::fromValue((void*)symbol)); // 存储符号对象
                    item->setData(Qt::DecorationRole, previewImage); // 设置符号的预览图像

                    // 将项添加到符号列表中
                    ui.listWidgetSymbols->addItem(item);
                }
            }
        }
    }
}

// 从 SLD 文件的 Rule 元素创建符号
QgsSymbol* SymbolLibManager::createSymbolFromSLD(const QDomElement& ruleElement)
{
    QgsSymbol* symbol = nullptr;

    // 解析填充符号
    QDomNodeList fills = ruleElement.elementsByTagName("Fill");
    if (!fills.isEmpty()) {
        symbol = QgsFillSymbol::createSimple(QMap<QString, QVariant>{
            {"color", QColor(fills.at(0).toElement().text()).name()}
        });
    }

    // 解析线条符号
    QDomNodeList strokes = ruleElement.elementsByTagName("Stroke");
    if (!strokes.isEmpty()) {
        if (!symbol) {
            symbol = QgsLineSymbol::createSimple(QMap<QString, QVariant>{
                {"color", QColor(strokes.at(0).toElement().text()).name()}
            });
        }
        else {
            // 如果已经有填充符号，添加线条
            QgsLineSymbol* lineSymbol = dynamic_cast<QgsLineSymbol*>(symbol);
            if (lineSymbol) {
                lineSymbol->setColor(QColor(strokes.at(0).toElement().text()));
            }
        }
    }

    // 解析标记符号（例如点符号）
    QDomNodeList points = ruleElement.elementsByTagName("PointSymbolizer");
    if (!points.isEmpty()) {
        // 创建一个默认的标记符号（例如圆形标记）
        QgsMarkerSymbol* markerSymbol = QgsMarkerSymbol::createSimple(QMap<QString, QVariant>{
            {"color", QColor(points.at(0).toElement().text()).name()},
            { "size", 6 }  // 默认大小为 6
        });

        // 如果有标记颜色，设置颜色
        QDomNodeList fillColor = points.at(0).toElement().elementsByTagName("Fill");
        if (!fillColor.isEmpty()) {
            markerSymbol->setColor(QColor(fillColor.at(0).toElement().text()));
        }

        symbol = markerSymbol;
    }

    // 解析图形符号（如图像符号或自定义符号）
    QDomNodeList graphics = ruleElement.elementsByTagName("Graphic");
    if (!graphics.isEmpty()) {
        QDomElement graphicElement = graphics.at(0).toElement();

        // 解析 ExternalGraphic（SVG）
        QDomNodeList externalGraphics = graphicElement.elementsByTagName("ExternalGraphic");
        if (!externalGraphics.isEmpty()) {
            QDomElement externalGraphicElement = externalGraphics.at(0).toElement();
            QString svgFilePath = externalGraphicElement.attribute("href");

            if (!svgFilePath.isEmpty()) {
                // 创建一个SVG标记符号
                QgsMarkerSymbol* svgMarkerSymbol = QgsMarkerSymbol::createSimple(QMap<QString, QVariant>{
                    {"size", 16},  // 默认大小
                    { "file", svgFilePath }  // 设置SVG文件路径
                });
                symbol = svgMarkerSymbol;
            }
        }
        else {
            // 如果没有 ExternalGraphic，检查 Mark（如矩形、圆形等）
            QDomNodeList marks = graphicElement.elementsByTagName("Mark");
            if (!marks.isEmpty()) {
                QDomElement markElement = marks.at(0).toElement();
                QString markName = markElement.attribute("wellKnownName");

                if (markName == "circle") {
                    // 创建一个简单的圆形标记
                    QgsMarkerSymbol* circleMarker = QgsMarkerSymbol::createSimple(QMap<QString, QVariant>{
                        {"color", "black"},  // 设置颜色为黑色
                        { "size", 8 }  // 默认大小
                    });
                    symbol = circleMarker;
                }
                else if (markName == "square") {
                    // 创建一个简单的方形标记
                    QgsMarkerSymbol* squareMarker = QgsMarkerSymbol::createSimple(QMap<QString, QVariant>{
                        {"color", "black"},  // 设置颜色为黑色
                        { "size", 8 }  // 默认大小
                    });
                    symbol = squareMarker;
                }
            }
        }
    }
    return symbol;

}

//导出为SLD格式
void SymbolLibManager::on_btnExportSLD_clicked()
{
    // 获取当前选中的多个符号
    QList<QListWidgetItem*> selectedItems = ui.listWidgetSymbols->selectedItems();

    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("导出失败"), QStringLiteral("请先选择要导出的符号！"));
        return;
    }

    // 弹出文件保存对话框
    QString fileName = QFileDialog::getSaveFileName(this, QStringLiteral("保存为SLD文件"), "", QStringLiteral("SLD样式文件 (*.sld)"));

    if (fileName.isEmpty()) {
        return;  // 用户取消了保存
    }

    // 创建 XML 文档
    QDomDocument doc;
    QDomElement sld = doc.createElement("StyledLayerDescriptor");
    sld.setAttribute("version", "1.0.0");
    doc.appendChild(sld);

    // 遍历选中的符号，导出每个符号的样式
    QDomElement namedLayer = doc.createElement("NamedLayer");
    sld.appendChild(namedLayer);

    QDomElement layerName = doc.createElement("Name");
    QDomText layerNameText = doc.createTextNode("Symbols Layer");
    layerName.appendChild(layerNameText);
    namedLayer.appendChild(layerName);

    // 创建样式元素
    QDomElement userStyle = doc.createElement("UserStyle");
    namedLayer.appendChild(userStyle);

    QDomElement styleName = doc.createElement("Name");
    QDomText styleNameText = doc.createTextNode("Combined Symbol Styles");
    styleName.appendChild(styleNameText);
    userStyle.appendChild(styleName);

    // 对每个选中的符号生成规则并添加到样式中
    for (QListWidgetItem* item : selectedItems) {
        QgsSymbol* symbol = (QgsSymbol*)item->data(Qt::UserRole).value<void*>();
        if (symbol) {
            QDomElement rule = doc.createElement("Rule");
            userStyle.appendChild(rule);

            // 设置符号颜色、大小等属性
            QgsFillSymbol* fillSymbol = dynamic_cast<QgsFillSymbol*>(symbol);
            if (fillSymbol) {
                // 对于填充符号，添加填充颜色
                QDomElement fillColor = doc.createElement("Fill");
                QDomElement color = doc.createElement("CssParameter");
                color.setAttribute("name", "fill");
                color.appendChild(doc.createTextNode(fillSymbol->color().name()));  // 使用符号的颜色
                fillColor.appendChild(color);
                rule.appendChild(fillColor);
            }

            QgsLineSymbol* lineSymbol = dynamic_cast<QgsLineSymbol*>(symbol);
            if (lineSymbol) {
                // 对于线符号，添加线宽和颜色
                QDomElement stroke = doc.createElement("Stroke");
                QDomElement strokeColor = doc.createElement("CssParameter");
                strokeColor.setAttribute("name", "stroke");
                strokeColor.appendChild(doc.createTextNode(lineSymbol->color().name()));  // 使用符号的颜色
                stroke.appendChild(strokeColor);
                QDomElement strokeWidth = doc.createElement("CssParameter");
                strokeWidth.setAttribute("name", "stroke-width");
                strokeWidth.appendChild(doc.createTextNode(QString::number(lineSymbol->width())));  // 使用符号的宽度
                stroke.appendChild(strokeWidth);
                rule.appendChild(stroke);
            }

            QgsMarkerSymbol* markerSymbol = dynamic_cast<QgsMarkerSymbol*>(symbol);
            if (markerSymbol) {
                // 对于标记符号，添加标记大小和颜色
                QDomElement pointSymbol = doc.createElement("PointSymbolizer");
                QDomElement marker = doc.createElement("Graphic");
                QDomElement size = doc.createElement("Size");
                size.appendChild(doc.createTextNode(QString::number(markerSymbol->size())));
                marker.appendChild(size);

                QDomElement markerColor = doc.createElement("Fill");
                QDomElement markerFill = doc.createElement("CssParameter");
                markerFill.setAttribute("name", "fill");
                markerFill.appendChild(doc.createTextNode(markerSymbol->color().name()));
                markerColor.appendChild(markerFill);
                marker.appendChild(markerColor);

                pointSymbol.appendChild(marker);
                rule.appendChild(pointSymbol);
            }
        }
    }

    // 保存到文件
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("导出失败"), QStringLiteral("无法打开文件进行写入！"));
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    doc.save(out, 4);  // 使用4个空格进行缩进

    file.close();

    QMessageBox::information(this, QStringLiteral("导出成功"), QStringLiteral("选中的符号已成功导出为 SLD 文件！"));
}

//选择图层
void SymbolLibManager::setCurrentLayer(QgsMapLayer* layer)
{
    m_curMapLayer = layer;
    // 同步comboBox，添加当前图层
    ui.LayerComboBox->clear();
    const auto layers = QgsProject::instance()->mapLayers();
    for (const auto& layer : layers) {
        if (layer->type() == Qgis::LayerType::Vector) {
            ui.LayerComboBox->addItem(layer->name());
        }
    }
}

// 应用按钮
void SymbolLibManager::on_applyButton_clicked()
{
    // 获取当前选中的符号
    QList<QListWidgetItem*> selectedItems = ui.listWidgetSymbols->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("应用失败"), QStringLiteral("请选择一个符号！"));
        return;
    }

    // 如果选中了多个符号，提醒用户
    if (selectedItems.size() > 1) {
        QMessageBox::warning(this, QStringLiteral("应用失败"), QStringLiteral("只能选择一个符号！"));
        return;
    }

    // 获取当前选中的符号
    QgsSymbol* symbol = (QgsSymbol*)selectedItems.first()->data(Qt::UserRole).value<void*>();

    if (!symbol || !m_curMapLayer) {
        QMessageBox::warning(this, QStringLiteral("应用失败"), QStringLiteral("符号或图层无效！"));
        return;
    }

    // 检查是否选择了多个图层
    if (ui.LayerComboBox->count() > 1 && ui.LayerComboBox->currentIndex() == -1) {
        QMessageBox::warning(this, QStringLiteral("应用失败"), QStringLiteral("请选择一个图层！"));
        return;
    }

    // 检查图层类型与符号类型是否匹配
    if (m_curMapLayer->type() == Qgis::LayerType::Vector) {
        QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(m_curMapLayer);

        // 判断符号类型
        if (dynamic_cast<QgsLineSymbol*>(symbol)) {
            if (vectorLayer->geometryType() != Qgis::GeometryType::Line) {
                QMessageBox::warning(this, QStringLiteral("应用失败"), QStringLiteral("选中的符号是线符号，当前图层不是线图层！"));
                return;
            }
        }
        else if (dynamic_cast<QgsFillSymbol*>(symbol)) {
            if (vectorLayer->geometryType() != Qgis::GeometryType::Polygon) {
                QMessageBox::warning(this, QStringLiteral("应用失败"), QStringLiteral("选中的符号是填充符号，当前图层不是面图层！"));
                return;
            }
        }
        else if (dynamic_cast<QgsMarkerSymbol*>(symbol)) {
            if (vectorLayer->geometryType() != Qgis::GeometryType::Point) {
                QMessageBox::warning(this, QStringLiteral("应用失败"), QStringLiteral("选中的符号是点符号，当前图层不是点图层！"));
                return;
            }
        }
        else {
            QMessageBox::warning(this, QStringLiteral("应用失败"), QStringLiteral("无法识别的符号类型！"));
            return;
        }

        // 如果符号与图层匹配，应用符号
        QgsSingleSymbolRenderer* renderer = new QgsSingleSymbolRenderer(symbol);
        vectorLayer->setRenderer(renderer);
        vectorLayer->triggerRepaint();  // 刷新图层显示
    }
}

//combobox改变
void SymbolLibManager::onLayerComboBoxIndexChanged(int index)
{
    // 获取用户在 LayerComboBox 中选中的图层名称
    QString layerName = ui.LayerComboBox->itemText(index);

    // 遍历所有图层，查找与名称匹配的图层
    const auto layers = QgsProject::instance()->mapLayers();
    for (const auto& layer : layers) {
        if (layer->name() == layerName) {
            // 将 m_curMapLayer 更新为选中的图层
            m_curMapLayer = layer;
            break;
        }
    }
}