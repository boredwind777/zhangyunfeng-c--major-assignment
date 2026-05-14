#include "gamewidget.h"
#include <QApplication>
#include <QMessageBox>
#include<menuwindow.h>
#include<qdebug.h>
#include<QImage>
// ===================== 存档点 =====================
void SavePoint::draw(QPainter &p, double camerax)
{
    if (!isVisible()) return;
    // 未存档：青色；已存档：绿色
    if(saved)
        p.setBrush(Qt::red);
    else
        p.setBrush(Qt::cyan);

    p.setPen(Qt::white);
    p.drawEllipse(x - camerax, y, w, h);
}

// ===================== 组件实现 =====================
MoveHoriz::MoveHoriz(int l, int r, double sp)
    : left(l), right(r), speed(sp), dir(1), deltaX(0.0) {}

void MoveHoriz::update(double &x, double w, double dt) {
    deltaX = dir * speed * dt;
    x += deltaX;
    if (x <= left) dir = 1;
    if (x + w >= right) dir = -1;
}

MoveVert::MoveVert(int t, int b, double sp)
    : top(t), bottom(b), speed(sp), dir(1) {}

void MoveVert::update(double &y, double h, double dt) {
    y += dir * speed * dt;
    if (y <= top) dir = 1;
    if (y + h >= bottom) dir = -1;
}

Disappear::Disappear(bool initVis) : visible(initVis) {}

// ===================== 游戏物体 =====================
GameObject::GameObject(double x_, double y_, double w_, double h_)
    : x(x_), y(y_), w(w_), h(h_)
{
    triggerArea = rect();
    hiddenBoard = false;
    boardRevealed = false;
    isLocked = false;
}

void GameObject::update(double dt)
{
    if(isFlySpike)
    {
        if(moveHoriz) moveHoriz->update(x,w,dt);
        if(moveVert) moveVert->update(y,h,dt);
    }
    else
    {
        if (!isTrap || triggered)
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

    if (isFlySpike && triggered && !isLocked)
    {
        x -= flyDir * flySpeed * dt;
        if (x <= flyLeftBound) { x = flyLeftBound; flylaunched = true; }
        if (x >= flyRightBound) { x = flyRightBound; flylaunched = true; }
    }

    onUpdate(dt);
}

void GameObject::onUpdate(double dt) {}

QRectF GameObject::rect() const { return {x, y, w, h}; }

bool GameObject::isVisible() const {
    if (!disappear) return true;
    return disappear->visible;
}

// ===================== 平台 =====================
class Platform : public GameObject
{
public:
    using GameObject::GameObject;
    void draw(QPainter &p, double camerax) override;
};

void Platform::draw(QPainter &p, double camerax)
{
    if (!isVisible()) return;
    p.setBrush(Qt::gray);
    int x = qRound(this->x - camerax);
    int y = qRound(this->y);
    int w = qRound(this->w);
    int h = qRound(this->h);
    p.drawRect(x, y, w, h);
}

// ===================== 尖刺 =====================
class Spike : public GameObject
{
public:
    using GameObject::GameObject;
    void draw(QPainter &p, double camerax) override;
};

void Spike::draw(QPainter &p, double camerax)
{
    if (!isVisible()) return;
    p.setBrush(Qt::white);
    QPolygon tri;
    tri << QPoint((int)(x - camerax), (int)(y + h))
        << QPoint((int)(x + w/2 - camerax), (int)y)
        << QPoint((int)(x + w - camerax), (int)(y + h));
    p.drawPolygon(tri);
}

// ===================== 终点刀 =====================
class GoalBlade : public GameObject
{
public:
    using GameObject::GameObject;
    void draw(QPainter &p, double camerax) override;
};

void GoalBlade::draw(QPainter &p, double camerax)
{
    if (!isVisible()) return;
    p.setBrush(QColor(200, 200, 255));
    p.setPen(Qt::black);
    p.drawRect(QRectF(x - camerax, y, w, h));

    p.setBrush(QColor(100, 60, 20));
    p.drawRect(QRectF(x - camerax - 5, y + h - 15, w + 10, 15));
}

// ===================== 玩家 =====================
void Player::init()
{
    x = 100;
    y = GAME_HEIGHT - PLAYER_SIZE;
    platformspeedx = 0.0;
    vx = vy = 0;
    onGround = true;
    jumpCount = 0;
    jumppressedlast = false;
}

void Player::move(double dt)
{
    if(vx > 0) faceDir = 1;
    if(vx < 0) faceDir = -1;
    x += (vx + platformspeedx) * dt;
    y += vy * dt;
    onGround = false;

    if (y >= GAME_HEIGHT - PLAYER_SIZE) {
        y = GAME_HEIGHT - PLAYER_SIZE;
        vy = 0;
        onGround = true;
        jumpCount = 0;
    }

    if (!onGround) vy += GRAVITY * dt;

    if (x < 0) x = 0;
    if (x > MAP_WIDTH - PLAYER_SIZE) x = MAP_WIDTH - PLAYER_SIZE;
    if (y < 0) y = 0;
    if (vx != 0) {
        animTimer += dt;
        if (animTimer > 0.10) {
            animFrame = (animFrame + 1) % walkFrames;
            animTimer = 0;
        }
    } else {

        animFrame = 0;
    }

}

void Player::draw(QPainter &p, double camerax)
{
    QString path;
    // 静止用第0帧站姿，行走用动画帧
    if (vx == 0) {
        path = ":/picture/walk0.png";
    } else {
        path = QString(":/picture/walk%1.png").arg(animFrame);
    }

    QImage img(path);
    p.save();

    // 根据 faceDir 决定翻不翻转
    if(faceDir < 0)
    {

        p.scale(-1, 1);
        p.drawImage(QRectF(-(x - camerax) - 30, y, 30, 30), img);
    }
    else
    {

        p.drawImage(QRectF(x - camerax, y, 30,30), img);
    }

    p.restore();
}
void Player::input(bool A, bool D, bool W)
{
    if (A) vx = -MOVE_SPEED;
    else if (D) vx = MOVE_SPEED;
    else vx = 0;

    if (W && !jumppressedlast && jumpCount < maxJump) {
        vy = JUMP_POWER;
        jumpCount++;
    }
    jumppressedlast = W;
}

QRectF Player::rect() const { return QRectF(x, y, PLAYER_SIZE, PLAYER_SIZE); }
void Player::respawn() {

        init();

}
double Player::getx() const { return x; }
double Player::gety() const { return y; }
double Player::setvx(double a) { vx=a; return vx; }
double Player::setvy(double b) { vy=b; return vy; }
double Player::getvx() { return vx; }
double Player::getvy() { return vy; }
bool Player::getonground() { return onGround; }

bool Player::checkCollisions(QVector<GameObject*>& objs, double dt)
{
    QRectF pr = rect();
    bool hitSpike = false;
    platformspeedx = 0.0;

    for (auto obj : objs) {
        if(obj->hiddenBoard && !obj->boardRevealed)
        {
            bool isSpecialP18 = (obj->x == 840 && obj->y == 590);
            if(!isSpecialP18) continue;
        }
        if (!obj->isVisible() && !obj->isFlySpike)
        {
            bool isSpecialP18 = (obj->x == 840 && obj->y == 590);
            if(!isSpecialP18) continue;
        }

        QRectF or_ = obj->rect();
        if (!pr.intersects(or_)) continue;

        if (dynamic_cast<Platform*>(obj)) {
            if (obj->isFallTrap && !obj->fallTriggered) {
                obj->fallTriggered = true;
                if (obj->disappear) obj->disappear->visible = false;
                vx=0; vy+=500;
                continue;
            }

            bool vertOverlap = pr.bottom() > or_.top() && pr.top() < or_.bottom();
            bool horiOverlap = pr.right() > or_.left() && pr.left() < or_.right();

            if (vy >= 0 && pr.bottom() <= or_.top() + 12 && vertOverlap)
            {
                y = or_.top() - PLAYER_SIZE;
                vy = 0;
                onGround = true;
                jumpCount = 0;
                if (obj->moveHoriz) {
                    platformspeedx = obj->moveHoriz->dir * obj->moveHoriz->speed;
                }
                continue;
            }

            if (vy < 0 && pr.top() >= or_.bottom() - 12 && vertOverlap)
            {
                vy = 0;
                y = or_.bottom();
                continue;
            }

            if (horiOverlap)
            {
                if (pr.center().x() < or_.center().x())
                    x = or_.left() - PLAYER_SIZE;
                else
                    x = or_.right();
                vx = 0;
            }
        }

        if (dynamic_cast<Spike*>(obj)) { hitSpike = true; }
    }
    return hitSpike;
}


// ===================== 游戏初始化 =====================
void GameWidget::initGame()
{
    hasSavePoint = false;
    saveX = 100;
    saveY = GAME_HEIGHT - PLAYER_SIZE;
    player.init();
    keyA = keyD = keyW = false;
    setFixedSize(GAME_WIDTH, GAME_HEIGHT);
    camerax = 0.0;
    objs.clear();

    auto p1 = new Platform(150, 725, 50, 20); objs.append(p1);
    auto p2 = new Platform(200, 650, 20, 75); objs.append(p2);
    auto p3 = new Platform(200, 630, 221, 20); objs.append(p3);
    auto p6 = new Platform(540, 630, 100, 20); objs.append(p6);
    auto p7 = new Platform(640, 550, 20, 100); objs.append(p7);
    auto p8 = new Platform(640, 530, 70, 20); objs.append(p8);
    auto p9 = new Platform(710, 410, 20, 120); objs.append(p9);
    auto p10 = new Platform(0, 470, 380, 20); objs.append(p10);

    auto p5 = new Platform(420, 630, 120, 20);
    p5->isFallTrap = true;
    p5->fallRecoverTime = 2.0;
    p5->disappear = new Disappear(true);
    objs.append(p5);

    auto hideBoard1 = new Platform(420, 550, 100, 20);
    hideBoard1->hiddenBoard = true;
    hideBoard1->boardRevealed = false;
    hideBoard1->disappear = new Disappear(false);
    objs.append(hideBoard1);

    auto p11 = new Platform(730, 410, 100, 20); objs.append(p11);
    auto p21 = new Platform(120, 380, 200, 20); objs.append(p21);
    auto p22 = new Platform(320, 330, 20, 70); objs.append(p22);
    auto p23 = new Platform(340, 330, 120, 20); objs.append(p23);
    auto p24 = new Platform(460, 290, 100, 20);
    p24->moveHoriz=new MoveHoriz(460,660,100);
    objs.append(p24);

    auto p25 = new Platform(730, 180, 120, 20); objs.append(p25);
    auto p12 = new Platform(1000, 710, 100, 20); objs.append(p12);

    auto p13 = new Platform(920, 290, 140, 20);
    p13->isFallTrap = true;
    p13->fallRecoverTime = 0.1;
    p13->disappear = new Disappear(true);
    objs.append(p13);

    auto p14 = new Platform(900, 290, 20, 100);
   p14->hiddenBoard = true;
   p14->boardRevealed = false;
   p14->disappear = new Disappear(false);
    objs.append(p14);
    auto p15 = new Platform(1060, 290, 20, 100);
   p15->hiddenBoard = true;
    p15->boardRevealed = false;
    p15->disappear = new Disappear(false);
    objs.append(p15);
    auto p16 = new Platform(900, 370, 180, 30);
    p16->hiddenBoard = true;
    p16->boardRevealed = false;
    p16->disappear = new Disappear(false);
    objs.append(p16);
    auto p17 = new Platform(1300,700, 200, 100); objs.append(p17);

    auto p18 = new Platform(840, 590, 100, 30);
    p18->hiddenBoard = true;
    p18->boardRevealed = false;
    p18->disappear = new Disappear(false);
    objs.append(p18);

    auto p19 = new Platform(1150, 290, 100, 20);
    p19->isFallTrap = true;
    p19->moveHoriz=new MoveHoriz(1150,1350,70);
    p19->fallRecoverTime = 2.0;
    p19->disappear = new Disappear(true);
    objs.append(p19);

    // 尖刺
    auto s1 = new Spike(160, 705, SPIKE_SIZE, SPIKE_SIZE);
    s1->isTrap = true;
    s1->disappear = new Disappear(false);
    s1->needStaySec = 0.0;
    s1->oneShot =true;
    s1->trapKeepTime = 2.0;
    s1->triggerArea = QRectF(160, 705,40,-60);
    objs.append(s1);

    auto s2 = new Spike(580, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE);
    s2->isTrap = true; s2->disappear = new Disappear(false); s2->needStaySec = 0; s2->oneShot = false; objs.append(s2);
    auto s4 = new Spike(480, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE);
    s4->isTrap = true; s4->disappear = new Disappear(false); s4->needStaySec = 0; s4->oneShot = false; objs.append(s4);
    auto s5 = new Spike(510, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE);
    s5->isTrap = true; s5->disappear = new Disappear(false); s5->needStaySec = 0; s5->oneShot = false; objs.append(s5);
    auto s6 = new Spike(560, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE);
    s6->isTrap = true; s6->disappear = new Disappear(false); s6->needStaySec = 0; s6->oneShot = false; objs.append(s6);
    auto s7 = new Spike(600, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE);
    s7->isTrap = true; s7->disappear = new Disappear(false); s7->needStaySec = 0; s7->oneShot = false; objs.append(s7);

    auto s8 = new Spike(280, 610, SPIKE_SIZE, SPIKE_SIZE); s8->disappear = new Disappear(true); objs.append(s8);
    auto s9 = new Spike(470, 548, 4, 4); s9->disappear = new Disappear(true); objs.append(s9);

    auto s10=new Spike(710,435,SPIKE_SIZE,SPIKE_SIZE);
    s10->isFlySpike=true;
    s10->flyDir=1;
    s10->flySpeed=1100;
    s10->flyBack=true;
    s10->flyTriggerRange=QRectF(0,440,380,30);
    s10->disappear=new Disappear(false);
    s10->flylaunched=false;
    s10->isLocked = false;
    objs.append(s10);

    auto s3 = new Spike(700, 800, SPIKE_SIZE, SPIKE_SIZE);
    s3->isTrap = true; s3->disappear = new Disappear(false); s3->moveHoriz = new MoveHoriz(600,900,150);
    s3->needStaySec = 0.5; s3->oneShot = true; objs.append(s3);

    auto s11 = new Spike(940, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE); objs.append(s11);
    auto s12 = new Spike(975, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE); objs.append(s12);
    auto s13 = new Spike(1005, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE); objs.append(s13);
    auto s14 = new Spike(835, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE); objs.append(s14);
    auto s15 = new Spike(870, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE); objs.append(s15);
    auto s16 = new Spike(905, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE); objs.append(s16);
    auto s17 = new Spike(902, 290, SPIKE_SIZE, SPIKE_SIZE); objs.append(s17);

    auto s18 = new Spike(730, 390, SPIKE_SIZE, SPIKE_SIZE);
    s18->isTrap = true; s18->disappear = new Disappear(true); s18->needStaySec = 0; s18->oneShot = false;
    s18->triggerArea = QRectF(730, 390,40,-60); objs.append(s18);
    auto s19 = new Spike(760, 390, SPIKE_SIZE, SPIKE_SIZE);
    s19->isTrap = true; s19->disappear = new Disappear(true); s19->needStaySec = 0; s19->oneShot = false;
    s19->triggerArea = QRectF(760, 390,40,-60); objs.append(s19);
    auto s20 = new Spike(790, 390, SPIKE_SIZE, SPIKE_SIZE);
    s20->isTrap = true; s20->disappear = new Disappear(true); s20->needStaySec = 0; s20->oneShot = false;
    s20->triggerArea = QRectF(790, 390,40,-60); objs.append(s20);

    auto s21 = new Spike(1040, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE); objs.append(s21);
    auto s22 = new Spike(1075, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE); objs.append(s22);
    auto s23 = new Spike(1105, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE); objs.append(s23);
    auto s24 = new Spike(1140, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE); objs.append(s24);
    auto s25 = new Spike(1175, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE); objs.append(s25);
    auto s26 = new Spike(1105, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE); objs.append(s26);
    auto s27 = new Spike(1059, 290, SPIKE_SIZE, SPIKE_SIZE); objs.append(s27);
    auto s28 = new Spike(1300,680, SPIKE_SIZE, SPIKE_SIZE); objs.append(s28);
    auto s29 = new Spike(1335, 680, SPIKE_SIZE, SPIKE_SIZE); objs.append(s29);
    auto s30 = new Spike(1370, 680, SPIKE_SIZE, SPIKE_SIZE); objs.append(s30);

    auto goal = new GoalBlade(1450, 620, 15, 80); objs.append(goal);
//存档
    auto save1 = new SavePoint(280, 300, 25, 25);  // X Y 宽高
    objs.append(save1);

}

// ===================== 主窗口 =====================
GameWidget::GameWidget(QWidget *p) : QWidget(p)
{
    isPause = false;
    isWin = false;
    isQuitSad = false;
    deathCount = 0;
    initGame();
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    lastTime.start();
    bgImage.load(":/picture/background.png");
    connect(&timer, &QTimer::timeout, [this] {

        double rawDt = lastTime.restart() / 1000.0;
        double dt = rawDt > 0.05 ? 0.05 : rawDt;


        if(isPause || isWin)
          {
              update();
              return;
          }

        QRectF playerRect(player.getx(), player.gety(), PLAYER_SIZE, PLAYER_SIZE);

        for (GameObject* obj : objs)
        {
            obj->update(dt);
            if (dynamic_cast<SavePoint*>(obj)) {

                if (!hasSavePoint && playerRect.intersects(obj->rect())) {
                    saveX = obj->x;
                    saveY = obj->y - PLAYER_SIZE;
                    hasSavePoint = true;
                    if (SavePoint* sp = dynamic_cast<SavePoint*>(obj)) {
                        sp->saved = true;
                    }
                    qDebug() << "已存档！";
                }
            }

            if (obj->hiddenBoard && !obj->boardRevealed)
            {
                bool isSpecialP18 = (obj->x == 840 && obj->y == 590);
                if (playerRect.intersects(obj->rect()))
                {
                    obj->boardRevealed = true;
                    obj->disappear->visible = true;
                }
            }
            // 监牢陷阱：只要玩家掉在 p13 区域，直接显示牢房
            if (obj->hiddenBoard)
            {
                bool isInPrisonArea = playerRect.x() > 880 && playerRect.x() < 1100 && playerRect.y() > 280;
                if (isInPrisonArea) {
                    obj->boardRevealed = true;
                    obj->disappear->visible = true;
                }
            }

            if (obj->trapdestroyed) continue;

            if (obj->isTrap)
            {
                if (obj->oneShot && obj->triggered) continue;
                if (playerRect.intersects(obj->triggerArea))
                {
                    if (obj->needStaySec <= 0) {
                        obj->triggered = true; obj->disappear->visible = true;
                    } else {
                        obj->triggerTimer += dt;
                        if (obj->triggerTimer >= obj->needStaySec) {
                            obj->triggered = true; obj->disappear->visible = true;
                        }
                    }
                } else {
                    obj->triggerTimer = 0;
                    if (!obj->oneShot) { obj->triggered = false; obj->disappear->visible = false; }
                }
            }

            if (obj->isFlySpike && !obj->isLocked)
            {
                bool onP10 = playerRect.intersects(obj->flyTriggerRange);
                if (onP10 && !obj->triggered && !obj->flylaunched)
                {
                    obj->triggered = true;
                    obj->disappear->visible = true;
                }
            }
        }

        if(!isDying) player.input(keyA, keyD, keyW);
        player.move(dt);

        double targetcamx = player.getx() - GAME_WIDTH/2;
        if (targetcamx < 0) targetcamx = 0;
        double maxCamX = MAP_WIDTH - GAME_WIDTH;
        if (targetcamx > maxCamX) targetcamx = maxCamX;
        camerax += (targetcamx - camerax) * 0.08;
        if(!isDying && !isWin)
        {
            for(auto obj : objs)
            {
                if(dynamic_cast<GoalBlade*>(obj))
                {
                    if(playerRect.intersects(obj->rect()))
                    {
                        isWin = true;
                        break;
                    }
                }
            }
        }

        if(!isDying)
        {
            if (player.checkCollisions(objs, dt))
            {
                isDying = true;
                dyingTimer = 0;
                player.isDying = true;
                player.setvx(0);
                player.setvy(-380);
                deathCount++;
            }
        }
        else
        {
            dyingTimer += dt;
            player.setvx(0);
            if(dyingTimer >= DYING_DELAY)
            {
                isDying = false;
                      player.isDying = false;

                if (hasSavePoint)
                {
                    player.setx(saveX)  ;
                    player.sety(saveY);
                    player.setvx(0);
                    player.setvy(0);
                    player.setOnGround(true);
                       player.setJumpCount(0);
                }
                else
                {
                    player.respawn();
                }

                resetAllTraps();

            }
        }
//文字提示
        showTip = false;
        QRectF p25Rect(730, 180, 120, 20);
        bool onP25 = (
            playerRect.bottom() >= p25Rect.top() - 5 &&
            playerRect.bottom() <= p25Rect.top() + 15 &&
            playerRect.right() > p25Rect.left() &&
            playerRect.left() < p25Rect.right()
        );

        if (onP25) {
            showTip = true;
        } else {
            if (playerRect.right() < p25Rect.left() || playerRect.left() > p25Rect.right()) {
                showTip = false;
            }
        }

        tipX = 730;
        tipY = 160;
        update();
    });
    timer.start(15);
}

void GameWidget::resetAllTraps()
{
    for (GameObject* obj : objs)
    {
        if (obj->isTrap) {
            obj->triggered = false; obj->triggerTimer = 0; obj->trapTimer = 0; obj->trapdestroyed = false;
            if (obj->disappear) obj->disappear->visible = false;
        }
        if (obj->isFallTrap) {
            obj->fallTriggered = false; obj->fallTimer = 0;
            if (obj->disappear) obj->disappear->visible = true;
        }
        if (obj->hiddenBoard) {
            obj->boardRevealed = false;
            if (obj->disappear) obj->disappear->visible = false;
        }
        if (obj->isFlySpike) {
            obj->flylaunched = false; obj->isLocked = false; obj->triggered = false;
            obj->x = 710; obj->disappear->visible = false; obj->flyDir=1;
        }

        if (auto sp = dynamic_cast<SavePoint*>(obj))
        {
            sp->saved = false;
        }
    }
}

GameWidget::~GameWidget() { qDeleteAll(objs); }

void GameWidget::drawGame(QPainter &p)
{

    for (auto o : objs)
        o->draw(p, camerax);
    player.draw(p, camerax);

    p.setPen(Qt::white);
    QFont font;
    font.setPointSize(14);
    p.setFont(font);
    p.drawText(width() - 180, 35, QString("死亡次数：%1").arg(deathCount));

    if(showTip)
    {
        p.setPen(Qt::yellow);
        QFont tipFont;
        tipFont.setPointSize(16);
        tipFont.setBold(true);
        p.setFont(tipFont);
        p.drawText(tipX - camerax, tipY, "路在脚下？");
    }

    // ========== 黑色虚化开始界面 ==========
    if(isConfirmStart)
    {
        p.fillRect(rect(), QColor(0,0,0,180));
        p.setPen(Qt::yellow);
        p.setFont(QFont("Microsoft YaHei", 28, QFont::Bold));
        p.drawText(0,120,width(),80,Qt::AlignCenter,"是否开始游戏？");

        // 玩法按键说明
        p.setPen(Qt::lightGray);
        p.setFont(QFont("Microsoft YaHei", 18));
        p.drawText(0,220,width(),40,Qt::AlignCenter,"A 键 — 向左移动");
        p.drawText(0,260,width(),40,Qt::AlignCenter,"D 键 — 向右移动");
        p.drawText(0,300,width(),40,Qt::AlignCenter,"W 键 — 跳跃（可二段跳）");
        p.drawText(0,340,width(),40,Qt::AlignCenter,"ESC — 打开暂停菜单");
        p.drawText(0,380,width(),40,Qt::AlignCenter,"经过青色圆点自动存档（已存档变红色）");
        // 操作选项
        p.setPen(Qt::white);
        p.setFont(QFont("Microsoft YaHei", 20));
        p.drawText(0,420,width(),60,Qt::AlignCenter,"空格 → 开始游戏");
        p.drawText(0,470,width(),60,Qt::AlignCenter,"ESC  → 返回主菜单");
        return;
    }
    if(isPause)
    {
        p.fillRect(rect(), QColor(0,0,0,180));
        p.setFont(QFont("Microsoft YaHei",20));

        QString opts[] = {"继续游戏","重新开始","结束游戏"};
        for(int i=0;i<3;i++){
            p.setPen(i==pauseSel ? Qt::yellow : Qt::white);
            p.drawText(330, 220+i*50, opts[i]);
        }
        p.setFont(QFont("Microsoft YaHei",12));
        p.setPen(Qt::gray);
        p.drawText(300,400,"↑↓选择 回车确认");
    }

    if(isWin)
    {
        p.fillRect(rect(), QColor(0,0,0,200));
        p.setPen(Qt::yellow);
        p.setFont(QFont("Microsoft HuaWenXingKai",18, QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, "天上地下，唯我独尊！亲爱的学生们，老师我要回来啦哈哈哈!");

        p.setPen(Qt::white);
        p.setFont(QFont("Microsoft YaHei",16));

        p.drawText(0,height()-110,width(),60,Qt::AlignCenter,"R 重新开始 | ESC 返回菜单 | Q 结束游戏");
    }
    // 未通关遗憾离开 虚化界面
    if(isQuitSad)
    {
        p.fillRect(rect(), QColor(0,0,0,220));
        p.setPen(QColor(180, 120, 120));
        p.setFont(QFont("Microsoft YaHei", 22, QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, "真是抱歉没能让咒灵大人使出全力");
    }
}

void GameWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);


    if (!bgImage.isNull()) {
        p.drawImage(rect(), bgImage);
    } else {
        p.fillRect(rect(), Qt::black);
    }


    drawGame(p);
}
void GameWidget::keyPressEvent(QKeyEvent *e)
{
    // ---------- 黑色虚化界面 ----------
    if(isConfirmStart)
    {
        if(e->key() == Qt::Key_Space)
        {
            isConfirmStart = false;
            initGame();
            update();
        }
        if(e->key() == Qt::Key_Escape)
        {
            this->close();
            MenuWindow *w = new MenuWindow();
            w->show();
        }
        return;
    }

    // ---------- 悲伤退出界面---------
    if(isQuitSad)
    {
        return;
    }

    // ---------- 暂停 ----------
    if(isPause)
    {
        if(e->key() == Qt::Key_Up)
        {
            pauseSel--;
            if(pauseSel<0) pauseSel=2;
            update();
            return;
        }
        if(e->key() == Qt::Key_Down)
        {
            pauseSel++;
            if(pauseSel>2) pauseSel=0;
            update();
            return;
        }
        if(e->key() == Qt::Key_Return)
        {
            if(pauseSel == 0)
            {
                isPause = false;
            }
            else if(pauseSel == 1)
            {
                isPause = false;
                deathCount = 0;
                initGame();
            }
            else if(pauseSel == 2)
            {
                isPause = false;
                if(isWin)
                {
                    QApplication::quit();
                }
                else
                {
                    isQuitSad = true;
                    update();
                    QTimer::singleShot(2000, this, [this](){
                        QApplication::quit();
                    });
                }
            }
            update();
            return;
        }
        if(e->key() == Qt::Key_Escape)
        {
            isPause = false;
            update();
            return;
        }
        return;
    }

    // ---------- 胜利 ----------
    if(isWin)
    {
        if(e->key() == Qt::Key_R)
        {
            isWin = false;
            deathCount = 0;
            initGame();
        }
        else if(e->key() == Qt::Key_Escape)
        {
            isWin = false;
            this->close();
            MenuWindow *w = new MenuWindow();
            w->show();
        }
        else if(e->key() == Qt::Key_Q)
        {
            QApplication::quit();
        }
        return;
    }

    // ---------- 游戏中按键 ----------
    if(e->key() == Qt::Key_Escape)
    {
        isPause = true;
        pauseSel = 0;
        update();
        return;
    }

    switch(e->key())
    {
        case Qt::Key_A: keyA = 1; break;
        case Qt::Key_D: keyD = 1; break;
        case Qt::Key_W: keyW = 1; break;
        default: break;
    }
}
void GameWidget::keyReleaseEvent(QKeyEvent *e) {
    if(isPause || isWin || isQuitSad)
        return;

    switch(e->key()){
        case Qt::Key_A: keyA=0; break;
        case Qt::Key_D: keyD=0; break;
        case Qt::Key_W: keyW=0; break;
        default: QWidget::keyReleaseEvent(e);
    }
}
