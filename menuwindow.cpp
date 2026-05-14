#include "menuwindow.h"
#include "gamewidget.h"
#include <QFont>
#include <QKeyEvent>

MenuWindow::MenuWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setFixedSize(1200, 800);
    setWindowTitle("I Wanna 自制版 - 狱门疆领域");

    storyLine = 0;
    storyFinished = false;
    storyTexts << "【旁白】狱门疆之内，时空凝滞，无边死寂。"
              << "【咒灵】终于……等到你了。"
              << "【五条悟】哦？没想到这种地方，还藏着这么有意思的家伙。"
              << "【咒灵】吾乃由无数人沉沦I wanna所积攒的怨念化生之特技咒灵。"
              << "【咒灵】自我被封入狱门疆的那一刻，我便已展开领域。"
              << "【咒灵】借狱门疆隔绝外界的特性，沉眠至今，静候有人踏入。"
              << "【咒灵】来一场游戏吧。"
              << "【咒灵】束缚已然定下：你在此领域内封禁全部咒术与咒力。"
              << "【咒灵】而我，不得插手游戏分毫。"
              << "【咒灵】若你能抵达领域终点，便可借我曾经收集的天逆鉾之力冲破狱门疆。"
              << "【咒灵】若你不能……呵呵，那就献上一切换取我的自由！"
              << "【五条悟】有点意思啊……既然都找上门了，那我就陪你玩玩好了。"
              << "【系统】游戏开启，前路皆由脚下而行……";

    storyTimer = new QTimer(this);
    connect(storyTimer, &QTimer::timeout, this, &MenuWindow::updateStory);
    storyTimer->start(1500);

    btnStart = new QPushButton("踏入领域 · 奔赴狱门", this);
    btnStart->setFixedSize(260, 70);
    btnStart->hide();

    btnStart->setStyleSheet(R"(
        QPushButton{
            font-size:22px;
            font-weight:bold;
            background-color:#2b0808;
            color:#bb2222;
            border:3px solid #661111;
            border-radius:8px;
        }
        QPushButton:hover{
            background-color:#440c0c;
            color:#ff4444;
            border:3px solid #aa2222;
        }
        QPushButton:pressed{
            background-color:#1a0505;
            color:#ff6666;
        }
    )");

    connect(btnStart, &QPushButton::clicked, this, &MenuWindow::onStartGameClicked);
}

void MenuWindow::updateStory()
{
    if (storyFinished) return;

    storyLine++;
    if (storyLine >= storyTexts.size()) {
        storyTimer->stop();
        storyFinished = true;

        btnStart->move(470, 680);
        btnStart->show();
    }
    update();
}

void MenuWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);
    QPainter p(this);
    p.fillRect(rect(), QColor(5, 5, 18));
    p.setPen(QColor(200,200,220));
    p.setFont(QFont("Microsoft YaHei", 14));

    int y = 100;
    for (int i = 0; i <= storyLine && i < storyTexts.size(); i++) {
        p.drawText(70, y, storyTexts[i]);
        y += 36;
    }

    p.setPen(QColor(100,100,120));
    p.drawText(width() - 240, height() - 30, "按 ESC 跳过剧情");
}

void MenuWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        storyTimer->stop();
        storyFinished = true;
        storyLine = storyTexts.size() - 1;
        btnStart->move(470, 680);
        btnStart->show();
        update();
    }
    QMainWindow::keyPressEvent(event);
}

void MenuWindow::onStartGameClicked()
{
    this->close();
    GameWidget *game = new GameWidget();
    game->setWindowTitle("I Wanna 游戏界面");
    game->show();
}
