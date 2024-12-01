#include "RasterCalculator.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QgsProcessing.h>
#include <QgsApplication.h>
#include <QgsProject.h>
#include <QgsRasterLayer.h>
#include <QgsCoordinateReferenceSystem.h>
#include <QgsProcessingAlgorithm.h>
#include <QgsProcessingContext.h>
#include <QgsProcessingFeedback.h>
#include <QgsProcessingRegistry.h>
#include <qgsnativealgorithms.h>
#include <QgsRasterCalculator.h>
#include <QDebug>
#include <QListView>


RasterCalculator::RasterCalculator(QgsMapCanvas* m_mapCanvas, QWidget* parent)
    : QMainWindow(parent), m_mapCanvas(m_mapCanvas),
    rasterLayerModel(new QStringListModel(this))
{
    ui.setupUi(this);
    setWindowIcon(QIcon(":/MainInterface/res/rasterCalculator.png"));
    setWindowTitle(QStringLiteral("դ�������"));
    setStyle();
    QgsApplication::setPrefixPath("D:\\OSGeo4w\\apps\\qgis-ltr", true); // ���� QGIS ·��
    QgsApplication::init();
    QgsApplication::initQgis();
    initializeProcessing();  // ����ע���㷨
    ui.rasterLayerlistView->setModel(rasterLayerModel);

    updateListview();

    connect(m_mapCanvas, &QgsMapCanvas::extentsChanged, this, &RasterCalculator::updateListview);

    // ˫��rasterLayerlistView�е�������ӵ�������ʽ��
    connect(ui.rasterLayerlistView, &QListView::doubleClicked, this, &RasterCalculator::on_rasterLayerlistView_doubleClicked);
}

RasterCalculator::~RasterCalculator()
{
    QgsApplication::exitQgis();
}

void RasterCalculator::setStyle() {
    style.setConfirmButton(ui.selectRasterLayersButton);

    style.setConfirmButton(ui.calculateButton);

    style.setCalculatorButton(ui.pushButton_add);

    style.setCalculatorButton(ui.pushButton_closeBracket);

    style.setCalculatorButton(ui.pushButton_divide);

    style.setCalculatorButton(ui.pushButton_equal);

    style.setCalculatorButton(ui.pushButton_if);

    style.setCalculatorButton(ui.pushButton_index);

    style.setCalculatorButton(ui.pushButton_ln);

    style.setCalculatorButton(ui.pushButton_log10);

    style.setCalculatorButton(ui.pushButton_multiply);

    style.setCalculatorButton(ui.pushButton_notEqual);

    style.setCalculatorButton(ui.pushButton_openBracket);

    style.setCalculatorButton(ui.pushButton_subtract);

    style.setCommonButton(ui.selectOutputFileButton);

    style.setLineEdit(ui.outputFileLineEdit);

    style.setLineEdit(ui.expressionLineEdit);
}

void RasterCalculator::initializeProcessing()
{
    QgsApplication::processingRegistry()->addProvider(new QgsNativeAlgorithms());
}

void RasterCalculator::updateListview() {
    // ������ǰQgsProject�е�����դ��ͼ�㣬������listView
    for (const QgsMapLayer* layer : QgsProject::instance()->mapLayers())
    {
        if (layer->type() == Qgis::LayerType::Raster) {
            const QgsRasterLayer* rasterLayer = dynamic_cast<const QgsRasterLayer*>(layer);
            if (rasterLayer && rasterLayer->isValid()) {
                m_rasterLayerNames.append(rasterLayer->name()); // �����Ч��դ��ͼ������
            }
        }
    }

    // ���� listView �е�ͼ������
    ui.rasterLayerlistView->setModel(new QStringListModel(m_rasterLayerNames, this));  // ����ģ������
}

// ѡ��դ��ͼ��
void RasterCalculator::on_selectRasterLayersButton_clicked() {

    // ���ļ��Ի���ѡ��դ��ͼ��
    QStringList rasterFilePaths = QFileDialog::getOpenFileNames(this, QStringLiteral("ѡ��դ���ļ�"), "", tr("GeoTIFF Files (*.tif);;�����ļ� (*)"));
    if (!rasterFilePaths.isEmpty()) {
        m_rasterLayerNames.clear(); // ������е�����
        m_rasterLayers.clear();     // ������е�ͼ�����

        for (const QString& filePath : rasterFilePaths) {
            QFileInfo fileInfo(filePath);
            if (fileInfo.exists()) {
                QString layerBaseName = fileInfo.baseName(); // ��ȡ�ļ�����������·����
                QgsRasterLayer* rasterLayer = new QgsRasterLayer(filePath, layerBaseName);

                if (rasterLayer->isValid()) {
                    // ��ȡ������Ŀ
                    int bandCount = rasterLayer->bandCount();

                    // ֻ����ͼ�����ƣ������Ӳ��κ�
                    m_rasterLayerNames.append(layerBaseName);
                    m_rasterLayers.append(rasterLayer);  // ����Ч��ͼ����ӵ��б���
                }
                else {
                    QMessageBox::information(this, QStringLiteral("����ʧ��"), QStringLiteral("ͼ��: ") + layerBaseName + QStringLiteral(" ��Ч"));
                }
            }
        }

        // ���� listView �е�ͼ������
        ui.rasterLayerlistView->setModel(new QStringListModel(m_rasterLayerNames, this));  // ����ģ������
    }
}

// ˫�� rasterLayerlistView �е�դ�����ƣ�������ӵ� expressionLineEdit ��
void RasterCalculator::on_rasterLayerlistView_doubleClicked(const QModelIndex& index) {
    QString layerName = ui.rasterLayerlistView->model()->data(index).toString();

    // ��ȡ��ͼ��Ĳ�����
    QgsRasterLayer* layer = nullptr;
    for (QgsRasterLayer* rasterLayer : m_rasterLayers) {
        if (rasterLayer->name() == layerName) {
            layer = rasterLayer;
            break;
        }
    }

    if (layer) {
        int bandCount = layer->bandCount();
        QString bandString = (bandCount > 0) ? QStringLiteral("@1") : ""; // ����ж�����Σ�Ĭ��ѡ���1����

        // ��ȡ��ǰ���ʽ
        QString currentExpression = ui.expressionLineEdit->text();

        // ��鵱ǰ���ʽĩβ�Ƿ��Ѿ�������ͼ������
        QString newLayerName = "\"" + layerName + bandString + "\"";
        if (currentExpression.isEmpty() || !currentExpression.endsWith(newLayerName)) {
            // �����б��ʽ��׷��ͼ�����ƺͲ��Σ��������������
            QString newExpression = currentExpression + (currentExpression.isEmpty() ? "" : " ") + newLayerName;
            ui.expressionLineEdit->setText(newExpression);  // ���±��ʽ
        }
    }
}

// ѡ������ļ�·��
void RasterCalculator::on_selectOutputFileButton_clicked() {
    m_outputFilePath = QFileDialog::getSaveFileName(this, QStringLiteral("ѡ������ļ�"), "", QStringLiteral("GeoTIFF Files (*.tif);;�����ļ� (*)"));
    if (!m_outputFilePath.isEmpty()) {
        ui.outputFileLineEdit->setText(m_outputFilePath); // ��������ļ�·��
    }
}

//����դ�����
void RasterCalculator::on_calculateButton_clicked() {
    // ��ȡ���ʽ
    QString expression = ui.expressionLineEdit->text();

    // ��������
    if (expression.isEmpty() || m_outputFilePath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("����"), QStringLiteral("��ȷ��������ʽ�����·��"));
        return;
    }

    // ��ȡ��Ŀ�е�����ͼ��
    const auto layers = QgsProject::instance()->mapLayers();
    QList<QgsRasterLayer*> inputLayers;

    for (const auto& layer : layers) {
        if (layer->type() == Qgis::LayerType::Raster) {
            QgsRasterLayer* rasterLayer = qobject_cast<QgsRasterLayer*>(layer);
            if (rasterLayer && rasterLayer->isValid()) {
                // �������Ч��դ��ͼ�㣬��ӵ�����ͼ���б���
                inputLayers.append(rasterLayer);
            }
        }
    }

    if (inputLayers.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("����"), QStringLiteral("û����Ч��դ��ͼ��"));
        return;
    }

    // ����դ�����Ĳ���
    QgsProcessingFeedback feedback;                              //������Ϣ
    QgsProcessingContext context;                                //�������㷨��ִ���ڼ������Ҫ�ĸ�����Դ������
    QVariantMap parameters;                                      //����

    // ��ȡͼ��·���б�
    QStringList layerPaths;
    for (QgsRasterLayer* layer : inputLayers) {
        layerPaths.append(layer->dataProvider()->dataSourceUri());  // ��ȡͼ���·��
    }

    parameters.insert("LAYERS", layerPaths);
    parameters.insert("EXPRESSION", expression);  // ���ñ��ʽ
    parameters.insert("OUTPUT", m_outputFilePath);  // �������·��

    // ��ȡդ������㷨
    QgsProcessingAlgorithm* alg = QgsApplication::processingRegistry()->createAlgorithmById("native:rastercalc");
    if (alg) {
        // ����׼���㷨
        if (alg->prepare(parameters, context, &feedback)) {
            qDebug() << "Algorithm prepared successfully.";
            bool ok = false;
            QVariantMap result = alg->run(parameters, context, &feedback, &ok);
            if (ok) {
                QMessageBox::information(this, QStringLiteral("�������"), QStringLiteral("դ�����ɹ�������ļ�: ") + m_outputFilePath);



                //���õ��Ľ��դ����ӵ�QGIS��Ŀ��

                // �����µ�դ��ͼ��
                QgsRasterLayer* resultLayer = new QgsRasterLayer(m_outputFilePath, QStringLiteral("������"));
                // ���դ��ͼ���Ƿ���Ч
                if (resultLayer->isValid()) {
                    // ���µ�դ��ͼ����ӵ� resultLayers ������
                    m_rasterLayers.append(resultLayer);

                    // ��դ��ͼ����ӵ� QGIS ��Ŀ��
                    QgsProject::instance()->addMapLayer(resultLayer);
                }




            }
            else {
                // �������
                QMessageBox::warning(this, QStringLiteral("����ʧ��"), QStringLiteral("դ�����ʧ�ܣ���־: ") + feedback.textLog());
            }
        }
        else {

            QMessageBox::warning(this, QStringLiteral("�㷨ʧ��"), QStringLiteral("�޷�ִ��դ����㣬������־: ") + feedback.textLog());
        }
    }
    else {
        QMessageBox::warning(this, QStringLiteral("�㷨ʧ��"), QStringLiteral("�޷���ȡդ������㷨"));
    }
}

// ��Ӽӷ������
void RasterCalculator::on_pushButton_add_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("+"));
}

// ��Ӽ��������
void RasterCalculator::on_pushButton_subtract_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("-"));
}

// ��ӳ˷������
void RasterCalculator::on_pushButton_multiply_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("*"));
}

// ��ӳ��������
void RasterCalculator::on_pushButton_divide_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("/"));
}

// ��Ӷ������㣨log10��
void RasterCalculator::on_pushButton_log10_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("log10("));
}

// �����Ȼ������ln��
void RasterCalculator::on_pushButton_ln_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("ln("));
}

// ���ָ������(^)
void RasterCalculator::on_pushButton_index_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("^"));
}

// �����������
void RasterCalculator::on_pushButton_if_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("if("));
}

// ��������� '('
void RasterCalculator::on_pushButton_openBracket_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("("));
}

// ��������� ')'
void RasterCalculator::on_pushButton_closeBracket_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral(")"));
}

// ��ӵ��ڷ��� '='
void RasterCalculator::on_pushButton_equal_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("="));
}

// ��Ӳ����ڷ��� '!='
void RasterCalculator::on_pushButton_notEqual_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("!="));
}
