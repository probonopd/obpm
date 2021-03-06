#include "setup.h"

setupDialog::setupDialog(QWidget *parent, struct CONFIG *config) : QDialog(parent)
{
	setupUi(this);

	setWindowFlags(Qt::Tool);
	layout()->setSizeConstraint(QLayout::SetFixedSize);

	cfg = config;

	horizontalSlider_sys->setValue(cfg->sys);
	horizontalSlider_dia->setValue(cfg->dia);
	horizontalSlider_bpm->setValue(cfg->bpm);
	checkBox_legend->setChecked(cfg->legend);

	if(cfg->style == QCPGraph::lsLine)
	{
		radioButton_line->setChecked(true);
	}
	else if(cfg->style == QCPGraph::lsStepCenter)
	{
		radioButton_step->setChecked(true);
	}
	else if(cfg->style == QCPGraph::lsImpulse)
	{
		radioButton_impulse->setChecked(true);
	}
	else
	{
		radioButton_point->setChecked(true);
	}

	checkBox_update->setChecked(cfg->update);

	show();
	activateWindow();
	move(parent->mapToGlobal(parent->rect().center()) - rect().center());
}

void setupDialog::on_pushButton_reset_clicked()
{
	horizontalSlider_sys->setValue(SYS_NORM);
	horizontalSlider_dia->setValue(DIA_NORM);
	horizontalSlider_bpm->setValue(BPM_NORM);

	checkBox_legend->setChecked(true);
	radioButton_line->setChecked(true);

	checkBox_update->setChecked(true);
}

void setupDialog::on_pushButton_save_clicked()
{
	cfg->sys = horizontalSlider_sys->value();
	cfg->dia = horizontalSlider_dia->value();
	cfg->bpm = horizontalSlider_bpm->value();
	cfg->legend = checkBox_legend->isChecked();

	if(radioButton_line->isChecked())
	{
		cfg->style = QCPGraph::lsLine;
	}
	else if(radioButton_step->isChecked())
	{
		cfg->style = QCPGraph::lsStepCenter;
	}
	else if(radioButton_impulse->isChecked())
	{
		cfg->style = QCPGraph::lsImpulse;
	}
	else
	{
		cfg->style = QCPGraph::lsNone;
	}

	cfg->update = checkBox_update->isChecked();

	done(QDialog::Accepted);
}

void setupDialog::on_horizontalSlider_sys_valueChanged(int value)
{
	label_sys->setText(QString("%1").arg(value));
}

void setupDialog::on_horizontalSlider_dia_valueChanged(int value)
{
	label_dia->setText(QString("%1").arg(value));
}

void setupDialog::on_horizontalSlider_bpm_valueChanged(int value)
{
	label_bpm->setText(QString("%1").arg(value));
}

void setupDialog::keyPressEvent(QKeyEvent *ke)
{
	if(ke->key() == Qt::Key_F1)
	{
		QMessageBox::information(this, APPNAME, tr("Help is not yet implemented..."));
	}

	QDialog::keyPressEvent(ke);
}

void setupDialog::reject()
{
	if(QMessageBox::question(this, APPNAME, tr("Abort setup and discard all changes?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
	{
		done(QDialog::Rejected);
	}
}
