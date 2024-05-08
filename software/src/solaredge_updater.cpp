#include "solaredge_updater.h"
#include "solaredge_inverter.h"
#include "modbus_tcp_client.h"
#include "logging.h"
#include "modbus_spy.h"

#include <QStringList>

// Extended classes relating to SolarEdge specific updating
// ========================================================
enum PowerControl : uint16_t {
	// Commit / Restore
	CommitPowerControlSettings = 0xF100,         //   Int16,              ?,     [-]  (Execute = 1)
	RestorePowerControlDefaultSettings = 0xF101, //   Int16,              ?,     [-]  (Execute = 1)

	// Global Dynamic Power Control (GDPC) Active & Reactive
	AdvancedPowerControlEnable = 0xF142,         //   Int32,          0 - 1,     [-]

	// GDPC - Active
	ActivePowerLimit = 0xF001,                   //  Uint16,        0 - 100,     [%]

	// GDPC - Reactive
	/* add if/when needed */

	// Enhanced Dynamic Power Control (EDPC) Active & Reactive
	EnableDynamicPowerControl = 0xF300,          //  Uint16,          0 - 1,     [-]
	CommandTimeout = 0xF310,                     //  Uint32, 0 - MAX_UINT32,     [s]  (Default = 60)

	// EDPC - Active
	MaxActivePower = 0xF304,                     // Float32,     Inv rating,     [W]
	FallbackActivePowerLimit = 0xF312,           // Float32,        0 - 100,     [%]  (Default = 100)
	ActivePowerRampUpRate = 0xF318,              // Float32,        0 - 100, [%/min]  (Default = 5, Disable = -1)
	ActivePowerRampDownRate = 0xF31A,            // Float32,        0 - 100, [%/min]  (Default = 5, Disable = -1)
	DynamicActivePowerLimit = 0xF322,            // Float32,        0 - 100,     [%]

	// EDPC - Reactive
	/* add if/when needed */
};

static constexpr float FallbackActivePowerLimitValue = 100; // [%] For timeout on inverter (e.g. communication failure)
static constexpr float PowerLimitDisableValue = 100;        // [%] For timeout on updating powerLimit (venus-os)

template <typename T, typename std::enable_if<std::is_arithmetic<T>::value, bool>::type = true>
QVector<uint16_t> toWords(T value) {
	QVector<uint16_t> words((sizeof(T)+1)/2);
	memcpy(words.data(), &value, sizeof(T));
	return words;
}

SolaredgeUpdater::SolaredgeUpdater(Inverter *inverter, InverterSettings *settings, QObject *parent):
	SunspecUpdater(inverter, settings, parent),
	mIncludeInitCommands(true)
{
	mExternalModbusRequest = std::make_shared<ExternalModbusRequest>();
	connect(modbusClient(), SIGNAL(connected()), this, SLOT(setIncludeInitCommands()));
	connect(modbusClient(), SIGNAL(connected()), this, SLOT(readMaxPower()));
	auto modbusSpy = reinterpret_cast<SolaredgeInverter*>(inverter)->modbusSpy();
	connect(modbusSpy, SIGNAL(modbusRequest(ExternalModbusRequest)), this, SLOT(onExternalModbusRequest(ExternalModbusRequest)));
	connect(this, SIGNAL(externalModbusReply(ExternalModbusReply)), modbusSpy, SLOT(onModbusReply(ExternalModbusReply)));
}

void SolaredgeUpdater::setIncludeInitCommands()
{
	mIncludeInitCommands = true;
}

void SolaredgeUpdater::writeCommands(bool firstCommand)
{
	if (!firstCommand) {
		ModbusReply *previousReply = static_cast<ModbusReply *>(sender());
		previousReply->deleteLater();
	}

	if (mCommands.isEmpty())
		return;

	auto cmd = mCommands.takeFirst();
	const DeviceInfo &deviceInfo = inverter()->deviceInfo();
	ModbusReply *reply = modbusClient()->writeMultipleHoldingRegisters(deviceInfo.networkId, cmd.first, cmd.second);

	// Use original onWriteCompleted when all commands are done
	if (mCommands.isEmpty())
		connect(reply, SIGNAL(finished()), this, SLOT(onWriteCompleted()));
	else
		connect(reply, SIGNAL(finished()), this, SLOT(writeCommands()));
}

void SolaredgeUpdater::writePowerLimit(double powerLimitPct)
{
	const DeviceInfo &deviceInfo = inverter()->deviceInfo();
	mCommands = {
		{DynamicActivePowerLimit,   toWords(static_cast<float>(powerLimitPct * deviceInfo.powerLimitScale))}
	};
	if (mIncludeInitCommands) {
		mIncludeInitCommands = false;
		// Set ramp rates to 100 first and then to -1/disable.
		// Last firmware for inverters with display (3.2537) doesn't allow -1/disable:
		//   - It will then stay at 100 [%/min] (fastest setting possible) for these inverters.
		mCommands.append({ActivePowerRampUpRate,     toWords(static_cast<float>(100))});
		mCommands.append({ActivePowerRampDownRate,   toWords(static_cast<float>(100))});
		mCommands.append({ActivePowerRampUpRate,     toWords(static_cast<float>(-1))});
		mCommands.append({ActivePowerRampDownRate,   toWords(static_cast<float>(-1))});
		mCommands.append({FallbackActivePowerLimit,  toWords(FallbackActivePowerLimitValue)});
		mCommands.append({CommandTimeout,            toWords(static_cast<uint32_t>(PowerLimitTimeout))});
		mCommands.append({EnableDynamicPowerControl, {1}});
	}
	writeCommands(true);
}

void SolaredgeUpdater::disablePowerLimiting()
{
	// Cancel limiter by setting DynamicActivePowerLimit to 100 [%].
	// This will cause the inverter to go to full power.
	const DeviceInfo &deviceInfo = inverter()->deviceInfo();
	mCommands = {
		{DynamicActivePowerLimit, toWords(PowerLimitDisableValue)}
	};
	writeCommands(true);
	inverter()->setPowerLimit(deviceInfo.maxPower);
}

void SolaredgeUpdater::readMaxPower()
{
	const DeviceInfo &deviceInfo = inverter()->deviceInfo();
	if (deviceInfo.maxPower == 0) {
		ModbusReply *reply = modbusClient()->readHoldingRegisters(deviceInfo.networkId, MaxActivePower, 2);
		connect(reply, SIGNAL(finished()), this, SLOT(onReadMaxPowerCompleted()));
	}
}

void SolaredgeUpdater::onReadMaxPowerCompleted()
{
	ModbusReply *reply = static_cast<ModbusReply *>(sender());
	reply->deleteLater();
	if (!reply->error()) {
		QVector <quint16> words = reply->registers();
		auto value = static_cast<double>(*reinterpret_cast<float *>(words.data()));
		if (value > 0) {
			auto seInverter = reinterpret_cast<SolaredgeInverter *>(inverter());
			qInfo() << "Setting 'Ac/MaxPower' and 'Ac/PowerLimit' for SolarEdge Inverter to" << value;
			seInverter->setMaxPower(value);
			seInverter->setPowerLimit(value);
		}
	}
}

void SolaredgeUpdater::onExternalModbusRequest(const ExternalModbusRequest& request)
{
	ExternalModbusReply externalReply{.error = ModbusReply::ExceptionCode::UnsupportedFunction, .values = {},
									  .info = ""};
	if (mProcessExternalModbusRequest) {
		// already busy
		externalReply.info = mExternalModbusRequest->info().append(" - busy");
		emit externalModbusReply(externalReply);
		return;
	}
	if (!request.address || !request.size) {
		externalReply.info = request.info().append(" - invalid address/size");
		emit externalModbusReply(externalReply);
		return;
	}
	mProcessExternalModbusRequest = true;
	*mExternalModbusRequest = request;
}

void SolaredgeUpdater::onExternalModbusRequestCompleted()
{
	ModbusReply *reply = static_cast<ModbusReply *>(sender());
	reply->deleteLater();
	ExternalModbusReply externalReply{.error = reply->error(), .values = reply->registers(),
									  .info = mExternalModbusRequest->info().append(" - ")};
	if (reply->error())
		externalReply.info.append(reply->toString().trimmed());
	else
		externalReply.info.append("succeeded");
	emit externalModbusReply(externalReply);
	mProcessExternalModbusRequest = false;
	SunspecUpdater::readPowerAndVoltage();
}

void SolaredgeUpdater::readPowerAndVoltage()
{
	// Process ModbusSpy stuff first (if any), then continue as normal
	if (mProcessExternalModbusRequest) {
		const DeviceInfo &deviceInfo = inverter()->deviceInfo();
		const auto& req = *mExternalModbusRequest;
		if (req.type == ExternalModbusRequest::Type::Read) {
			ModbusReply *reply = modbusClient()->readHoldingRegisters(deviceInfo.networkId, req.address, req.size);
			connect(reply, SIGNAL(finished()), this, SLOT(onExternalModbusRequestCompleted()));
			return; // SunspecUpdater::readPowerAndVoltage() at end of onExternalModbusRequestCompleted()
		} else if (req.type == ExternalModbusRequest::Type::Write) {
			ModbusReply *reply = modbusClient()->writeMultipleHoldingRegisters(deviceInfo.networkId, req.address, req.values);
			connect(reply, SIGNAL(finished()), this, SLOT(onExternalModbusRequestCompleted()));
			return; // SunspecUpdater::readPowerAndVoltage() at end of onExternalModbusRequestCompleted()
		}
	}
	SunspecUpdater::readPowerAndVoltage();
}
