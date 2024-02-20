#ifndef SOLAREDGE_MODBUS_UPDATER_H
#define SOLAREDGE_MODBUS_UPDATER_H

#include "sunspec_updater.h"

class SolaredgeUpdater : public SunspecUpdater
{
	Q_OBJECT
public:
	explicit SolaredgeUpdater(Inverter *inverter, InverterSettings *settings, QObject *parent = 0);

private slots:
	void writeCommands(bool firstCommand = false);

	void setIncludeInitCommands();

private:
	void writePowerLimit(double powerLimitPct) override;

	void disablePowerLimiting() override;

	QList<std::pair<uint16_t, QVector<uint16_t>>> mCommands;

	bool mIncludeInitCommands;
};
#endif // SOLAREDGE_MODBUS_UPDATER_H
