#include "solaredge_inverter.h"
#include "modbus_spy.h"

SolaredgeInverter::SolaredgeInverter(VeQItem *root, const DeviceInfo &deviceInfo,
					int deviceInstance, QObject *parent) :
	Inverter(root, deviceInfo, deviceInstance, parent),
	mDevInfo(const_cast<DeviceInfo&>(Inverter::deviceInfo())), // We want to change some deviceInfo values
	mMaxPower(createItem("Ac/MaxPower")), // createItem returns existing (already created by Inverter base class)
	mModbusSpy(new ModbusSpy(root->itemGetOrCreate("ModbusSpy", false), this))
{
	// Using 'Enhanced Dynamic Power Control' we can set a double of value [0 - 100]
	// Enable the power limiter and initialise it to maxPower.
	mDevInfo.powerLimitScale = 100;
	setPowerLimit(mDevInfo.maxPower);
}

void SolaredgeInverter::setMaxPower(double p)
{
	mDevInfo.maxPower = p > 0 ? p : 0;
	produceDouble(mMaxPower, mDevInfo.maxPower, 0, "W");
}
