#ifndef MENUWINDOW_H
#define MENUWINDOW_H
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>
#include <QPainter>

class MenuWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MenuWindow(QWidget *parent = nullptr);

protected:

    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
private slots:
    void onStartGameClicked();
    void updateStory();

private:
    QPushButton *btnStart;
    QTimer *storyTimer;

    int storyLine;
    QStringList storyTexts;
    bool storyFinished;
};

#endif // MENUWINDOW_H
