#include "solaredge_inverter.h"
#include "modbus_spy.h"

SolaredgeInverter::SolaredgeInverter(VeQItem *root, const DeviceInfo &deviceInfo,
					int deviceInstance, QObject *parent) :
	Inverter(root, deviceInfo, deviceInstance, parent),
	mModbusSpy(new ModbusSpy(root->itemGetOrCreate("ModbusSpy", false), this))
{
	// Using 'Enhanced Dynamic Power Control' we can set a double of value [0 - 100]
	// Enable the power limiter and initialise it to maxPower.
	const_cast<DeviceInfo&>(this->deviceInfo()).powerLimitScale = 100;
	setPowerLimit(deviceInfo.maxPower);
}
