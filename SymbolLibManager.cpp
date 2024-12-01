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
    setWindowTitle(QStringLiteral("���Ź�����"));

    setStyle();

    ui.listWidgetSymbols->setSelectionMode(QAbstractItemView::MultiSelection);

    // ��ʼ�� mStyle
    mStyle = QgsStyle::defaultStyle();  // ʹ��Ĭ����ʽ
     // ���� LayerComboBox ���źŲۣ�ѡ�б仯ʱ���� m_curMapLayer
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

//���밴ť
void SymbolLibManager::on_btnImport_clicked()
{
    // ѡ��Ҫ������ļ���֧�ֶ��ָ�ʽ
    QStringList fileNames = QFileDialog::getOpenFileNames(this, QStringLiteral("������ſ����ʽ�ļ�"), "",
        QStringLiteral("���ſ��ļ� (*.xml);;SLD��ʽ�ļ� (*.sld)"));

    if (fileNames.isEmpty()) {
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // �����ļ�����ѡ���뷽ʽ
    for (const QString& fileName : fileNames) {
        QFileInfo fileInfo(fileName);
        QString fileExtension = fileInfo.suffix().toLower();  // ��ȡ�ļ���չ��

        if (fileExtension == "xml") {
            // ������ſ�
            importSymbolLibrary(fileName);
        }
        else if (fileExtension == "sld") {
            // ����SLD��ʽ�ļ�
            importSLD(fileName);
        }
        else {
            QMessageBox::warning(this, QStringLiteral("�ļ���ʽ��֧��"), QStringLiteral("��ǰֻ֧�ֵ��� .xml �� .sld �ļ�"));
        }
    }

    QApplication::restoreOverrideCursor();
}

// ���� XML ��ʽ�ļ�
void SymbolLibManager::importSymbolLibrary(const QString& fileName)
{
    bool status = mStyle->importXml(fileName);
    if (!status) {
        QFileInfo file(fileName);
        QMessageBox::warning(this, QStringLiteral("������ſ�"), QStringLiteral("������ſ� '%1' ʧ�ܣ�").arg(file.baseName()));
        return;
    }

    // ��ȡ���з�������
    QStringList symbolNames = mStyle->allNames(QgsStyle::StyleEntity::SymbolEntity);
    for (const QString& symbolName : symbolNames) {
        // ���ݷ������ƻ�ȡ����
        QgsSymbol* symbol = mStyle->symbol(symbolName);
        if (symbol) {
            // ��ȡ���ŵ�Ԥ��ͼ��
            QSize iconSize(32, 32); // ����Ԥ��ͼ��Ĵ�С
            QImage previewImage = symbol->asImage(iconSize);

            // ������Ŵ��ڣ����ɱ�ǩ�����ƣ�
            QString symbolLabel = symbolName; // Ĭ��ʹ�÷���������Ϊ��ǩ

            // ���� QListWidgetItem
            QListWidgetItem* item = new QListWidgetItem(symbolLabel);

            // �洢���Ŷ����Ԥ��ͼ��
            item->setData(Qt::UserRole, QVariant::fromValue((void*)symbol)); // �洢���Ŷ���
            item->setData(Qt::DecorationRole, previewImage); // ���÷��ŵ�Ԥ��ͼ��

            // ������ӵ������б���
            ui.listWidgetSymbols->addItem(item);
        }
    }
}

// ���� SLD ��ʽ�ļ�
void SymbolLibManager::importSLD(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("����ʧ��"), QStringLiteral("�޷��� SLD �ļ� '%1'").arg(fileName));
        return;
    }

    // ���� SLD �ļ�
    QDomDocument doc;
    if (!doc.setContent(&file)) {
        QMessageBox::warning(this, QStringLiteral("����ʧ��"), QStringLiteral("�޷����� SLD �ļ�"));
        return;
    }

    file.close();

    // ��ȡ SLD �ļ��еĹ��򲢵������
    QDomElement sld = doc.documentElement();
    QDomNodeList namedLayers = sld.elementsByTagName("NamedLayer");

    for (int i = 0; i < namedLayers.count(); ++i) {
        QDomElement namedLayer = namedLayers.at(i).toElement();
        QDomNodeList userStyles = namedLayer.elementsByTagName("UserStyle");

        for (int j = 0; j < userStyles.count(); ++j) {
            QDomElement userStyle = userStyles.at(j).toElement();

            // �� UserStyle �л�ȡ����Rule��
            QDomNodeList rules = userStyle.elementsByTagName("Rule");
            for (int k = 0; k < rules.count(); ++k) {
                QDomElement rule = rules.at(k).toElement();

                // �� Rule �л�ȡ����
                QString ruleName = rule.attribute("Name");
                if (ruleName.isEmpty()) {
                    // ���û�� Name ���ԣ�����ʹ��Ĭ������
                    ruleName = QString("Rule %1").arg(k);  // ʹ�ù����������Ϊ����
                }

                // �������Ų��� SLD ����������ʽ
                QgsSymbol* symbol = createSymbolFromSLD(rule);
                if (symbol) {
                    // ��ȡ���ŵ�Ԥ��ͼ��
                    QSize iconSize(32, 32); // ����Ԥ��ͼ��Ĵ�С
                    QImage previewImage = symbol->asImage(iconSize);

                    // ���� QListWidgetItem �����÷�������
                    QListWidgetItem* item = new QListWidgetItem(ruleName);

                    // �洢���Ŷ����Ԥ��ͼ��
                    item->setData(Qt::UserRole, QVariant::fromValue((void*)symbol)); // �洢���Ŷ���
                    item->setData(Qt::DecorationRole, previewImage); // ���÷��ŵ�Ԥ��ͼ��

                    // ������ӵ������б���
                    ui.listWidgetSymbols->addItem(item);
                }
            }
        }
    }
}

// �� SLD �ļ��� Rule Ԫ�ش�������
QgsSymbol* SymbolLibManager::createSymbolFromSLD(const QDomElement& ruleElement)
{
    QgsSymbol* symbol = nullptr;

    // ����������
    QDomNodeList fills = ruleElement.elementsByTagName("Fill");
    if (!fills.isEmpty()) {
        symbol = QgsFillSymbol::createSimple(QMap<QString, QVariant>{
            {"color", QColor(fills.at(0).toElement().text()).name()}
        });
    }

    // ������������
    QDomNodeList strokes = ruleElement.elementsByTagName("Stroke");
    if (!strokes.isEmpty()) {
        if (!symbol) {
            symbol = QgsLineSymbol::createSimple(QMap<QString, QVariant>{
                {"color", QColor(strokes.at(0).toElement().text()).name()}
            });
        }
        else {
            // ����Ѿ��������ţ��������
            QgsLineSymbol* lineSymbol = dynamic_cast<QgsLineSymbol*>(symbol);
            if (lineSymbol) {
                lineSymbol->setColor(QColor(strokes.at(0).toElement().text()));
            }
        }
    }

    // ������Ƿ��ţ��������ţ�
    QDomNodeList points = ruleElement.elementsByTagName("PointSymbolizer");
    if (!points.isEmpty()) {
        // ����һ��Ĭ�ϵı�Ƿ��ţ�����Բ�α�ǣ�
        QgsMarkerSymbol* markerSymbol = QgsMarkerSymbol::createSimple(QMap<QString, QVariant>{
            {"color", QColor(points.at(0).toElement().text()).name()},
            { "size", 6 }  // Ĭ�ϴ�СΪ 6
        });

        // ����б����ɫ��������ɫ
        QDomNodeList fillColor = points.at(0).toElement().elementsByTagName("Fill");
        if (!fillColor.isEmpty()) {
            markerSymbol->setColor(QColor(fillColor.at(0).toElement().text()));
        }

        symbol = markerSymbol;
    }

    // ����ͼ�η��ţ���ͼ����Ż��Զ�����ţ�
    QDomNodeList graphics = ruleElement.elementsByTagName("Graphic");
    if (!graphics.isEmpty()) {
        QDomElement graphicElement = graphics.at(0).toElement();

        // ���� ExternalGraphic��SVG��
        QDomNodeList externalGraphics = graphicElement.elementsByTagName("ExternalGraphic");
        if (!externalGraphics.isEmpty()) {
            QDomElement externalGraphicElement = externalGraphics.at(0).toElement();
            QString svgFilePath = externalGraphicElement.attribute("href");

            if (!svgFilePath.isEmpty()) {
                // ����һ��SVG��Ƿ���
                QgsMarkerSymbol* svgMarkerSymbol = QgsMarkerSymbol::createSimple(QMap<QString, QVariant>{
                    {"size", 16},  // Ĭ�ϴ�С
                    { "file", svgFilePath }  // ����SVG�ļ�·��
                });
                symbol = svgMarkerSymbol;
            }
        }
        else {
            // ���û�� ExternalGraphic����� Mark������Ρ�Բ�εȣ�
            QDomNodeList marks = graphicElement.elementsByTagName("Mark");
            if (!marks.isEmpty()) {
                QDomElement markElement = marks.at(0).toElement();
                QString markName = markElement.attribute("wellKnownName");

                if (markName == "circle") {
                    // ����һ���򵥵�Բ�α��
                    QgsMarkerSymbol* circleMarker = QgsMarkerSymbol::createSimple(QMap<QString, QVariant>{
                        {"color", "black"},  // ������ɫΪ��ɫ
                        { "size", 8 }  // Ĭ�ϴ�С
                    });
                    symbol = circleMarker;
                }
                else if (markName == "square") {
                    // ����һ���򵥵ķ��α��
                    QgsMarkerSymbol* squareMarker = QgsMarkerSymbol::createSimple(QMap<QString, QVariant>{
                        {"color", "black"},  // ������ɫΪ��ɫ
                        { "size", 8 }  // Ĭ�ϴ�С
                    });
                    symbol = squareMarker;
                }
            }
        }
    }
    return symbol;

}

//����ΪSLD��ʽ
void SymbolLibManager::on_btnExportSLD_clicked()
{
    // ��ȡ��ǰѡ�еĶ������
    QList<QListWidgetItem*> selectedItems = ui.listWidgetSymbols->selectedItems();

    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("����ʧ��"), QStringLiteral("����ѡ��Ҫ�����ķ��ţ�"));
        return;
    }

    // �����ļ�����Ի���
    QString fileName = QFileDialog::getSaveFileName(this, QStringLiteral("����ΪSLD�ļ�"), "", QStringLiteral("SLD��ʽ�ļ� (*.sld)"));

    if (fileName.isEmpty()) {
        return;  // �û�ȡ���˱���
    }

    // ���� XML �ĵ�
    QDomDocument doc;
    QDomElement sld = doc.createElement("StyledLayerDescriptor");
    sld.setAttribute("version", "1.0.0");
    doc.appendChild(sld);

    // ����ѡ�еķ��ţ�����ÿ�����ŵ���ʽ
    QDomElement namedLayer = doc.createElement("NamedLayer");
    sld.appendChild(namedLayer);

    QDomElement layerName = doc.createElement("Name");
    QDomText layerNameText = doc.createTextNode("Symbols Layer");
    layerName.appendChild(layerNameText);
    namedLayer.appendChild(layerName);

    // ������ʽԪ��
    QDomElement userStyle = doc.createElement("UserStyle");
    namedLayer.appendChild(userStyle);

    QDomElement styleName = doc.createElement("Name");
    QDomText styleNameText = doc.createTextNode("Combined Symbol Styles");
    styleName.appendChild(styleNameText);
    userStyle.appendChild(styleName);

    // ��ÿ��ѡ�еķ������ɹ�����ӵ���ʽ��
    for (QListWidgetItem* item : selectedItems) {
        QgsSymbol* symbol = (QgsSymbol*)item->data(Qt::UserRole).value<void*>();
        if (symbol) {
            QDomElement rule = doc.createElement("Rule");
            userStyle.appendChild(rule);

            // ���÷�����ɫ����С������
            QgsFillSymbol* fillSymbol = dynamic_cast<QgsFillSymbol*>(symbol);
            if (fillSymbol) {
                // ���������ţ���������ɫ
                QDomElement fillColor = doc.createElement("Fill");
                QDomElement color = doc.createElement("CssParameter");
                color.setAttribute("name", "fill");
                color.appendChild(doc.createTextNode(fillSymbol->color().name()));  // ʹ�÷��ŵ���ɫ
                fillColor.appendChild(color);
                rule.appendChild(fillColor);
            }

            QgsLineSymbol* lineSymbol = dynamic_cast<QgsLineSymbol*>(symbol);
            if (lineSymbol) {
                // �����߷��ţ�����߿����ɫ
                QDomElement stroke = doc.createElement("Stroke");
                QDomElement strokeColor = doc.createElement("CssParameter");
                strokeColor.setAttribute("name", "stroke");
                strokeColor.appendChild(doc.createTextNode(lineSymbol->color().name()));  // ʹ�÷��ŵ���ɫ
                stroke.appendChild(strokeColor);
                QDomElement strokeWidth = doc.createElement("CssParameter");
                strokeWidth.setAttribute("name", "stroke-width");
                strokeWidth.appendChild(doc.createTextNode(QString::number(lineSymbol->width())));  // ʹ�÷��ŵĿ��
                stroke.appendChild(strokeWidth);
                rule.appendChild(stroke);
            }

            QgsMarkerSymbol* markerSymbol = dynamic_cast<QgsMarkerSymbol*>(symbol);
            if (markerSymbol) {
                // ���ڱ�Ƿ��ţ���ӱ�Ǵ�С����ɫ
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

    // ���浽�ļ�
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("����ʧ��"), QStringLiteral("�޷����ļ�����д�룡"));
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    doc.save(out, 4);  // ʹ��4���ո��������

    file.close();

    QMessageBox::information(this, QStringLiteral("�����ɹ�"), QStringLiteral("ѡ�еķ����ѳɹ�����Ϊ SLD �ļ���"));
}

//ѡ��ͼ��
void SymbolLibManager::setCurrentLayer(QgsMapLayer* layer)
{
    m_curMapLayer = layer;
    // ͬ��comboBox����ӵ�ǰͼ��
    ui.LayerComboBox->clear();
    const auto layers = QgsProject::instance()->mapLayers();
    for (const auto& layer : layers) {
        if (layer->type() == Qgis::LayerType::Vector) {
            ui.LayerComboBox->addItem(layer->name());
        }
    }
}

// Ӧ�ð�ť
void SymbolLibManager::on_applyButton_clicked()
{
    // ��ȡ��ǰѡ�еķ���
    QList<QListWidgetItem*> selectedItems = ui.listWidgetSymbols->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Ӧ��ʧ��"), QStringLiteral("��ѡ��һ�����ţ�"));
        return;
    }

    // ���ѡ���˶�����ţ������û�
    if (selectedItems.size() > 1) {
        QMessageBox::warning(this, QStringLiteral("Ӧ��ʧ��"), QStringLiteral("ֻ��ѡ��һ�����ţ�"));
        return;
    }

    // ��ȡ��ǰѡ�еķ���
    QgsSymbol* symbol = (QgsSymbol*)selectedItems.first()->data(Qt::UserRole).value<void*>();

    if (!symbol || !m_curMapLayer) {
        QMessageBox::warning(this, QStringLiteral("Ӧ��ʧ��"), QStringLiteral("���Ż�ͼ����Ч��"));
        return;
    }

    // ����Ƿ�ѡ���˶��ͼ��
    if (ui.LayerComboBox->count() > 1 && ui.LayerComboBox->currentIndex() == -1) {
        QMessageBox::warning(this, QStringLiteral("Ӧ��ʧ��"), QStringLiteral("��ѡ��һ��ͼ�㣡"));
        return;
    }

    // ���ͼ����������������Ƿ�ƥ��
    if (m_curMapLayer->type() == Qgis::LayerType::Vector) {
        QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(m_curMapLayer);

        // �жϷ�������
        if (dynamic_cast<QgsLineSymbol*>(symbol)) {
            if (vectorLayer->geometryType() != Qgis::GeometryType::Line) {
                QMessageBox::warning(this, QStringLiteral("Ӧ��ʧ��"), QStringLiteral("ѡ�еķ������߷��ţ���ǰͼ�㲻����ͼ�㣡"));
                return;
            }
        }
        else if (dynamic_cast<QgsFillSymbol*>(symbol)) {
            if (vectorLayer->geometryType() != Qgis::GeometryType::Polygon) {
                QMessageBox::warning(this, QStringLiteral("Ӧ��ʧ��"), QStringLiteral("ѡ�еķ����������ţ���ǰͼ�㲻����ͼ�㣡"));
                return;
            }
        }
        else if (dynamic_cast<QgsMarkerSymbol*>(symbol)) {
            if (vectorLayer->geometryType() != Qgis::GeometryType::Point) {
                QMessageBox::warning(this, QStringLiteral("Ӧ��ʧ��"), QStringLiteral("ѡ�еķ����ǵ���ţ���ǰͼ�㲻�ǵ�ͼ�㣡"));
                return;
            }
        }
        else {
            QMessageBox::warning(this, QStringLiteral("Ӧ��ʧ��"), QStringLiteral("�޷�ʶ��ķ������ͣ�"));
            return;
        }

        // ���������ͼ��ƥ�䣬Ӧ�÷���
        QgsSingleSymbolRenderer* renderer = new QgsSingleSymbolRenderer(symbol);
        vectorLayer->setRenderer(renderer);
        vectorLayer->triggerRepaint();  // ˢ��ͼ����ʾ
    }
}

//combobox�ı�
void SymbolLibManager::onLayerComboBoxIndexChanged(int index)
{
    // ��ȡ�û��� LayerComboBox ��ѡ�е�ͼ������
    QString layerName = ui.LayerComboBox->itemText(index);

    // ��������ͼ�㣬����������ƥ���ͼ��
    const auto layers = QgsProject::instance()->mapLayers();
    for (const auto& layer : layers) {
        if (layer->name() == layerName) {
            // �� m_curMapLayer ����Ϊѡ�е�ͼ��
            m_curMapLayer = layer;
            break;
        }
    }
}