#ifndef IS_GUARD
#define IS_GUARD

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <time.h>
#include <modbus.h>
#include <Windows.h>

#endif // IS_GUARD

//#define OFFLINE
#define MODBUS_IP "192.168.0.1"
#define MODBUS_PORT 502
#define MODBUS_READ_ADDR 0
#define MODBUS_WRITE_ADDR 0
#define MODBUS_READ_LEN 2
#define MODBUS_WRITE_LEN 3
#define MODBUS_TIMEOUT_SEC 1
#define MODBUS_TIMEOUT_USEC 0
#define READ_BUTTON 0
#define READ_SWITCH 1 
#define WRITE_LIGHT 0
#define TIMESLOT 999
#define BTN_PRESS 0
#define BTN_RELEASE 1
#define BTN_X 256
#define BTN_Y 256
#define BTN_D 256
#define SWT_X 128
#define SWT_Y 64
#define SWT_D 64
#define SWT_P_X 288
#define SWT_P_Y 96
#define BTN_L 340
#define BTN_R 510
#define TIMEOUT_WAIT 1000

static bool g_pcnt;
static bool g_btn_event;
static bool act;
static bool g_tkl_act;
static bool tkl;
static bool g_swt_act;
static clock_t g_start;
static modbus_t *g_mb;
static uint8_t g_light[32];

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	INIT();

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(MAIN()));
	timer->start(1);

	QGraphicsScene *scene = new QGraphicsScene(this);
	ui->graph_all->setScene(scene);

	ui->graph_all->viewport()->installEventFilter(this);
}

void MainWindow::INIT()
{
	g_pcnt = 0;
	g_btn_event = BTN_RELEASE;
	act = 0;
	g_tkl_act = 0;
	tkl = 0;
	g_swt_act = 0;
	g_start = 0;
	g_light[0] = {0};

#ifndef OFFLINE
	g_mb = modbus_new_tcp(MODBUS_IP, MODBUS_PORT);
	modbus_connect(g_mb);
	modbus_set_response_timeout(g_mb, MODBUS_TIMEOUT_SEC, MODBUS_TIMEOUT_USEC);
#endif
	g_light[WRITE_LIGHT] = g_pcnt;
}

void delay(int ms)
{
	clock_t init = clock();
	while (clock() - init < ms);
}

int timeout(int ret)
{
	int ret_now;
	if (ret == -1) {
		printf("MODBUS connect error\n");
		modbus_close(g_mb);
		modbus_free(g_mb);

		while (1) {
			delay(TIMEOUT_WAIT);
			printf("MODBUS is connecting...\n");
			g_mb = modbus_new_tcp(MODBUS_IP, MODBUS_PORT);
			ret_now = modbus_connect(g_mb);
			if (ret_now != -1) {
				modbus_set_response_timeout(g_mb, MODBUS_TIMEOUT_SEC, MODBUS_TIMEOUT_USEC);
				printf("MODBUS connect again\n");
				break;
			} else {
				modbus_close(g_mb);
				modbus_free(g_mb);
			}
		}
		return 1;
	} else {
		return 0;
	}
}

void MainWindow::MAIN()
{
	ui->graph_all->scene()->clear();
#ifdef OFFLINE   
	BUTTON(g_btn_event ? 1 : 0);
	LIGHT(g_btn_event ? 0 : 1);
	SWITCH(g_btn_event ? 0 : 1);
#else	
	uint8_t button[32];
	int ret_b, ret_l, tm;
	tm = 0;
	ret_b = modbus_read_input_bits(g_mb, MODBUS_READ_ADDR, MODBUS_READ_LEN, button);
	//////////////////////
	tm = timeout(ret_b);
	if (tm) {
		g_pcnt = 0;
		g_light[WRITE_LIGHT] = g_pcnt;
		g_tkl_act = 0;
		return; //break
	}
	//////////////////////	
	BUTTON(act ? 0 :1);

	if (!button[READ_BUTTON] && g_btn_event) {
		act = 0;
	}

	if (!button[READ_SWITCH]) {
		SWITCH(0);
		if (g_swt_act) {
			printf("switch on left\n");
			g_light[WRITE_LIGHT] = 0;
			g_pcnt = 0;
			g_tkl_act = 0;
			g_swt_act = 0;
		}

		if ( (button[READ_BUTTON] || !g_btn_event) && !act) {
			printf("TEST left: Button pressed in normal mode\n");
			g_pcnt = !g_pcnt;
			g_light[WRITE_LIGHT] = g_pcnt;
			act = 1;
		}
	} else {
		SWITCH(1);
		if (!g_swt_act) {
			printf("switch on right\n");
			g_light[WRITE_LIGHT] = 0;
			g_pcnt = 0;
			g_tkl_act = 0;
			g_swt_act = 1;
		}	

		if ((button[READ_BUTTON] || !g_btn_event) && !act) {
			g_tkl_act = !g_tkl_act;
			printf("TEST right: Button pressed in flash mode\n");
			act = 1;
			g_start = clock();
			tkl = !tkl;
			g_light[WRITE_LIGHT] = tkl;
		}

		if (g_tkl_act) {
			if (clock() - g_start > TIMESLOT) {
				g_start = clock();
				tkl = !tkl;
				g_light[WRITE_LIGHT] = tkl;
			}
		} else {
			g_light[WRITE_LIGHT] = 0;
			tkl = 0;
		}

	}
	ret_l = modbus_write_bits(g_mb, MODBUS_WRITE_ADDR, MODBUS_WRITE_LEN, g_light);
	//////////////////////
	tm = timeout(ret_l);
	if (tm) {
		g_pcnt = 0;
		g_light[WRITE_LIGHT] = g_pcnt;
		g_tkl_act = 0;
		return;
	} else {
		(g_light[WRITE_LIGHT] == 1) ? LIGHT(1) : LIGHT(0);
	}
		
	//////////////////////
#endif	
}

void MainWindow::LIGHT(bool status)
{	
	QPixmap *image_ori = new QPixmap("lightbulb_on_off.png");
	QPixmap image_light = image_ori->copy(0, (status ? BTN_D : 0), BTN_X, BTN_Y);
	QGraphicsPixmapItem *item_light = ui->graph_all->scene()->addPixmap(image_light);
	item_light->setOffset(QPointF(-BTN_D, 0));
	ui->graph_all->scene()->addItem(item_light);
	
	image_ori->swap(QPixmap());
}

void MainWindow::BUTTON(bool status)
{	
	QPixmap *image_ori = new QPixmap("button_press_release.png");
	QPixmap image_button = image_ori->copy(0, (status ? 0 : BTN_D), BTN_X, BTN_Y);
	QGraphicsPixmapItem *item_button = ui->graph_all->scene()->addPixmap(image_button);
	item_button->setOffset(QPointF(0, 0));
	ui->graph_all->scene()->addItem(item_button);

	image_ori->swap(QPixmap());
}

void MainWindow::SWITCH(bool status)
{
	QPixmap *image_ori = new QPixmap("switches-on-and-off.png");
	QPixmap image_switch = image_ori->copy(0, (status ? SWT_D : 0), SWT_X, SWT_Y);
	QGraphicsPixmapItem *item_switch = ui->graph_all->scene()->addPixmap(image_switch);
	item_switch->setOffset(QPointF(SWT_P_X, SWT_P_Y));
	ui->graph_all->scene()->addItem(item_switch);

	image_ori->swap(QPixmap());
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == ui->graph_all->viewport()) {
		if (event->type() == QEvent::MouseButtonPress) {
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
			if (mouseEvent->button() == Qt::LeftButton) {
				const QPointF position = mouseEvent->pos();
				if (position.x() > BTN_L && position.x() < BTN_R) {
					g_btn_event = BTN_PRESS;
					return true;
				}
			}
		} else if (event->type() == QEvent::MouseButtonRelease) {
			g_btn_event = BTN_RELEASE;
			return true;
		} else {
			return false;
		}
			
	} else {
		// pass the event on to the parent class
		return QMainWindow::eventFilter(obj, event);
	}
}

MainWindow::~MainWindow()
{
#ifndef OFFLINE	
	modbus_close(g_mb);
	modbus_free(g_mb);
#endif
	ui->graph_all->scene()->clear();
	delete ui;
}
