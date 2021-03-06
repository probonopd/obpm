#include "donation.h"

donationDialog::donationDialog(QWidget *parent) : QDialog(parent)
{
	setupUi(this);

	setWindowFlags(Qt::Tool);
	layout()->setSizeConstraint(QLayout::SetFixedSize);

	BTCexchangeRate = -1;

	show();
	activateWindow();
	move(parent->mapToGlobal(parent->rect().center()) - rect().center());
}

void donationDialog::on_horizontalSlider_valueChanged(int value)
{
	pushButton->setText(QString(tr("donate %1€ via %2")).arg(value).arg(comboBox->currentText()));

	if(comboBox->currentIndex() == 1)
	{
		lineEdit_payment2->setText(QString("%1 EUR = %2 BTC").arg(value).arg(BTCexchangeRate ? QString::number(horizontalSlider->value() * BTCexchangeRate, 'f', 8) : "?"));
	}
}

void donationDialog::on_comboBox_activated(const QString &text)
{
	pushButton->setText(QString(tr("donate %1€ via %2")).arg(horizontalSlider->value()).arg(text));

	if(comboBox->currentIndex() == 0)
	{
		lineEdit_payment1->setText(QByteArray::fromBase64("TGF6eVRAZ214Lm5ldA=="));
		lineEdit_payment1->setToolTip(tr("beneficiary email address"));
		lineEdit_payment2->setText("https://www.paypal.com");
		lineEdit_payment2->setToolTip(tr("transaction website"));

		label_qrcode->setPixmap(QPixmap(":/png/png/qr_pp.png"));
	}
	else
	{
		if(BTCexchangeRate == -1)
		{
			BTCexchangeRate = EUR2BTC();
		}

		lineEdit_payment1->setText(QByteArray::fromBase64("MUVqRDhOa2RCUDFBeUd5OWdod3VQcGc4Z1FFb29qM1E4aw=="));
		lineEdit_payment1->setToolTip(tr("beneficiary BitCoin address"));
		lineEdit_payment2->setText(QString("%1 EUR = %2 BTC").arg(horizontalSlider->value()).arg(BTCexchangeRate ? QString::number(horizontalSlider->value() * BTCexchangeRate, 'f', 8) : "?"));
		lineEdit_payment2->setToolTip(tr("current exchange rate"));

		label_qrcode->setPixmap(QPixmap(":/png/png/qr_bc.png"));

	}
}

void donationDialog::on_pushButton_clicked()
{
	if(comboBox->currentIndex() == 0)
	{
		QDesktopServices::openUrl(QString(QByteArray::fromBase64("aHR0cHM6Ly93d3cucGF5cGFsLmNvbS9jZ2ktYmluL3dlYnNjcj9jbWQ9X3hjbGljayZidXNpbmVzcz1MYXp5VEBnbXgubmV0Jml0ZW1fbmFtZT0lMSZhbW91bnQ9JTImY3VycmVuY3lfY29kZT1FVVI=")).arg("OBPM").arg(horizontalSlider->value()));

		QMessageBox::information(this, APPNAME, tr("The default web browser will be started now with the donation website.\n\nIf this fails please donate manually with the shown information."));
	}
	else if(comboBox->currentIndex() == 1)
	{
		QDesktopServices::openUrl(QString(QByteArray::fromBase64("Yml0Y29pbjolMT9hbW91bnQ9JTImbGFiZWw9TGF6eVQmbWVzc2FnZT0lMw==")).arg(lineEdit_payment1->text()).arg(horizontalSlider->value() * BTCexchangeRate).arg(APPNAME));

		QMessageBox::information(this, APPNAME, tr("The default Bitcoin client will be started now with the payment information.\n\nIf this fails please donate manually with the shown information."));
	}

	close();
}

double donationDialog::EUR2BTC()
{
	QNetworkAccessManager *mgr = new QNetworkAccessManager();
	QNetworkReply *rep = mgr->get(QNetworkRequest(QUrl("https://blockchain.info/de/tobtc?currency=EUR&value=1")));
	QElapsedTimer timeout;
	QByteArray raw;

	rep->ignoreSslErrors();

	lineEdit_payment2->setText(tr("getting current exchange rate..."));
	comboBox->setEnabled(false);
	horizontalSlider->setEnabled(false);
	pushButton->setEnabled(false);

	timeout.start();

	while(rep->isRunning())
	{
		QCoreApplication::processEvents();

		if(timeout.hasExpired(3000))
		{
			rep->abort();
		}
	}

	comboBox->setEnabled(true);
	horizontalSlider->setEnabled(true);
	pushButton->setEnabled(true);

	if(rep->error())
	{
		QMessageBox::warning(this, APPNAME, tr("Current exchange rate could not be determined!\n\nPlease specify the desired amount manually..."));

		return 0.0;
	}

	raw = rep->readAll();

	rep->deleteLater();

	return raw.toDouble();
}

void donationDialog::keyPressEvent(QKeyEvent *ke)
{
	if(ke->key() == Qt::Key_F1)
	{
		QMessageBox::information(this, APPNAME, tr("Help is not yet implemented..."));
	}

	QDialog::keyPressEvent(ke);
}
