#pragma once

#include <QgsRubberBand.h>
#include <qgsmapmouseevent.h>
#include <qgsmaptool.h>
#include <qgspointxy.h>
#include <qgsmapcanvas.h>
#include <QPainter>
#include <QgsCircularString.h>
#include <QFileInfo>
#include <QStandardItem>
#include <QstringLiteral>

class DrawGeometryTool : public QgsMapTool
{
	Q_OBJECT

public:
	enum DrawShape { Rectangle, Circle, Polygon };

	explicit DrawGeometryTool(QgsMapCanvas* canvas, DrawShape shape)
		: QgsMapTool(canvas), mShape(shape), mDrawing(false), mIsPolygonFinished(false) {}

	void canvasPressEvent(QgsMapMouseEvent* event) override {
		if (mShape == Polygon) {
			if (event->button() == Qt::LeftButton) {
				mPolygonPoints.append(event->mapPoint());
			}
			else if (event->button() == Qt::RightButton) {
				mIsPolygonFinished = true;
				emit geometryDrawn(createGeometry());
				canvas()->refresh();
			}
		}
		else if (event->button() == Qt::LeftButton) {
			mStartPoint = event->mapPoint();
			mDrawing = true;
		}
	}

	void canvasMoveEvent(QgsMapMouseEvent* event) override {
		if (mDrawing && (mShape == Rectangle || mShape == Circle)) {
			mCurrentPoint = event->mapPoint();
			canvas()->refresh();
		}
	}

	void canvasReleaseEvent(QgsMapMouseEvent* event) override {
		if (mDrawing && event->button() == Qt::LeftButton && (mShape == Rectangle || mShape == Circle)) {
			mDrawing = false;
			mCurrentPoint = event->mapPoint();
			emit geometryDrawn(createGeometry());
			canvas()->refresh();
		}
	}

	void draw(QPainter* painter) {
		if (!mDrawing && !mIsPolygonFinished) return;

		QPen pen(Qt::red, 2);
		painter->setPen(pen);

		if (mShape == Rectangle) {
			QRectF rect(mStartPoint.x(), mStartPoint.y(),
				mCurrentPoint.x() - mStartPoint.x(),
				mCurrentPoint.y() - mStartPoint.y());
			painter->drawRect(rect);
		}
		else if (mShape == Circle) {
			double radius = sqrt(pow(mCurrentPoint.x() - mStartPoint.x(), 2) +
				pow(mCurrentPoint.y() - mStartPoint.y(), 2));
			painter->drawEllipse(QPointF(mStartPoint.x(), mStartPoint.y()), radius, radius);
		}
		else if (mShape == Polygon) {
			if (!mPolygonPoints.isEmpty()) {
				for (int i = 1; i < mPolygonPoints.size(); ++i) {
					painter->drawLine(mPolygonPoints[i - 1].toQPointF(), mPolygonPoints[i].toQPointF());
				}
			}
		}
	}

signals:
	void geometryDrawn(const QgsGeometry& geometry);

private:
	QgsGeometry createGeometry() const
	{
		if (mShape == Circle) {
			QgsPointXY center(mStartPoint.x(), mStartPoint.y());
			double radius = sqrt(pow(mCurrentPoint.x() - mStartPoint.x(), 2) +
				pow(mCurrentPoint.y() - mStartPoint.y(), 2));

			//     Բ εĶ    б 
			QgsPolylineXY ring;  // ʹ   QgsPolylineXY     QList<QgsPointXY>
			int segments = 64;   //  ֶ     Խ  ԲԽƽ  
			for (int i = 0; i <= segments; ++i) {
				double angle = (2.0 * M_PI * i) / segments;
				double x = center.x() + radius * cos(angle);
				double y = center.y() + radius * sin(angle);
				ring.append(QgsPointXY(x, y));
			}

			// ȷ  Բ   Ǳպϵ 
			if (ring.first() != ring.last()) {
				ring.append(ring.first());
			}

			//         
			QVector<QgsPolylineXY> polygon;
			polygon.append(ring);

			//    ؼ   
			return QgsGeometry::fromPolygonXY(polygon);
		}
		else if (mShape == Polygon) {
			if (mPolygonPoints.size() < 3) {
				return QgsGeometry();  //            3   򷵻ؿռ   
			}

			//         λ 
			QgsPolylineXY ring(mPolygonPoints);
			if (ring.first() != ring.last()) {
				ring.append(ring.first());  //  պ϶    
			}

			//         
			QVector<QgsPolylineXY> polygon;
			polygon.append(ring);

			return QgsGeometry::fromPolygonXY(polygon);
		}
		else if (mShape == Rectangle) {
			QgsRectangle rect(mStartPoint.x(), mStartPoint.y(), mCurrentPoint.x(), mCurrentPoint.y());
			return QgsGeometry::fromRect(rect);
		}

		return QgsGeometry();
	}


	DrawShape mShape;
	bool mDrawing;
	bool mIsPolygonFinished;
	QgsPointXY mStartPoint;
	QgsPointXY mCurrentPoint;
	QVector<QgsPointXY> mPolygonPoints;
}; 
