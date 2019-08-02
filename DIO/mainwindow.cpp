#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <modbus.h>
#include <Windows.h>

//#define OFFLINE
#define MODBUS_IP "192.168.0.1"
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

static bool pcnt;
static bool btn_event;
static bool act;
static bool tkl_act;
static bool tkl;
static bool swt_act;
static clock_t start;
static modbus_t *mb;
static uint8_t light[32];

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	PRA_INIT();

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(MAIN()));
	timer->start(1);

	QGraphicsScene *scene = new QGraphicsScene(this);
	ui->graph_all->setScene(scene);

	ui->graph_all->viewport()->installEventFilter(this);
}

void MainWindow::PRA_INIT()
{
	pcnt = 0;
	btn_event = BTN_RELEASE;
	act = 0;
	tkl_act = 0;
	tkl = 0;
	swt_act = 0;
	start = 0;
	light[0] = {0};

#ifndef OFFLINE
	mb = modbus_new_tcp(MODBUS_IP, 502);
	modbus_connect(mb);
	modbus_set_response_timeout(mb, MODBUS_TIMEOUT_SEC, MODBUS_TIMEOUT_USEC);
#endif
	light[WRITE_LIGHT] = pcnt;
}

void delay(int ms)
{
	clock_t init = clock();
	while (clock() - init < ms);
}

int timeout(int ret, modbus_t *mb)
{
	int ret_now;
	if (ret == -1)
	{
		printf("MODBUS connect error\n");
		modbus_close(mb);
		modbus_free(mb);

		while (1)
		{
			delay(1000);
			printf("MODBUS is connecting...\n");
			mb = modbus_new_tcp(MODBUS_IP, 502);
			ret_now = modbus_connect(mb);
			if (ret_now != -1)
			{
				modbus_set_response_timeout(mb, MODBUS_TIMEOUT_SEC, MODBUS_TIMEOUT_USEC);
				printf("MODBUS connect again\n");
				delay(100);
				break;
			}
			else
			{
				modbus_close(mb);
				modbus_free(mb);
			}
		}
		return 1;
	}
	else
		return 0;

}

void MainWindow::MAIN()
{
	ui->graph_all->scene()->clear();
#ifdef OFFLINE   
	BUTTON(btn_event ? 1 : 0);
	LIGHT(btn_event ? 0 : 1);
	SWITCH(btn_event ? 0 : 1);
#else	
	uint8_t button[32];
	int ret_b, ret_l, tm;
	tm = 0;
	ret_b = modbus_read_input_bits(mb, MODBUS_READ_ADDR, MODBUS_READ_LEN, button);
	//////////////////////
	tm = timeout(ret_b, mb);
	if (tm)
	{
		pcnt = 0;
		light[WRITE_LIGHT] = pcnt;
		tkl_act = 0;
		return;
	}
	//////////////////////	
	BUTTON(act ? 0 :1);

	if (!button[READ_BUTTON] && btn_event)
		act = 0;

	if (!button[READ_SWITCH])
	{
		SWITCH(0);
		if (swt_act)
		{
			printf("switch on left\n");
			light[WRITE_LIGHT] = 0;
			pcnt = 0;
			tkl_act = 0;
			swt_act = 0;
		}

		if ( (button[READ_BUTTON] || !btn_event) && !act)
		{
			printf("TEST left: Button pressed in normal mode\n");
			pcnt = !pcnt;
			light[WRITE_LIGHT] = pcnt;
			act = 1;
		}
	}
	else
	{
		SWITCH(1);
		if (!swt_act)
		{
			printf("switch on right\n");
			light[WRITE_LIGHT] = 0;
			pcnt = 0;
			tkl_act = 0;
			swt_act = 1;
		}	

		if ((button[READ_BUTTON] || !btn_event) && !act)
		{
			tkl_act = !tkl_act;
			printf("TEST right: Button pressed in flash mode\n");
			act = 1;
			start = clock();
			tkl = !tkl;
			light[WRITE_LIGHT] = tkl;
		}

		if (tkl_act)
		{
			if (clock() - start > TIMESLOT)
			{
				start = clock();
				tkl = !tkl;
				light[WRITE_LIGHT] = tkl;
			}
		}
		else
		{
			light[WRITE_LIGHT] = 0;
			tkl = 0;
		}

	}
	ret_l = modbus_write_bits(mb, MODBUS_WRITE_ADDR, MODBUS_WRITE_LEN, light);
	//////////////////////
	tm = timeout(ret_l, mb);
	if (tm)
	{
		pcnt = 0;
		light[WRITE_LIGHT] = pcnt;
		tkl_act = 0;
		return;
	}
	else
		(light[WRITE_LIGHT] == 1) ? LIGHT(256) : LIGHT(0);
	//////////////////////
#endif	
}

void MainWindow::LIGHT(bool status)
{	
	QPixmap *image_ori = new QPixmap("lightbulb_on_off.png");
	QPixmap image_light = image_ori->copy(0, (status ? 256 : 0), 256, 256);
	QGraphicsPixmapItem *item_light = ui->graph_all->scene()->addPixmap(image_light);
	item_light->setOffset(QPointF(-256, 0));
	ui->graph_all->scene()->addItem(item_light);
	
	image_ori->swap(QPixmap());
}

void MainWindow::BUTTON(bool status)
{	
	QPixmap *image_ori = new QPixmap("button_press_release.png");
	QPixmap image_button = image_ori->copy(0, (status ? 0 : 256), 256, 256);
	QGraphicsPixmapItem *item_button = ui->graph_all->scene()->addPixmap(image_button);
	item_button->setOffset(QPointF(0, 0));
	ui->graph_all->scene()->addItem(item_button);

	image_ori->swap(QPixmap());
}

void MainWindow::SWITCH(bool status)
{
	QPixmap *image_ori = new QPixmap("switches-on-and-off.png");
	QPixmap image_switch = image_ori->copy(0, (status ? 64 : 0), 128, 64);
	QGraphicsPixmapItem *item_switch = ui->graph_all->scene()->addPixmap(image_switch);
	item_switch->setOffset(QPointF(288, 96));
	ui->graph_all->scene()->addItem(item_switch);

	image_ori->swap(QPixmap());
}
/*
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == ui->graph_all->viewport())
	{
		if (event->type() == QEvent::GraphicsSceneMousePress)//mouse button pressed
		{
			const QGraphicsSceneMouseEvent* const mouseEvent = static_cast<const QGraphicsSceneMouseEvent*>(event);
			if (mouseEvent->button() & Qt::LeftButton)
			{
				const QPointF position = mouseEvent->scenePos();
				if (position.x() > 0 && position.x() < 512)
				{
					btn_event = BTN_PRESS;
					return true;
				}
			}

		}
		else if (event->type() == QEvent::GraphicsSceneMouseRelease)
		{
			btn_event = BTN_RELEASE;
			return true;
		}
		else
			return false;
	}
	else
	{
		// pass the event on to the parent class
		return QMainWindow::eventFilter(obj, event);
	}
}
*/
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == ui->graph_all->viewport())
	{
		if (event->type() == QEvent::MouseButtonPress)//mouse button pressed
		{
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
			if (mouseEvent->button() == Qt::LeftButton)
			{
				const QPointF position = mouseEvent->pos();
				if (position.x() > 340 && position.x() < 510)
				{
					btn_event = BTN_PRESS;
					return true;
				}
			}

		}
		else if (event->type() == QEvent::MouseButtonRelease)
		{
			btn_event = BTN_RELEASE;
			return true;
		}
		else
			return false;
	}
	else
	{
		// pass the event on to the parent class
		return QMainWindow::eventFilter(obj, event);
	}
}

MainWindow::~MainWindow()
{
#ifndef OFFLINE	
	modbus_close(mb);
	modbus_free(mb);
#endif
	ui->graph_all->scene()->clear();
	delete ui;
}
