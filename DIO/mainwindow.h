#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include <QTimer>
#include <QTime>
#include <QImage>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
	bool eventFilter(QObject *obj, QEvent *event);

private:
    Ui::MainWindow *ui;
	QTimer *timer;
	QTime time;
	void BUTTON(bool status);
	void SWITCH(bool status);
	void INIT();
	void LIGHT(bool status);

private slots:
	void MAIN();
	
};

#endif // MAINWINDOW_H
