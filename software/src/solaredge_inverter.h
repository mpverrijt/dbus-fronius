#ifndef SOLAREDGE_INVERTER_H
#define SOLAREDGE_INVERTER_H

#include "inverter.h"

class ModbusSpy;

class SolaredgeInverter : public Inverter
{
	Q_OBJECT
public:
	SolaredgeInverter(VeQItem *root, const DeviceInfo &deviceInfo, int deviceInstance, QObject *parent = 0);

	void setMaxPower(double p);

	ModbusSpy* modbusSpy() { return mModbusSpy; };

private:
	DeviceInfo& mDevInfo;
	VeQItem *mMaxPower;
	ModbusSpy *mModbusSpy;
};

#endif // SOLAREDGE_INVERTER_H
