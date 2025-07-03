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
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMenuBar>
#include <QMenu>
#include <QDialog>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <iostream>
#include <vector>
#include <QString>

#include "happly.h"

// --- 設定ダイアログ ---
class ConfigDialog : public QDialog
{
public:
    ConfigDialog(const QVector3D& initialPos, const QVector3D& initialCenter, const QVector3D& initialUp, QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("初期視点の設定");
        QFormLayout *formLayout = new QFormLayout(this);

        // カメラ位置
        pos_x = createSpinBox(); pos_y = createSpinBox(); pos_z = createSpinBox();
        QHBoxLayout* posLayout = new QHBoxLayout;
        posLayout->addWidget(pos_x); posLayout->addWidget(pos_y); posLayout->addWidget(pos_z);
        formLayout->addRow("カメラ初期位置 (X, Y, Z):", posLayout);

        // 注視点
        center_x = createSpinBox(); center_y = createSpinBox(); center_z = createSpinBox();
        QHBoxLayout* centerLayout = new QHBoxLayout;
        centerLayout->addWidget(center_x); centerLayout->addWidget(center_y); centerLayout->addWidget(center_z);
        formLayout->addRow("注視点 初期位置 (X, Y, Z):", centerLayout);

        // Upベクトル
        up_x = createSpinBox(); up_y = createSpinBox(); up_z = createSpinBox();
        QHBoxLayout* upLayout = new QHBoxLayout;
        upLayout->addWidget(up_x); upLayout->addWidget(up_y); upLayout->addWidget(up_z);
        formLayout->addRow("Upベクトル (X, Y, Z):", upLayout);

        // 値を設定
        pos_x->setValue(initialPos.x()); pos_y->setValue(initialPos.y()); pos_z->setValue(initialPos.z());
        center_x->setValue(initialCenter.x()); center_y->setValue(initialCenter.y()); center_z->setValue(initialCenter.z());
        up_x->setValue(initialUp.x()); up_y->setValue(initialUp.y()); up_z->setValue(initialUp.z());

        // OK/Cancel ボタン
        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
        formLayout->addRow(buttonBox);

        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    QVector3D getCameraPosition() const { return QVector3D(pos_x->value(), pos_y->value(), pos_z->value()); }
    QVector3D getViewCenter() const { return QVector3D(center_x->value(), center_y->value(), center_z->value()); }
    QVector3D getUpVector() const { return QVector3D(up_x->value(), up_y->value(), up_z->value()).normalized(); }

private:
    QDoubleSpinBox* createSpinBox() {
        QDoubleSpinBox* spinBox = new QDoubleSpinBox;
        spinBox->setRange(-10000, 10000);
        spinBox->setDecimals(2);
        spinBox->setSingleStep(1.0);
        return spinBox;
    }
    QDoubleSpinBox *pos_x, *pos_y, *pos_z;
    QDoubleSpinBox *center_x, *center_y, *center_z;
    QDoubleSpinBox *up_x, *up_y, *up_z;
};


// 点群データを格納する構造体
struct Point {
    float x, y, z;
    unsigned char r, g, b;
    unsigned int u, v; // u, v座標を追加
};

class PointCloudWidget; // 前方宣言

// 視点コントロールパネル
class ViewControlPanel : public QWidget
{
    Q_OBJECT

public:
    ViewControlPanel(QWidget *parent = nullptr) : QWidget(parent) {
        QPushButton *frontButton = new QPushButton("正面");
        QPushButton *rightButton = new QPushButton("右");
        QPushButton *topButton = new QPushButton("上");

        QPushButton *yawLeftButton = new QPushButton("Yaw ◀");
        QPushButton *yawRightButton = new QPushButton("Yaw ▶");
        QPushButton *pitchUpButton = new QPushButton("Pitch ⏶");
        QPushButton *pitchDownButton = new QPushButton("Pitch ⏷");
        QPushButton *rollLeftButton = new QPushButton("Roll ⤿");
        QPushButton *rollRightButton = new QPushButton("Roll ⤾");

        QGridLayout *layout = new QGridLayout(this);
        // Direct views
        layout->addWidget(frontButton, 0, 1);
        layout->addWidget(rightButton, 1, 1);
        layout->addWidget(topButton, 2, 1);

        // Rotations
        layout->addWidget(yawLeftButton, 1, 0);
        layout->addWidget(yawRightButton, 1, 2);
        layout->addWidget(pitchUpButton, 0, 2);
        layout->addWidget(pitchDownButton, 2, 2);
        layout->addWidget(rollLeftButton, 0, 0);
        layout->addWidget(rollRightButton, 2, 0);

        layout->setSpacing(2);
        layout->setContentsMargins(5, 5, 5, 5);

        // スタイル設定
        setStyleSheet("QPushButton { background-color: rgba(70, 70, 70, 180); color: white; border: 1px solid #555; padding: 5px; min-width: 60px; } QPushButton:hover { background-color: rgba(90, 90, 90, 220); }");

        connect(frontButton, &QPushButton::clicked, this, &ViewControlPanel::frontViewRequested);
        connect(rightButton, &QPushButton::clicked, this, &ViewControlPanel::rightViewRequested);
        connect(topButton, &QPushButton::clicked, this, &ViewControlPanel::topViewRequested);

        float angle = 10.0f;
        connect(yawLeftButton, &QPushButton::clicked, this, [this, angle](){ emit yawRequested(angle); });
        connect(yawRightButton, &QPushButton::clicked, this, [this, angle](){ emit yawRequested(-angle); });
        connect(pitchUpButton, &QPushButton::clicked, this, [this, angle](){ emit pitchRequested(angle); });
        connect(pitchDownButton, &QPushButton::clicked, this, [this, angle](){ emit pitchRequested(-angle); });
        connect(rollLeftButton, &QPushButton::clicked, this, [this, angle](){ emit rollRequested(angle); });
        connect(rollRightButton, &QPushButton::clicked, this, [this, angle](){ emit rollRequested(-angle); });
    }

signals:
    void frontViewRequested();
    void rightViewRequested();
    void topViewRequested();
    void yawRequested(float angle);
    void pitchRequested(float angle);
    void rollRequested(float angle);
};


// 点群を描画するOpenGLウィジェット
class PointCloudWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    PointCloudWidget(QWidget *parent = nullptr) : QOpenGLWidget(parent) {}

    void setInitialCameraState(const QVector3D& pos, const QVector3D& center, const QVector3D& up) {
        initialCameraPosition = pos;
        initialViewCenter = center;
        initialUpVector = up;
    }

    // PLYファイルから点群をロードする
    void loadPly(const std::string& filepath) {
        try {
            happly::PLYData plyIn(filepath);
            std::vector<double> x = plyIn.getElement("vertex").getProperty<double>("x");
            std::vector<double> y = plyIn.getElement("vertex").getProperty<double>("y");
            std::vector<double> z = plyIn.getElement("vertex").getProperty<double>("z");
            std::vector<unsigned char> r, g, b;
            bool hasColor = false;
            try {
                r = plyIn.getElement("vertex").getProperty<unsigned char>("red");
                g = plyIn.getElement("vertex").getProperty<unsigned char>("green");
                b = plyIn.getElement("vertex").getProperty<unsigned char>("blue");
                hasColor = true;
            } catch (const std::exception&) {}
            std::vector<unsigned int> u, v;
            bool hasUV = false;
            try {
                u = plyIn.getElement("vertex").getProperty<unsigned int>("u");
                v = plyIn.getElement("vertex").getProperty<unsigned int>("v");
                hasUV = true;
            } catch (const std::exception&) {}

            points.clear();
            for (size_t i = 0; i < x.size(); ++i) {
                Point p;
                p.x = static_cast<float>(x[i]);
                p.y = static_cast<float>(y[i]);
                p.z = static_cast<float>(z[i]);
                p.r = hasColor ? r[i] : 255;
                p.g = hasColor ? g[i] : 255;
                p.b = hasColor ? b[i] : 255;
                p.u = hasUV ? u[i] : 0;
                p.v = hasUV ? v[i] : 0;
                points.push_back(p);
            }
            update();
        } catch (const std::exception& e) {
            std::cerr << "Error loading PLY file: " << e.what() << std::endl;
        }
    }

public slots:
    void resetView() {
        cameraPosition = initialCameraPosition;
        viewCenter = initialViewCenter;
        upVector = initialUpVector;
        requestUpdate();
    }
    void setFrontView() {
        float distance = cameraPosition.distanceToPoint(viewCenter);
        cameraPosition = viewCenter + QVector3D(0, 0, distance);
        upVector = QVector3D(0, 1, 0);
        requestUpdate();
    }
    void setRightView() {
        float distance = cameraPosition.distanceToPoint(viewCenter);
        cameraPosition = viewCenter + QVector3D(distance, 0, 0);
        upVector = QVector3D(0, 1, 0);
        requestUpdate();
    }
    void setTopView() {
        float distance = cameraPosition.distanceToPoint(viewCenter);
        cameraPosition = viewCenter + QVector3D(0, distance, 0.01f); // ジンバルロックを避けるため微小値を追加
        upVector = QVector3D(0, 0, -1);
        requestUpdate();
    }
    void rotateYaw(float angle) {
        QMatrix4x4 rotationMatrix;
        rotationMatrix.rotate(angle, upVector);
        cameraPosition = viewCenter + rotationMatrix.map(cameraPosition - viewCenter);
        requestUpdate();
    }
    void rotatePitch(float angle) {
        QVector3D viewDirection = (cameraPosition - viewCenter).normalized();
        QVector3D rightDirection = QVector3D::crossProduct(viewDirection, upVector).normalized();
        QMatrix4x4 rotationMatrix;
        rotationMatrix.rotate(angle, rightDirection);
        cameraPosition = viewCenter + rotationMatrix.map(cameraPosition - viewCenter);
        upVector = rotationMatrix.map(upVector).normalized();
        requestUpdate();
    }
    void rotateRoll(float angle) {
        QVector3D viewDirection = (cameraPosition - viewCenter).normalized();
        QMatrix4x4 rotationMatrix;
        rotationMatrix.rotate(angle, viewDirection);
        upVector = rotationMatrix.map(upVector).normalized();
        requestUpdate();
    }

signals:
    void cameraChanged(const QVector3D& pos, const QVector3D& center, const QVector3D& up);

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glPointSize(2.0f);
        resetView(); // 初期視点に設定
    }

    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);
    }

    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        QMatrix4x4 projection;
        projection.perspective(45.0f, float(width()) / float(height()), 0.1f, 10000.0f);
        glLoadMatrixf(projection.constData());
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        QMatrix4x4 view;
        view.lookAt(cameraPosition, viewCenter, upVector);
        glLoadMatrixf(view.constData());
        glBegin(GL_POINTS);
        for (const auto& p : points) {
            glColor3ub(p.r, p.g, p.b);
            glVertex3f(p.x, p.y, p.z);
        }
        glEnd();
    }

    void wheelEvent(QWheelEvent *event) override {
        float zoomFactor = 0.1f * cameraPosition.distanceToPoint(viewCenter);
        zoomFactor = std::max(1.0f, zoomFactor);
        float zoomAmount = event->angleDelta().y() / 120.0f * zoomFactor;
        QVector3D viewDirection = (cameraPosition - viewCenter).normalized();
        float distance = cameraPosition.distanceToPoint(viewCenter);
        if (distance - zoomAmount > 1.0f) {
            cameraPosition -= viewDirection * zoomAmount;
        }
        requestUpdate();
    }

    void mousePressEvent(QMouseEvent *event) override {
        lastPos = event->pos();
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        int dx = event->pos().x() - lastPos.x();
        int dy = event->pos().y() - lastPos.y();
        if (event->buttons() & Qt::RightButton) {
            float panSpeed = 0.002f * cameraPosition.distanceToPoint(viewCenter);
            QVector3D viewDirection = (viewCenter - cameraPosition).normalized();
            QVector3D rightDirection = QVector3D::crossProduct(viewDirection, upVector).normalized();
            QVector3D actualUpDirection = QVector3D::crossProduct(rightDirection, viewDirection).normalized();
            cameraPosition -= (rightDirection * dx * panSpeed);
            cameraPosition += (actualUpDirection * dy * panSpeed);
            viewCenter -= (rightDirection * dx * panSpeed);
            viewCenter += (actualUpDirection * dy * panSpeed);
        } else if (event->buttons() & Qt::LeftButton) {
            float rotationSpeed = 0.2f;
            QMatrix4x4 rotationMatrix;
            rotationMatrix.rotate(-dx * rotationSpeed, upVector);
            QVector3D viewDirection = (cameraPosition - viewCenter).normalized();
            QVector3D rightDirection = QVector3D::crossProduct(viewDirection, upVector).normalized();
            rotationMatrix.rotate(-dy * rotationSpeed, rightDirection);
            cameraPosition = viewCenter + rotationMatrix.map(cameraPosition - viewCenter);
        }
        lastPos = event->pos();
        requestUpdate();
    }

private:
    void requestUpdate() {
        update();
        emit cameraChanged(cameraPosition, viewCenter, upVector);
    }

    std::vector<Point> points;
    QVector3D cameraPosition;
    QVector3D viewCenter;
    QVector3D upVector;
    QVector3D initialCameraPosition;
    QVector3D initialViewCenter;
    QVector3D initialUpVector;
    QPoint lastPos;
};

// メインウィンドウクラス
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        // 初期値を設定
        initialCameraPosition = QVector3D(0, 0, 0);
        initialViewCenter = QVector3D(1, 1, 1);
        initialUpVector = QVector3D(0, -1, 0);

        // --- UIのセットアップ ---
        setupMenuBar();
        QWidget *centralWidget = new QWidget;
        setCentralWidget(centralWidget);
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
        QHBoxLayout *buttonLayout = new QHBoxLayout;
        QPushButton *loadImageButton = new QPushButton("画像を開く...");
        QPushButton *loadPlyButton = new QPushButton("点群(PLY)を開く...");
        QPushButton *resetViewButton = new QPushButton("視点をリセット");
        buttonLayout->addWidget(loadImageButton);
        buttonLayout->addWidget(loadPlyButton);
        buttonLayout->addStretch();
        buttonLayout->addWidget(resetViewButton);
        mainLayout->addLayout(buttonLayout);
        QSplitter *splitter = new QSplitter(this);
        mainLayout->addWidget(splitter);
        imageLabel = new QLabel("「画像を開く...」ボタンでファイルを選択してください。");
        imageLabel->setAlignment(Qt::AlignCenter);
        splitter->addWidget(imageLabel);

        // --- 点群ビューアとコントロールパネルのコンテナ ---
        QWidget *pointCloudContainer = new QWidget;
        QGridLayout *pointCloudLayout = new QGridLayout(pointCloudContainer);
        pointCloudLayout->setContentsMargins(0,0,0,0);
        pointCloudWidget = new PointCloudWidget;
        pointCloudWidget->setInitialCameraState(initialCameraPosition, initialViewCenter, initialUpVector);
        ViewControlPanel *viewPanel = new ViewControlPanel;

        // カメラ情報表示ラベル
        cameraInfoLabel = new QLabel;
        cameraInfoLabel->setStyleSheet("background-color: rgba(0, 0, 0, 150); color: white; padding: 5px; border-radius: 3px;");
        cameraInfoLabel->setAlignment(Qt::AlignRight);

        pointCloudLayout->addWidget(pointCloudWidget, 0, 0);
        pointCloudLayout->addWidget(viewPanel, 0, 0, Qt::AlignTop | Qt::AlignRight);
        pointCloudLayout->addWidget(cameraInfoLabel, 0, 0, Qt::AlignBottom | Qt::AlignRight); // 右下に配置
        splitter->addWidget(pointCloudContainer);

        splitter->setSizes({400, 600});

        connect(loadImageButton, &QPushButton::clicked, this, &MainWindow::loadImage);
        connect(loadPlyButton, &QPushButton::clicked, this, &MainWindow::loadPointCloud);
        connect(resetViewButton, &QPushButton::clicked, pointCloudWidget, &PointCloudWidget::resetView);

        // --- シグナル/スロット接続 ---
        connect(viewPanel, &ViewControlPanel::frontViewRequested, pointCloudWidget, &PointCloudWidget::setFrontView);
        connect(viewPanel, &ViewControlPanel::rightViewRequested, pointCloudWidget, &PointCloudWidget::setRightView);
        connect(viewPanel, &ViewControlPanel::topViewRequested, pointCloudWidget, &PointCloudWidget::setTopView);
        connect(viewPanel, &ViewControlPanel::yawRequested, pointCloudWidget, &PointCloudWidget::rotateYaw);
        connect(viewPanel, &ViewControlPanel::pitchRequested, pointCloudWidget, &PointCloudWidget::rotatePitch);
        connect(viewPanel, &ViewControlPanel::rollRequested, pointCloudWidget, &PointCloudWidget::rotateRoll);

        // カメラ位置が変更されたらウィンドウタイトルと情報ラベルを更新
        connect(pointCloudWidget, &PointCloudWidget::cameraChanged, this, &MainWindow::updateWindowTitle);
        connect(pointCloudWidget, &PointCloudWidget::cameraChanged, this, &MainWindow::updateCameraInfoLabel);

        resize(1000, 650);
        updateWindowTitle(initialCameraPosition, initialViewCenter, initialUpVector); // 初回タイトル設定
        updateCameraInfoLabel(initialCameraPosition, initialViewCenter, initialUpVector); // 初回ラベル設定
    }

private:
    void setupMenuBar() {
        QMenu *fileMenu = menuBar()->addMenu("ファイル");
        QAction *exitAction = new QAction("終了", this);
        connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
        fileMenu->addAction(exitAction);

        QMenu *settingsMenu = menuBar()->addMenu("設定");
        QAction *configAction = new QAction("初期視点を設定...", this);
        connect(configAction, &QAction::triggered, this, &MainWindow::openConfigDialog);
        settingsMenu->addAction(configAction);
    }

private slots:
    void openConfigDialog() {
        ConfigDialog dialog(initialCameraPosition, initialViewCenter, initialUpVector, this);
        if (dialog.exec() == QDialog::Accepted) {
            initialCameraPosition = dialog.getCameraPosition();
            initialViewCenter = dialog.getViewCenter();
            initialUpVector = dialog.getUpVector();
            pointCloudWidget->setInitialCameraState(initialCameraPosition, initialViewCenter, initialUpVector);
            pointCloudWidget->resetView(); // 設定を即時反映
        }
    }

    void loadImage() {
        QString filePath = QFileDialog::getOpenFileName(this, "画像ファイルを開く", "/app/data", "画像ファイル (*.png *.jpg *.jpeg *.bmp)");
        if (!filePath.isEmpty()) {
            QPixmap pixmap(filePath);
            if(pixmap.isNull()){
                imageLabel->setText("エラー: 画像を読み込めませんでした。");
            } else {
                imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }
    }

    void loadPointCloud() {
        QString filePath = QFileDialog::getOpenFileName(this, "PLYファイルを開く", "/app/data", "PLYファイル (*.ply)");
        if (!filePath.isEmpty()) {
            pointCloudWidget->loadPly(filePath.toStdString());
        }
    }

    void updateWindowTitle(const QVector3D& pos, const QVector3D& center, const QVector3D& up) {
        QString title = QString("カメラ位置: (%1, %2, %3) | 注視点: (%4, %5, %6) | Up: (%7, %8, %9)")
            .arg(pos.x(), 0, 'f', 1)
            .arg(pos.y(), 0, 'f', 1)
            .arg(pos.z(), 0, 'f', 1)
            .arg(center.x(), 0, 'f', 1)
            .arg(center.y(), 0, 'f', 1)
            .arg(center.z(), 0, 'f', 1)
            .arg(up.x(), 0, 'f', 2)
            .arg(up.y(), 0, 'f', 2)
            .arg(up.z(), 0, 'f', 2);
        setWindowTitle(title);
    }

    void updateCameraInfoLabel(const QVector3D& pos, const QVector3D& center, const QVector3D& up) {
        QString text = QString("Pos: (%1, %2, %3)\nCenter: (%4, %5, %6)\nUp: (%7, %8, %9)")
            .arg(pos.x(), 0, 'f', 1)
            .arg(pos.y(), 0, 'f', 1)
            .arg(pos.z(), 0, 'f', 1)
            .arg(center.x(), 0, 'f', 1)
            .arg(center.y(), 0, 'f', 1)
            .arg(center.z(), 0, 'f', 1)
            .arg(up.x(), 0, 'f', 2)
            .arg(up.y(), 0, 'f', 2)
            .arg(up.z(), 0, 'f', 2);
        cameraInfoLabel->setText(text);
    }

private:
    QLabel *imageLabel;
    PointCloudWidget *pointCloudWidget;
    QLabel *cameraInfoLabel; // 情報表示用ラベル
    QVector3D initialCameraPosition;
    QVector3D initialViewCenter;
    QVector3D initialUpVector;
};

#include "main.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}
