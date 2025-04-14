#include <QDebug>
#include <QHeaderView>
#include <QProgressBar>
#include <QString>
#include <QStringList>

#include "progressdock.h"

ProgressDock::ProgressDock(QWidget* parent) :
        QDockWidget("Progress viewer", parent) {

	tree = new QTreeWidget();
	tree->setMinimumWidth(250);
	setWidget(tree);
	tree->resize(200, tree->height());

	tree->setAllColumnsShowFocus(true);
	tree->setColumnCount(2);
	tree->setHeaderHidden(true);
	tree->setUniformRowHeights(true);
	tree->header()->setStretchLastSection(false);
	tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	tree->setColumnWidth(1, 100);
}

void ProgressDock::startTask(const QString& name) {
	int existingTask = taskWithName(name);
	if (existingTask != -1) {
		qDebug() << "task" << name << "already exists";
		return;
	}
	QTreeWidgetItem* item = new QTreeWidgetItem(tree, QStringList(name));
	tree->addTopLevelItem(item);
	auto* bar = new QProgressBar();
	tree->setItemWidget(item, 1, bar);
	bar->setValue(0);
	Task task;
	task.item = item;
	task.name = name;
	task.timer.start();
	tasks << task;
	tree->scrollToBottom();
}

void ProgressDock::setProgress(const QString& name, int progress) {
	int task = taskWithName(name);
	if (task == -1) {
		qDebug() << "invalid task name" << name;
		return;
	}
	QTreeWidgetItem* item = tasks[task].item;
	auto* bar = static_cast<QProgressBar*>(tree->itemWidget(item, 1));
	bar->setValue(progress);
}

void ProgressDock::endTask(const QString& name) {
	int task = taskWithName(name);
	if (task == -1) {
		qDebug() << "invalid task name" << name;
		return;
	}
	QTreeWidgetItem* item = tasks[task].item;
	if (tree->itemWidget(item, 1) == nullptr) {
		qDebug() << "couldn't find progress bar to remove for task " << name;
	}
	tree->setItemWidget(item, 1, nullptr);
	item->setText(1, QString("%1 s")
				.arg(tasks[task].timer.elapsed() / 1000.0,
						0, 'f', 2, '0'));
	item->setTextAlignment(1, Qt::AlignmentFlag::AlignCenter);
}

void ProgressDock::reset() {
	for (auto task : tasks) {
		delete task.item;  // TODO does this work?
	}
	tasks.clear();
}

int ProgressDock::taskWithName(const QString& name) {
	for (int i = 0; i < tasks.size(); i++) {
		if (tasks[i].name == name) {
			return i;
		}
	}
	return -1;
}
