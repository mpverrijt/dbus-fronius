#ifndef INVERTER_MODBUS_UPDATER_H
#define INVERTER_MODBUS_UPDATER_H

#include <QObject>
#include <QList>
#include <QAbstractSocket>
#include <QString>

class DataProcessor;
class Inverter;
class InverterSettings;
class ModbusReply;
class ModbusTcpClient;
class QTimer;

extern const int PowerLimitTimeout;

class SunspecUpdater: public QObject
{
	Q_OBJECT
public:
	explicit SunspecUpdater(Inverter *inverter, InverterSettings *settings, QObject *parent = 0);

	virtual ~SunspecUpdater();

	static bool hasConnectionTo(QString host, int id);

signals:
	void connectionLost();

	void inverterModelChanged();

private slots:
	void onReadCompleted();

	void onWriteCompleted();

	void onPowerLimitRequested(double value);

	void onConnected();

	void onDisconnected();

	void onTimer();

	void onPowerLimitExpired();

	void onPhaseChanged();

protected:
	virtual void readPowerAndVoltage();

	virtual void writePowerLimit(double powerLimitPct);

	virtual void disablePowerLimiting();

	virtual bool parsePowerAndVoltage(QVector<quint16> values);

	Inverter *inverter() { return mInverter; }

	InverterSettings *settings() { return mSettings; }

	ModbusTcpClient *modbusClient() { return mModbusClient; }

	DataProcessor *processor() { return mDataProcessor; }

	void readHoldingRegisters(quint16 startRegister, quint16 count);

	void updateSplitPhase(double power, double energy);

	void setInverterState(int sunSpecState);

private:
	enum ModbusState {
		ReadPowerAndVoltage,
		WritePowerLimit,
		Idle
	};

	enum OperatingState {
		SunspecOff = 1,
		SunspecSleeping = 2,
		SunspecStarting = 3,
		SunspecMppt = 4,
		SunspecThrottled = 5,
		SunspecShutdown = 6,
		SunspecFault = 7,
		SunspecStandby = 8
	};

	void connectModbusClient();

	void startNextAction(ModbusState state);

	void startIdleTimer();

	void writeMultipleHoldingRegisters(quint16 startReg, const QVector<quint16> &values);

	bool handleModbusError(ModbusReply *reply);

	void handleError();

	Inverter *mInverter;
	InverterSettings *mSettings;
	ModbusTcpClient *mModbusClient;
	QTimer *mTimer;
	QTimer *mPowerLimitTimer;
	DataProcessor *mDataProcessor;
	ModbusState mCurrentState;
	double mPowerLimitPct;
	int mRetryCount;
	bool mWritePowerLimitRequested;
	static QList<SunspecUpdater*> mUpdaters; // to keep track of inverters we have a connection with
};

class FroniusSunspecUpdater : public SunspecUpdater
{
	Q_OBJECT
public:
	explicit FroniusSunspecUpdater(Inverter *inverter, InverterSettings *settings, QObject *parent = 0);
private:
	bool parsePowerAndVoltage(QVector<quint16> values) override;
};

class Sunspec2018Updater : public SunspecUpdater
{
	Q_OBJECT
public:
	explicit Sunspec2018Updater(Inverter *inverter, InverterSettings *settings, QObject *parent = 0);
private:
	void readPowerAndVoltage() override;

	void writePowerLimit(double powerLimitPct) override;

	void disablePowerLimiting() override;

	bool parsePowerAndVoltage(QVector<quint16> values) override;
};


#endif // INVERTER_MODBUS_UPDATER_H
