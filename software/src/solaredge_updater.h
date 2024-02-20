#ifndef SOLAREDGE_MODBUS_UPDATER_H
#define SOLAREDGE_MODBUS_UPDATER_H

#include "sunspec_updater.h"
#include <memory>

class ExternalModbusReply;
class ExternalModbusRequest;

class SolaredgeUpdater : public SunspecUpdater
{
	Q_OBJECT
public:
	explicit SolaredgeUpdater(Inverter *inverter, InverterSettings *settings, QObject *parent = 0);

signals:
	void externalModbusReply(const ExternalModbusReply& reply);

private slots:
	void writeCommands(bool firstCommand = false);

	void setIncludeInitCommands();

	void onExternalModbusRequest(const ExternalModbusRequest& request);

	void onExternalModbusRequestCompleted();

private:
	void readPowerAndVoltage() override;

	void writePowerLimit(double powerLimitPct) override;

	void disablePowerLimiting() override;

	QList<std::pair<uint16_t, QVector<uint16_t>>> mCommands;

	bool mIncludeInitCommands;

	bool mProcessExternalModbusRequest{};

	std::shared_ptr<ExternalModbusRequest> mExternalModbusRequest;
};
#endif // SOLAREDGE_MODBUS_UPDATER_H
