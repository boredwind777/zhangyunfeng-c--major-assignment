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
#define PLAYER_SIZE 40
#define MOVE_SPEED 300.0
#define JUMP_POWER -500.0
#define GRAVITY 1600.0
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

    MoveHoriz(int l, int r, double sp)
        : left(l), right(r), speed(sp), dir(1), deltaX(0.0) {}

    void update(double &x, double w, double dt) {
        deltaX = dir * speed * dt;
        x += deltaX;
        if (x <= left) dir = 1;
        if (x + w >= right) dir = -1;
    }
};

struct MoveVert {
    int top, bottom;
    double speed;
    int dir;

    MoveVert(int t, int b, double sp)
        : top(t), bottom(b), speed(sp), dir(1) {}

    void update(double &y, double h, double dt) {
        y += dir * speed * dt;
        if (y <= top) dir = 1;
        if (y + h >= bottom) dir = -1;
    }
};

struct Disappear {
    bool visible;
    Disappear(bool initVis) : visible(initVis) {}
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
    double flyBackTimer = 0.0;
    QRectF flyTriggerRange;
    bool flyDone = false;
    bool flyLaunched = false;
    double waitBackTimer = 0.0;
    bool isLocked = false;

public:
    GameObject(double x_, double y_, double w_, double h_)
        : x(x_), y(y_), w(w_), h(h_) ,isFallTrap(false),
          fallTriggered(false), fallTimer(0.0)
    {
        triggerArea = rect();
        hiddenBoard = false;
        boardRevealed = false;
    }

    virtual ~GameObject() = default;

    virtual void update(double dt) final
    {
        if(isFlySpike)
           {
               if(moveHoriz) moveHoriz->update(x,w,dt);
               if(moveVert) moveVert->update(y,h,dt);
           }
           else
           {
               if (!isTrap || triggered|| isFlySpike)
               {
                   if (moveHoriz) moveHoriz->update(x, w, dt);
                   if (moveVert) moveVert->update(y, h, dt);
               }
           }

        if (isFallTrap && fallTriggered)
        {
            fallTimer += dt;
            if (fallTimer >= fallRecoverTime)
            {
                fallTriggered = false;
                fallTimer = 0.0;
                if(disappear) disappear->visible = true;
            }
        }

        if (!isFlySpike && isTrap && triggered) {
            trapTimer += dt;
            if (trapTimer >= trapKeepTime) {
                if (disappear) disappear->visible = false;
                if (oneShot) {
                    trapdestroyed = true;
                } else {
                    triggered = false;
                    trapTimer = 0.0;
                }
            }
        }

        if (customHideMode && !isFallTrap && !isTrap)
        {
            customTimer += dt;
            if(disappear->visible)
            {
                if(customTimer >= showTime)
                {
                    disappear->visible = false;
                    customTimer = 0.0;
                }
            }
            else
            {
                if(customTimer >= hideTime)
                {
                    disappear->visible = true;
                    customTimer = 0.0;
                }
            }
        }

        // ======== 飞行尖刺逻辑 ========
        if (isFlySpike && triggered && !isLocked)
        {
            // 第一波：向左正常飞出
            if (!flyLaunched)
            {
                x -= flySpeed * dt;
                // 飞出一段距离后，进入等待回马枪状态
                if (x <= 400)
                {
                    flyLaunched = true;
                }
            }
            else
            {
                // 玩家还在平台上，才开始倒计时
                // 计时3秒后折返
                waitBackTimer += dt;
                if (waitBackTimer >= 3.0)
                {
                    // 时间到，回马枪向右飞
                    x += flySpeed * dt;

                    // 飞回出生原点，锁定本局不再触发
                    if (x >= 710)
                    {
                        isLocked = true;
                        triggered = false;
                        flyLaunched = false;
                        waitBackTimer = 0.0;
                    }
                }
            }
        }

        onUpdate(dt);
    }

    virtual void onUpdate(double dt) {}
    virtual void draw(QPainter &p, double camerax) = 0;

    QRectF rect() const { return {x, y, w, h}; }
    bool isVisible() const {
        if (!disappear) return true;
        return disappear->visible;
    }
};

class Platform;
class Spike;

// ===================== 玩家类 =====================
class Player
{
public:
    bool isDying = false;
private:
    double x, y, vx, vy;
    double platformspeedx;
    bool onGround;
    int jumpCount;
    const int maxJump = 2;
    bool jumppressedlast;

public:
    void init();
    void move(double dt);
    void draw(QPainter &p, double camerax);
    void input(bool A, bool D, bool W);
    QRectF rect() const;
    void respawn();
    bool checkCollisions(QVector<GameObject*>& objs, double dt);

    double getx() const { return x; }
    double gety() const { return y; }
    double setvx(double a)  { vx=a;return vx; }
    double setvy(double b)  { vy=b;return vy; }
    double getvx()  { return vx; }
    double getvy()  { return vy; }
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

public:
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

#endif
