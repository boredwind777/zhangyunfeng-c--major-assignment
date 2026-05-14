#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QWidget>
#include <QKeyEvent>
#include <QTimer>
#include <QPainter>
#include <QTime>
#include <QVector>
#include <QPolygon>
#include <QRectF>

// ===================== 游戏常量 =====================
#define PLAYER_SIZE 30
#define MOVE_SPEED 240.0
#define JUMP_POWER -400.0
#define GRAVITY 1200.0
#define SPIKE_SIZE 20
#define GAME_WIDTH 1200
#define GAME_HEIGHT 800
#define MAP_WIDTH 1500

// ===================== 行为组件 =====================
struct MoveHoriz {
    int left, right;
    double speed;
    int dir;
    double deltaX;

    MoveHoriz(int l, int r, double sp);
    void update(double &x, double w, double dt);
};

struct MoveVert {
    int top, bottom;
    double speed;
    int dir;

    MoveVert(int t, int b, double sp);
    void update(double &y, double h, double dt);
};

struct Disappear {
    bool visible;
    Disappear(bool initVis);
};



// ===================== 游戏物体基类 =====================
class GameObject
{
public:
    double x, y, w, h;

    MoveHoriz* moveHoriz = nullptr;
    MoveVert* moveVert = nullptr;
    Disappear* disappear = nullptr;

    bool isTrap = false;
    QRectF triggerArea;
    bool triggered = false;
    double trapKeepTime = 3.0;
    double trapTimer = 0.0;
    double triggerTimer = 0.0;
    double needStaySec = 1.0;
    bool oneShot = true;
    bool trapdestroyed = false;

    bool isFallTrap = false;
    bool fallTriggered = false;
    double fallTimer = 0.0;
    double fallRecoverTime = 2.5;

    bool customHideMode = false;
    double showTime = 2.0;
    double hideTime = 2.0;
    double customTimer = 0.0;

    bool hiddenBoard = false;
    bool boardRevealed = false;

    bool isFlySpike = false;
    int flyDir = 1;
    double flySpeed = 350;
    bool flyBack = false;
    double flyBackInterval = 1.5;

    QRectF flyTriggerRange;
    bool flyDone = false;
    bool flylaunched = false;
    bool isLocked = false;
    double flyLeftBound = 0.0;
    double flyRightBound = MAP_WIDTH;

public:
    GameObject(double x_, double y_, double w_, double h_);
    virtual ~GameObject() = default;

    virtual void update(double dt) final;
    virtual void onUpdate(double dt);
    virtual void draw(QPainter &p, double camerax) = 0;

    QRectF rect() const;
    bool isVisible() const;
};

class Platform;
class Spike;

// ===================== 玩家类 =====================
class GameWidget;
class Player
{
public:
    bool isDying = false;
private:
    double x, y, vx, vy;
    double platformspeedx;
    int jumpCount;
    const int maxJump = 2;
    bool jumppressedlast;
    bool onGround;
    int animFrame = 0;    // 动画帧
    double animTimer = 0; // 计时
     const int walkFrames = 2;
     int faceDir = 1;
public:
    void init();
    void move(double dt);
    void draw(QPainter &p, double camerax);
    void input(bool A, bool D, bool W);
    QRectF rect() const;
     void respawn();
    bool checkCollisions(QVector<GameObject*>& objs, double dt);

    double getx() const;
    double gety() const;
    double setx(double a){x=a;}
        double sety(double b){y=b;}
    double setvx(double a);
    double setvy(double b);
    double getvx();
    double getvy();
    bool getonground();
    void setOnGround(bool on) {
            onGround = on;
        }
        void setJumpCount(int count) {
            jumpCount = count;
        }
};

// ===================== 游戏窗口 =====================
class GameWidget : public QWidget
{
    Q_OBJECT
private:
    Player player;
    QTimer timer;
    QTime lastTime;
    bool keyA, keyD, keyW;
    QVector<GameObject*> objs;
    double camerax;
    bool isDying = false;
    double dyingTimer = 0.0;
    const double DYING_DELAY = 0.6;
    int deathCount = 0;
    bool isWin = false;
    bool showTip = false;
    double tipX=0, tipY=0;
    bool isMenu = false;
    bool isPause = false;
    int pauseSel = 0;
    bool isConfirmStart = true;
    bool isQuitSad = false;
        QImage bgImage;

public:
    double saveX = 100;        // 存档X坐标（默认出生点）
    double saveY = GAME_HEIGHT - PLAYER_SIZE;  // 存档Y坐标
    bool hasSavePoint = false; // 是否激活过存档

    explicit GameWidget(QWidget *p = nullptr);
    ~GameWidget() override;
    void resetAllTraps();
private:
    void initGame();
    void drawGame(QPainter &p);

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
};
//===============存档================
class SavePoint : public GameObject
{
public:
    using GameObject::GameObject;
    void draw(QPainter &p, double camerax) override;
     bool saved = false;
};


#endif
