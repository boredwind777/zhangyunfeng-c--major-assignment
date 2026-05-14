#include "gamewidget.h"
#include <QApplication>

// ===================== 平台类实现 =====================
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

// ===================== 尖刺类实现 =====================
class Spike : public GameObject
{
public:
    using GameObject::GameObject;
    void draw(QPainter &p, double camerax) override;
};

void Spike::draw(QPainter &p, double camerax)
{
    if (!isVisible()) return;
    p.setBrush(Qt::red);
    QPolygon tri;
    tri << QPoint((int)(x - camerax), (int)(y + h))
        << QPoint((int)(x + w/2 - camerax), (int)y)
        << QPoint((int)(x + w - camerax), (int)(y + h));
    p.drawPolygon(tri);
}

// ===================== 玩家逻辑 =====================
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
    x += (vx + platformspeedx) * dt;
    y += vy * dt;

    onGround = false;

    if (y >= GAME_HEIGHT - PLAYER_SIZE) {
        y = GAME_HEIGHT - PLAYER_SIZE;
        vy = 0;
        onGround = true;
        jumpCount = 0;
    }

    if (!onGround)
        vy += GRAVITY * dt;

    if (x < 0) x = 0;
    if (x > MAP_WIDTH - PLAYER_SIZE)
        x = MAP_WIDTH - PLAYER_SIZE;
    if (y < 0) y = 0;
}

void Player::draw(QPainter &p, double camerax)
{
    if(!isDying)
        p.setBrush(Qt::blue);
    else
        p.setBrush(Qt::red);
    p.drawRect((int)(x - camerax), (int)y, PLAYER_SIZE, PLAYER_SIZE);
}

void Player::input(bool A, bool D, bool W)
{
    vx = 0;
    if (A) vx = -MOVE_SPEED;
    if (D) vx = MOVE_SPEED;

    if (W && !jumppressedlast && jumpCount < maxJump) {
        vy = JUMP_POWER;
        jumpCount++;
    }
    jumppressedlast = W;
}

QRectF Player::rect() const
{
    return QRectF(x, y, PLAYER_SIZE, PLAYER_SIZE);
}

void Player::respawn()
{
    init();
}

bool Player::checkCollisions(QVector<GameObject*>& objs, double dt)
{
    QRectF pr = rect();
    bool hitSpike = false;
    platformspeedx = 0.0;

    for (auto obj : objs) {
        if(obj->hiddenBoard && !obj->boardRevealed)
            continue;
        if (!obj->isVisible()&& !obj->isFlySpike)
            continue;
        QRectF or_ = obj->rect();

        if (!pr.intersects(or_)) continue;

        if (dynamic_cast<Platform*>(obj)) {
            if (obj->isFallTrap && !obj->fallTriggered) {
                obj->fallTriggered = true;
                if (obj->disappear) obj->disappear->visible = false;
                vx=0;
                vy+=500;
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
                    platformspeedx = obj->moveHoriz->deltaX / dt;
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
        if (dynamic_cast<Spike*>(obj)) {
            hitSpike = true;
            continue;
        }
    }

    return hitSpike;
}

// ===================== 游戏初始化 =====================
void GameWidget::initGame()
{
    player.init();
    keyA = keyD = keyW = false;
    setFixedSize(GAME_WIDTH, GAME_HEIGHT);
    camerax = 0.0;
    objs.clear();

    auto p1 = new Platform(150, 725, 50, 20);
    objs.append(p1);
    auto p2 = new Platform(200, 650, 20, 75);
    objs.append(p2);
    auto p3 = new Platform(200, 630, 221, 20);
    objs.append(p3);
    auto p6 = new Platform(540, 630, 100, 20);
    objs.append(p6);
    auto p7 = new Platform(640, 550, 20, 100);
    objs.append(p7);
    auto p8 = new Platform(640, 530, 70, 20);
    objs.append(p8);
    auto p9 = new Platform(710, 410, 20, 120);
    objs.append(p9);
    auto p10 = new Platform(0, 470, 380, 20);
    objs.append(p10);

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

    auto s1 = new Spike(160, 705, SPIKE_SIZE, SPIKE_SIZE);
    s1->isTrap = true;
    s1->disappear = new Disappear(false);
    s1->needStaySec = 0.0;
    s1->oneShot =true;
    s1->trapKeepTime = 2.0;
    s1->triggerArea = QRectF(160, 705,40,-60);
    objs.append(s1);

    auto s2 = new Spike(580, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE);
    s2->isTrap = true;
    s2->disappear = new Disappear(false);
    s2->needStaySec = 0.0;
    s2->oneShot = false;
    objs.append(s2);
    auto s4 = new Spike(480, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE);
    s4->isTrap = true;
    s4->disappear = new Disappear(false);
    s4->needStaySec = 0.0;
    s4->oneShot = false;
    objs.append(s4);
    auto s5 = new Spike(510, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE);
    s5->isTrap = true;
    s5->disappear = new Disappear(false);
    s5->needStaySec = 0.0;
    s5->oneShot = false;
    objs.append(s5);
    auto s6 = new Spike(560, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE);
    s6->isTrap = true;
    s6->disappear = new Disappear(false);
    s6->needStaySec = 0.0;
    s6->oneShot = false;
    objs.append(s6);
    auto s7 = new Spike(600, GAME_HEIGHT - SPIKE_SIZE, SPIKE_SIZE, SPIKE_SIZE);
    s7->isTrap = true;
    s7->disappear = new Disappear(false);
    s7->needStaySec = 0.0;
    s7->oneShot = false;
    objs.append(s7);

    auto s8 = new Spike(280, 610, SPIKE_SIZE, SPIKE_SIZE);
    s8->isTrap = false;
    s8->disappear = new Disappear(true);
    objs.append(s8);
    auto s9 = new Spike(470, 548, 4, 4);
    s9->isTrap = false;
    s9->disappear = new Disappear(true);
    objs.append(s9);

    // ===================== 最终完美飞刺 =====================
    auto s10=new Spike(710,435,SPIKE_SIZE,SPIKE_SIZE);
    s10->isFlySpike=true;
    s10->flyDir=1;
    s10->flySpeed=2000;
    s10->flyBack=true;
    s10->flyBackInterval=1.5;
    s10->flyTriggerRange=QRectF(0,440,380,30);
    s10->isTrap=false;
    s10->oneShot=false;
    s10->disappear=new Disappear(false);
    bool flyLaunched = false;     // 第一波是否已经飞出
    double waitBackTimer = 0.0;   // 原地等待回马枪计时
    bool isLocked = false;       // 是否本局锁住不再触发


    objs.append(s10);

    auto s3 = new Spike(700, 800, SPIKE_SIZE, SPIKE_SIZE);
    s3->isTrap = true;
    s3->disappear = new Disappear(false);
    s3->moveHoriz = new MoveHoriz(600, 900, 150);
    s3->needStaySec = 0.5;
    s3->oneShot = true;
    objs.append(s3);
}

// ===================== 游戏主循环 =====================
GameWidget::GameWidget(QWidget *p) : QWidget(p)
{
    initGame();
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    lastTime.start();

    connect(&timer, &QTimer::timeout, [this] {
        double rawDt = lastTime.restart() / 1000.0;
        double dt = rawDt > 0.05 ? 0.05 : rawDt;

        QRectF playerRect(player.getx(), player.gety(), PLAYER_SIZE, PLAYER_SIZE);

        for (GameObject* obj : objs)
        {
            obj->update(dt);

            if (obj->hiddenBoard && !obj->boardRevealed)
            {
                if (playerRect.intersects(obj->rect()))
                {
                    obj->boardRevealed = true;
                    if (obj->disappear)
                        obj->disappear->visible = true;
                }
            }


           if (!obj->isTrap && !obj->isFlySpike) continue;
            if (obj->trapdestroyed) continue;
            if (obj->oneShot && obj->triggered) continue;

            if (playerRect.intersects(obj->triggerArea))
            {
                if (obj->needStaySec <= 0.001)
                {
                    obj->triggered = true;
                    if (obj->disappear)
                        obj->disappear->visible = true;
                }
                else
                {
                    obj->triggerTimer += dt;
                    if (obj->triggerTimer >= obj->needStaySec)
                    {
                        obj->triggered = true;
                        if (obj->disappear)
                            obj->disappear->visible = true;
                    }
                }
            }
            else
            {
                obj->triggerTimer = 0.0;
                if (!obj->oneShot)
                {
                    obj->triggered = false;
                    if (obj->disappear)
                        obj->disappear->visible = false;
                }
            }

            // 飞行尖刺触发
            if (obj->isFlySpike && !obj->isLocked)
            {
                bool onP10 = playerRect.intersects(obj->flyTriggerRange);
                // 只有没启动过，才允许第一次触发
                if (onP10 && !obj->triggered && !obj->flyLaunched)
                {
                    obj->triggered = true;
                    obj->disappear->visible = true;
                }

                // 如果玩家离开平台，直接作废，不回马枪
                if (!onP10)
                {
                    obj->waitBackTimer = 0.0;
                }
            }
            if (obj->isFlySpike) continue;
        }

        if(!isDying)
        {
            player.input(keyA, keyD, keyW);
        }
        player.move(dt);

        double targetcamx = player.getx() - GAME_WIDTH / 2.0;
        if (targetcamx < 0) targetcamx = 0;
        double maxCamX = MAP_WIDTH - GAME_WIDTH;
        if (targetcamx > maxCamX) targetcamx = maxCamX;
        camerax += (targetcamx - camerax) * 0.08;

        if(!isDying)
        {
            if (player.checkCollisions(objs, dt))
            {
                isDying = true;
                dyingTimer = 0.0;
                player.isDying = true;
                player.setvx(0);
                player.setvy(0);
                player.setvy(-380);
            }
        }
        else
        {
            dyingTimer += dt;
            player.setvx(0);
            if(dyingTimer >= DYING_DELAY)
            {
                player.respawn();
                resetAllTraps();
                isDying = false;
                dyingTimer = 0.0;
                player.isDying = false;
            }
        }

        update();
    });

    timer.start(15);
}

void GameWidget::resetAllTraps()
{
    for (GameObject* obj : objs)
    {
        if (obj->isTrap)
        {
            obj->triggered = false;
            obj->triggerTimer = 0.0;
            obj->trapTimer = 0.0;
            obj->trapdestroyed = false;
            if (obj->disappear)
                obj->disappear->visible = false;
        }

        if (obj->isFallTrap)
        {
            obj->fallTriggered = false;
            obj->fallTimer = 0.0;
            if (obj->disappear)
                obj->disappear->visible = true;
        }

        if (obj->hiddenBoard)
        {
            obj->boardRevealed = false;
            if (obj->disappear)
                obj->disappear->visible = false;
        }
        if (obj->isFlySpike)
        {
            obj->flyLaunched = false;
            obj->waitBackTimer = 0.0;
            obj->isLocked = false;
            obj->triggered = false;
            obj->x = 710;
            obj->disappear->visible = false;
        }
    }
}

GameWidget::~GameWidget()
{
    qDeleteAll(objs);
}

void GameWidget::drawGame(QPainter &p)
{
    p.fillRect(rect(), Qt::black);
    for (auto o : objs)
        o->draw(p, camerax);
    player.draw(p, camerax);
}

void GameWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    drawGame(p);
}

void GameWidget::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_A: keyA = true; break;
    case Qt::Key_D: keyD = true; break;
    case Qt::Key_W: keyW = true; break;
    default: QWidget::keyPressEvent(e);
    }
}

void GameWidget::keyReleaseEvent(QKeyEvent *e)
{
    switch (e->key())
    {
    case Qt::Key_A: keyA = false; break;
    case Qt::Key_D: keyD = false; break;
    case Qt::Key_W: keyW = false; break;
    default: QWidget::keyReleaseEvent(e);
    }
}
