#include <QApplication>
#include <QMainWindow>
#include <QSplitter>
#include <QLabel>
#include <QPixmap>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QVector3D>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <iostream>
#include <vector>

#include "happly.h"

// 点群データを格納する構造体
struct Point {
    float x, y, z;
    unsigned char r, g, b;
    unsigned int u, v; // u, v座標を追加
};

// 点群を描画するOpenGLウィジェット
class PointCloudWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
    PointCloudWidget(QWidget *parent = nullptr) : QOpenGLWidget(parent) {}

    // PLYファイルから点群をロードする
    void loadPly(const std::string& filepath) {
        try {
            happly::PLYData plyIn(filepath);

            // 頂点データを取得
            std::vector<double> x = plyIn.getElement("vertex").getProperty<double>("x");
            std::vector<double> y = plyIn.getElement("vertex").getProperty<double>("y");
            std::vector<double> z = plyIn.getElement("vertex").getProperty<double>("z");

            // 色情報を取得 (存在すれば)
            std::vector<unsigned char> r, g, b;
            bool hasColor = false;
            try {
                r = plyIn.getElement("vertex").getProperty<unsigned char>("red");
                g = plyIn.getElement("vertex").getProperty<unsigned char>("green");
                b = plyIn.getElement("vertex").getProperty<unsigned char>("blue");
                hasColor = true;
            } catch (const std::exception& e) {
                std::cout << "Info: Point cloud does not contain color information." << std::endl;
            }

            // u, v 座標を取得 (存在すれば)
            std::vector<unsigned int> u, v;
            bool hasUV = false;
            try {
                u = plyIn.getElement("vertex").getProperty<unsigned int>("u");
                v = plyIn.getElement("vertex").getProperty<unsigned int>("v");
                hasUV = true;
            } catch (const std::exception& e) {
                std::cout << "Info: Point cloud does not contain u,v coordinate information." << std::endl;
            }


            points.clear();
            for (size_t i = 0; i < x.size(); ++i) {
                Point p;
                p.x = static_cast<float>(x[i]);
                p.y = static_cast<float>(y[i]);
                p.z = static_cast<float>(z[i]);

                if (hasColor) {
                    p.r = r[i];
                    p.g = g[i];
                    p.b = b[i];
                } else {
                    p.r = 255; p.g = 255; p.b = 255; // デフォルトは白
                }

                if (hasUV) {
                    p.u = u[i];
                    p.v = v[i];
                } else {
                    p.u = 0; p.v = 0;
                }
                points.push_back(p);
            }

            // 読み込んだ最初の点の情報を確認のために出力
            if (!points.empty()){
                const auto& firstPoint = points.front();
                std::cout << "First point loaded: "
                          << "x=" << firstPoint.x << ", y=" << firstPoint.y << ", z=" << firstPoint.z
                          << ", r=" << (int)firstPoint.r << ", g=" << (int)firstPoint.g << ", b=" << (int)firstPoint.b
                          << ", u=" << firstPoint.u << ", v=" << firstPoint.v << std::endl;
            }

            update(); // 再描画をトリガー
        } catch (const std::exception& e) {
            std::cerr << "Error loading PLY file: " << e.what() << std::endl;
        }
    }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f); // 背景を濃い青に
        glEnable(GL_DEPTH_TEST);
        glPointSize(2.0f); // 点のサイズを設定
    }

    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);
    }

    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        projection.setToIdentity();
        projection.perspective(45.0f, float(width()) / float(height()), 0.1f, 10000.0f); // far planeを調整
        glLoadMatrixf(projection.constData());

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        QMatrix4x4 view;
        view.translate(0.0, 0.0, -zoom); // ズーム
        view.rotate(rotation); // 回転
        glLoadMatrixf(view.constData());

        // 点群を描画
        glBegin(GL_POINTS);
        for (const auto& p : points) {
            glColor3ub(p.r, p.g, p.b);
            glVertex3f(p.x, p.y, p.z);
        }
        glEnd();
    }

    // マウスホイールでズーム
    void wheelEvent(QWheelEvent *event) override {
        zoom -= event->angleDelta().y() / 120.0f * (zoom * 0.1f); // ズーム量を調整
        if (zoom < 0.1f) zoom = 0.1f;
        update();
    }

    // マウスドラッグで回転
    void mousePressEvent(QMouseEvent *event) override {
        lastPos = event->pos();
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        int dx = event->pos().x() - lastPos.x();
        int dy = event->pos().y() - lastPos.y();

        if (event->buttons() & Qt::LeftButton) {
            QQuaternion q = QQuaternion::fromAxisAndAngle(dy, 1.0, 0.0, 0.0) *
                          QQuaternion::fromAxisAndAngle(dx, 0.0, 1.0, 0.0);
            rotation = q * rotation;
            update();
        }
        lastPos = event->pos();
    }

private:
    std::vector<Point> points;
    QMatrix4x4 projection;
    QQuaternion rotation;
    float zoom = 500.0f; // 初期ズーム値を調整
    QPoint lastPos;
};


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // ファイルパスの指定
    const std::string image_path = "/app/data/sample.png";
    const std::string ply_path = "/app/data/sample.ply";

    // --- UIのセットアップ ---
    QMainWindow mainWindow;
    QSplitter *splitter = new QSplitter(&mainWindow);
    mainWindow.setCentralWidget(splitter);

    // 左側の画像ウィジェット
    QLabel *imageLabel = new QLabel;
    QPixmap pixmap(QString::fromStdString(image_path));
    if(pixmap.isNull()){
        imageLabel->setText("Error: Cannot load image file.\nPlease check the path: " + QString::fromStdString(image_path));
    } else {
        imageLabel->setPixmap(pixmap.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    imageLabel->setAlignment(Qt::AlignCenter);
    splitter->addWidget(imageLabel);

    // 右側の点群ウィジェット
    PointCloudWidget *pointCloudWidget = new PointCloudWidget;
    pointCloudWidget->loadPly(ply_path);
    splitter->addWidget(pointCloudWidget);

    splitter->setSizes({400, 600}); // 初期サイズ

    mainWindow.setWindowTitle("Image and Point Cloud Viewer");
    mainWindow.resize(1000, 600);
    mainWindow.show();

    return app.exec();
}
