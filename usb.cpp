#include "usb.h"

unsigned char cmd_init[] = { 0x02, 0x08, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x18 };
unsigned char cmd_data[] = { 0x02, 0x08, 0x01, 0x00, 0x02, 0xac, 0x28, 0x00, 0x8f };
unsigned char cmd_done[] = { 0x02, 0x08, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07 };
unsigned char cmd_fail[] = { 0x02, 0x08, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x08 };

unsigned char rawdata[64];
unsigned char payload[40*70];

usbDialog::usbDialog(QWidget *parent) : QDialog(parent)
{
	setupUi(this);

	setWindowFlags(Qt::Tool);
	layout()->setSizeConstraint(QLayout::SetFixedSize);

	connect(pushButton_cancel, SIGNAL(clicked()), this, SLOT(reject()));

	scanHID();

	import = false;

	activateWindow();
	move(parent->mapToGlobal(parent->rect().center()) - rect().center());
}

int usbDialog::buildCRC(unsigned char* data)
{
	int crc = 0;
	int len = data[1];

	while(--len)
	{
		crc ^= data[len];
	}

	return crc;
}

void usbDialog::dumpRawData(bool out, int bytes, unsigned char* data)
{
	if(toolButton_dump->isChecked())
	{
		if(dumpfile.open(QIODevice::Append))
		{
			dumpfile.write(QString("%1 %2:").arg(out ? "->":"<-").arg(bytes, 2, 16, QChar('0')).toUpper().toUtf8());

			if(bytes)
			{
				for(int i = 0; i < data[1] + 1; i++)
				{
					dumpfile.write(QString(" %1").arg(data[i], 2, 16, QChar('0')).toUpper().toUtf8());
				}
			}
			else
			{
				dumpfile.write(" Timeout!");
			}

			dumpfile.write("\n");

			dumpfile.close();
		}
	}
}

bool usbDialog::readRawData()
{
	unsigned short addr = 0x02AC;
	int ptr = 0;
	wchar_t serial[10 + 1] = { 0 };

	di.ser.toWCharArray(serial);

	if((dev = hid_open(di.vid, di.pid, serial)) == 0)
	{
		QMessageBox::critical(this, APPNAME, tr("Could not open selected device!"));

		return false;
	}

	dumpfile.setFileName(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + "import.log");
	dumpfile.remove();

	if(!sendCMD(cmd_init))
	{
		hid_close(dev);

		QMessageBox::warning(this, APPNAME, tr("Press START/STOP button on device and try again..."));

		return false;
	}

	import = true;
	abort = false;

	comboBox->setEnabled(false);
	pushButton_import->setEnabled(false);
	toolButton_dump->setEnabled(false);

	progressBar->setMaximum(70);

	for(int i = 0; i < 70; i++)
	{
		if(abort)
		{
			sendCMD(cmd_fail);

			hid_close(dev);

			done(QDialog::Rejected);

			return false;
		}

		cmd_data[4] = addr >> 8;
		cmd_data[5] = addr & 0xFF;
		cmd_data[8] = buildCRC(cmd_data);

		sendCMD(cmd_data);

		memcpy(&payload[ptr], &rawdata[7], 40);

		ptr += 40;
		addr += 0x28;

		progressBar->setValue(i);

		QApplication::processEvents();
	}

	sendCMD(cmd_done);

	hid_close(dev);

	return true;
}

int usbDialog::sendCMD(unsigned char *cmd)
{
	int rc;

	rc = hid_write(dev, cmd, 1 + cmd[1]);

	dumpRawData(true, rc, cmd);

	rc = hid_read_timeout(dev, rawdata, sizeof(rawdata), 1000);

	dumpRawData(false, rc, rawdata);

	return rc;
}

void usbDialog::scanHID()
{
	hid_device_info *devi, *curi;
	hids_found = 0;

	if((devi = hid_enumerate(0, 0)))
	{
		curi = devi;

		while(curi)
		{
			devinfo di;
			QVariant data;

			di.vid = curi->vendor_id;
			di.pid = curi->product_id;
			di.man = QString::fromWCharArray(curi->manufacturer_string);
			di.prd = QString::fromWCharArray(curi->product_string);
			di.ser = QString::fromWCharArray(curi->serial_number);

			data.setValue(di);

			if(di.vid == VID && di.pid == PID)
			{
				comboBox->addItem(QIcon(":/png/png/usb.png"), QString("%1:%2 [ M500IT, %3 ]").arg(di.vid, 4, 16, QChar('0')).arg(di.pid, 4, 16, QChar('0')).arg(di.ser).toUpper(), data);

				hids_found++;
			}

			curi = curi->next;
		}

		hid_free_enumeration(devi);
	}
}

void usbDialog::on_comboBox_currentIndexChanged(int index)
{
	di = comboBox->itemData(index).value<devinfo>();

	label_man->setText(": " + (di.man.isEmpty() ? "-" : di.man));
	label_prd->setText(": " + (di.prd.isEmpty() ? "-" : di.prd));
	label_ser->setText(": " + (di.ser.isEmpty() ? "-" : di.ser));
}

void usbDialog::on_pushButton_import_clicked()
{
	HEALTHDATA set;

	if(!readRawData())
	{
		return;
	}

	user1 = 0;
	user2 = 0;

	for(int i = 0; i < 14*100; i += 14)
	{
		if((payload[i] != 0xFF) & (payload[i + 1] != 0xFF) && (payload[i + 2] != 0xFF))
		{
			user1++;
		}

		if((payload[i + 14*100] != 0xFF) & (payload[i + 1 + 14*100] != 0xFF) && (payload[i + 2 + 14*100] != 0xFF))
		{
			user2++;
		}
	}

	for(int i = 0; i < 14*user1; i += 14)
	{
		set.time = QDateTime(QDate(2000 + payload[i + 2], (payload[i + 4]>>2) & 0x0F, ((payload[i + 4]<<8 | payload[i + 5])>>5) & 0x1F), QTime(payload[i + 5] & 0x1F, ((payload[i + 6]<<8 | payload[i + 7])>>6) & 0x3F)).toTime_t();
		set.sys = payload[i + 1] + 25;
		set.dia = payload[i];
		set.bpm = payload[i + 3];

		((MainWindow*)parent())->healthdata[0].append(set);
	}

	for(int i = 0; i < 14*user2; i += 14)
	{
		set.time = QDateTime(QDate(2000 + payload[i + 2 + 14*100], (payload[i + 4 + 14*100]>>2) & 0x0F, ((payload[i + 4 + 14*100]<<8 | payload[i + 5 + 14*100])>>5) & 0x1F), QTime(payload[i + 5 + 14*100] & 0x1F, ((payload[i + 6 + 14*100]<<8 | payload[i + 7 + 14*100])>>6) & 0x3F)).toTime_t();
		set.sys = payload[i + 1] + 25;
		set.dia = payload[i];
		set.bpm = payload[i + 3];

		((MainWindow*)parent())->healthdata[1].append(set);
	}

	accept();
}

void usbDialog::keyPressEvent(QKeyEvent *ke)
{
	if(ke->key() == Qt::Key_F1)
	{
		QMessageBox::information(this, APPNAME, tr("Help is not yet implemented..."));
	}

	QDialog::keyPressEvent(ke);
}

void usbDialog::reject()
{
	if(import)
	{
		if(QMessageBox::question(this, APPNAME, tr("Really abort import?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
		{
			abort = true;
		}
	}
	else
	{
		done(QDialog::Rejected);
	}
}
