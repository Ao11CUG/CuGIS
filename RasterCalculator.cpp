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
    setWindowTitle(QStringLiteral("栅格计算器"));
    setStyle();
    QgsApplication::setPrefixPath("D:\\OSGeo4w\\apps\\qgis-ltr", true); // 设置 QGIS 路径
    QgsApplication::init();
    QgsApplication::initQgis();
    initializeProcessing();  // 激活注册算法
    ui.rasterLayerlistView->setModel(rasterLayerModel);

    updateListview();

    connect(m_mapCanvas, &QgsMapCanvas::extentsChanged, this, &RasterCalculator::updateListview);

    // 双击rasterLayerlistView中的名称添加到计算表达式中
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
    // 遍历当前QgsProject中的所有栅格图层，并加入listView
    for (const QgsMapLayer* layer : QgsProject::instance()->mapLayers())
    {
        if (layer->type() == Qgis::LayerType::Raster) {
            const QgsRasterLayer* rasterLayer = dynamic_cast<const QgsRasterLayer*>(layer);
            if (rasterLayer && rasterLayer->isValid()) {
                m_rasterLayerNames.append(rasterLayer->name()); // 添加有效的栅格图层名称
            }
        }
    }

    // 更新 listView 中的图层名称
    ui.rasterLayerlistView->setModel(new QStringListModel(m_rasterLayerNames, this));  // 更新模型数据
}

// 选择栅格图层
void RasterCalculator::on_selectRasterLayersButton_clicked() {

    // 打开文件对话框选择栅格图层
    QStringList rasterFilePaths = QFileDialog::getOpenFileNames(this, QStringLiteral("选择栅格文件"), "", tr("GeoTIFF Files (*.tif);;所有文件 (*)"));
    if (!rasterFilePaths.isEmpty()) {
        m_rasterLayerNames.clear(); // 清空已有的名称
        m_rasterLayers.clear();     // 清空已有的图层对象

        for (const QString& filePath : rasterFilePaths) {
            QFileInfo fileInfo(filePath);
            if (fileInfo.exists()) {
                QString layerBaseName = fileInfo.baseName(); // 获取文件名（不包括路径）
                QgsRasterLayer* rasterLayer = new QgsRasterLayer(filePath, layerBaseName);

                if (rasterLayer->isValid()) {
                    // 获取波段数目
                    int bandCount = rasterLayer->bandCount();

                    // 只保存图层名称，不附加波段号
                    m_rasterLayerNames.append(layerBaseName);
                    m_rasterLayers.append(rasterLayer);  // 将有效的图层添加到列表中
                }
                else {
                    QMessageBox::information(this, QStringLiteral("加载失败"), QStringLiteral("图层: ") + layerBaseName + QStringLiteral(" 无效"));
                }
            }
        }

        // 更新 listView 中的图层名称
        ui.rasterLayerlistView->setModel(new QStringListModel(m_rasterLayerNames, this));  // 更新模型数据
    }
}

// 双击 rasterLayerlistView 中的栅格名称，将其添加到 expressionLineEdit 中
void RasterCalculator::on_rasterLayerlistView_doubleClicked(const QModelIndex& index) {
    QString layerName = ui.rasterLayerlistView->model()->data(index).toString();

    // 获取该图层的波段数
    QgsRasterLayer* layer = nullptr;
    for (QgsRasterLayer* rasterLayer : m_rasterLayers) {
        if (rasterLayer->name() == layerName) {
            layer = rasterLayer;
            break;
        }
    }

    if (layer) {
        int bandCount = layer->bandCount();
        QString bandString = (bandCount > 0) ? QStringLiteral("@1") : ""; // 如果有多个波段，默认选择第1波段

        // 获取当前表达式
        QString currentExpression = ui.expressionLineEdit->text();

        // 检查当前表达式末尾是否已经包含该图层名称
        QString newLayerName = "\"" + layerName + bandString + "\"";
        if (currentExpression.isEmpty() || !currentExpression.endsWith(newLayerName)) {
            // 在现有表达式后追加图层名称和波段，不清空已有内容
            QString newExpression = currentExpression + (currentExpression.isEmpty() ? "" : " ") + newLayerName;
            ui.expressionLineEdit->setText(newExpression);  // 更新表达式
        }
    }
}

// 选择输出文件路径
void RasterCalculator::on_selectOutputFileButton_clicked() {
    m_outputFilePath = QFileDialog::getSaveFileName(this, QStringLiteral("选择输出文件"), "", QStringLiteral("GeoTIFF Files (*.tif);;所有文件 (*)"));
    if (!m_outputFilePath.isEmpty()) {
        ui.outputFileLineEdit->setText(m_outputFilePath); // 设置输出文件路径
    }
}

//进行栅格计算
void RasterCalculator::on_calculateButton_clicked() {
    // 获取表达式
    QString expression = ui.expressionLineEdit->text();

    // 检查必填项
    if (expression.isEmpty() || m_outputFilePath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请确保输入表达式和输出路径"));
        return;
    }

    // 获取项目中的所有图层
    const auto layers = QgsProject::instance()->mapLayers();
    QList<QgsRasterLayer*> inputLayers;

    for (const auto& layer : layers) {
        if (layer->type() == Qgis::LayerType::Raster) {
            QgsRasterLayer* rasterLayer = qobject_cast<QgsRasterLayer*>(layer);
            if (rasterLayer && rasterLayer->isValid()) {
                // 如果是有效的栅格图层，添加到输入图层列表中
                inputLayers.append(rasterLayer);
            }
        }
    }

    if (inputLayers.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("没有有效的栅格图层"));
        return;
    }

    // 设置栅格计算的参数
    QgsProcessingFeedback feedback;                              //反馈信息
    QgsProcessingContext context;                                //包含了算法在执行期间可能需要的各种资源和设置
    QVariantMap parameters;                                      //参数

    // 获取图层路径列表
    QStringList layerPaths;
    for (QgsRasterLayer* layer : inputLayers) {
        layerPaths.append(layer->dataProvider()->dataSourceUri());  // 获取图层的路径
    }

    parameters.insert("LAYERS", layerPaths);
    parameters.insert("EXPRESSION", expression);  // 设置表达式
    parameters.insert("OUTPUT", m_outputFilePath);  // 设置输出路径

    // 获取栅格计算算法
    QgsProcessingAlgorithm* alg = QgsApplication::processingRegistry()->createAlgorithmById("native:rastercalc");
    if (alg) {
        // 尝试准备算法
        if (alg->prepare(parameters, context, &feedback)) {
            qDebug() << "Algorithm prepared successfully.";
            bool ok = false;
            QVariantMap result = alg->run(parameters, context, &feedback, &ok);
            if (ok) {
                QMessageBox::information(this, QStringLiteral("计算完成"), QStringLiteral("栅格计算成功，输出文件: ") + m_outputFilePath);



                //将得到的结果栅格添加到QGIS项目中

                // 创建新的栅格图层
                QgsRasterLayer* resultLayer = new QgsRasterLayer(m_outputFilePath, QStringLiteral("计算结果"));
                // 检查栅格图层是否有效
                if (resultLayer->isValid()) {
                    // 将新的栅格图层添加到 resultLayers 容器中
                    m_rasterLayers.append(resultLayer);

                    // 将栅格图层添加到 QGIS 项目中
                    QgsProject::instance()->addMapLayer(resultLayer);
                }




            }
            else {
                // 输出反馈
                QMessageBox::warning(this, QStringLiteral("计算失败"), QStringLiteral("栅格计算失败，日志: ") + feedback.textLog());
            }
        }
        else {

            QMessageBox::warning(this, QStringLiteral("算法失败"), QStringLiteral("无法执行栅格计算，错误日志: ") + feedback.textLog());
        }
    }
    else {
        QMessageBox::warning(this, QStringLiteral("算法失败"), QStringLiteral("无法获取栅格计算算法"));
    }
}

// 添加加法运算符
void RasterCalculator::on_pushButton_add_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("+"));
}

// 添加减法运算符
void RasterCalculator::on_pushButton_subtract_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("-"));
}

// 添加乘法运算符
void RasterCalculator::on_pushButton_multiply_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("*"));
}

// 添加除法运算符
void RasterCalculator::on_pushButton_divide_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("/"));
}

// 添加对数运算（log10）
void RasterCalculator::on_pushButton_log10_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("log10("));
}

// 添加自然对数（ln）
void RasterCalculator::on_pushButton_ln_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("ln("));
}

// 添加指数运算(^)
void RasterCalculator::on_pushButton_index_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("^"));
}

// 添加条件运算
void RasterCalculator::on_pushButton_if_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("if("));
}

// 添加左括号 '('
void RasterCalculator::on_pushButton_openBracket_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("("));
}

// 添加右括号 ')'
void RasterCalculator::on_pushButton_closeBracket_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral(")"));
}

// 添加等于符号 '='
void RasterCalculator::on_pushButton_equal_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("="));
}

// 添加不等于符号 '!='
void RasterCalculator::on_pushButton_notEqual_clicked() {
    ui.expressionLineEdit->insert(QStringLiteral("!="));
}
