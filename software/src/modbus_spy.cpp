#include "modbus_spy.h"

#include <cinttypes>
#include <QString>
#include <QStringList>

auto constexpr readCommand = "read";
auto constexpr writeCommand = "write";

/// Helpers

template <typename T, typename std::enable_if<std::numeric_limits<T>::is_integer, bool>::type = true>
T stringToValue(QString string, T defaultValue) {
	bool valid[3]{};
	string = string.trimmed();
	auto value0 = string.toULongLong(&valid[0], 10); // Unsigned int, base 10
	auto value1 = string.toLongLong(&valid[1], 10);  // Int, base 10
	auto value2 = string.toULongLong(&valid[2], 16); // Unsigned int, base 16

	return static_cast<T>(valid[0] ? value0 : (valid[1] ? value1 : (valid[2] ? value2 : defaultValue)));
}

template <typename T, typename std::enable_if<std::numeric_limits<T>::is_integer, bool>::type = true>
QVector<T> stringToValues(QString string, T defaultValue) {
	QVector<T> values;
	QStringList stringList = string.split(",");
	for (const auto& s: stringList) {
		values.append(stringToValue(s, defaultValue));
	}
	return values;
}

template <typename T, typename std::enable_if<std::numeric_limits<T>::is_integer, bool>::type = true>
QString valueToHexString(T value) {
	return QString("%1").arg(value, 2*sizeof(T), 16, static_cast<QChar>('0')).toUpper().prepend("0x");
}

template <typename T, typename std::enable_if<std::numeric_limits<T>::is_integer, bool>::type = true>
QString valuesToHexString(const QVector<T>& values) {
	QStringList stringList;
	for (const auto& v : values) {
		stringList.append(valueToHexString(v));
	}
	return stringList.join(", ");
}

/// ModbusSpy

ModbusSpy::ModbusSpy(VeQItem *root, QObject *parent) :
	VeService(root, parent),
	mAddress(createItem("Address")),
	mCount(createItem("Cnt")),
	mCommand(createItem("Command")),
	mStatus(createItem("Status")),
	mPayload{.wordString = createItem("Payload/Words"),
			 .floatValue = createItem("Payload/Float"),
			 .intString = createItem("Payload/Int"),
			 .uIntString = createItem("Payload/UInt"),
			 .words = {}} {
	setCommand();
	setStatus();
}

int ModbusSpy::handleSetValue(VeQItem *item, const QVariant &variant)
{
	auto s = variant.toString();
	if (item == mAddress) {
		setAddress(stringToValue(s, 0));
		setCommand();
		setStatus();
		setPayload({});
	} else if (item == mCount) {
		setCount(stringToValue(s, 0));
		setCommand();
		setStatus();
		setPayload({});
	} else if (item == mCommand) {
		ExternalModbusRequest request = {.type = ExternalModbusRequest::Type::Read, .address = address(), .size = count(), .values = {}};
		if (s == readCommand) {
			setCommand(s);
			setStatus("reading");
			emit modbusRequest(request);
		} else if (s == writeCommand) {
			setCommand(s);
			setStatus("writing");
			request.type = ExternalModbusRequest::Type::Write;
			request.values = payloadWords();
			emit modbusRequest(request);
		}
	} else if (handleSetPayload(item, variant) != 0) {
		return VeService::handleSetValue(item, variant);
	}

	return 0;
}

uint16_t ModbusSpy::address() const
{
	return stringToValue(mAddress->getValue().toString(), 0);
}

void ModbusSpy::setAddress(uint16_t addr)
{
	produceValue(mAddress, addr ? valueToHexString(addr) : QVariant(), "");
}

uint8_t ModbusSpy::count() const
{
	return mCount->getValue().value<uint8_t>();
}

void ModbusSpy::setCount(uint8_t cnt)
{
	produceValue(mCount, cnt ? QVariant(cnt) : QVariant(), "");
}

void ModbusSpy::setCommand(QString cmd) {
	auto static const defaultMsg = QString("Commands: [%1, %2]").arg(readCommand, writeCommand);
	mCommand->produceValue(cmd.isEmpty() ? defaultMsg : QString("%1, last: %2").arg(defaultMsg, cmd),
						   VeQItem::State::Synchronized, true);
	mCommand->produceText("");
}

QString ModbusSpy::status() const
{
	return mStatus->getValue().toString();
}

void ModbusSpy::setStatus(QString status) {
	mStatus->produceValue(status.isEmpty() ? QVariant() : status, VeQItem::State::Synchronized, true);
	mStatus->produceText("");
}

void ModbusSpy::onModbusReply(const ExternalModbusReply& reply)
{
	if (!reply.error && !reply.values.isEmpty()) {
		// When writing values is empty
		setPayload(reply.values);
	}
	setStatus(reply.info);
}

QVector<uint16_t> ModbusSpy::payloadWords() const
{
	return mPayload.words;
}

void ModbusSpy::setPayload(const QVector<uint16_t>& words)
{
	mPayload.words = words;
	producePayload();
}

void ModbusSpy::producePayload()
{
	QString dText;
	QString uText;
	QVariant fValue;
	auto& words = mPayload.words;
	switch (words.size()) {
		case 1: {
			auto pd = reinterpret_cast<int16_t *>(words.data());
			dText.sprintf("%" PRId16, *pd);

			auto pu = reinterpret_cast<uint16_t *>(words.data());
			uText.sprintf("%" PRIu16, *pu);
			break;
		}
		case 2: {
			auto pd = reinterpret_cast<int32_t *>(words.data());
			dText.sprintf("%" PRId32, *pd);

			auto pu = reinterpret_cast<uint32_t *>(words.data());
			uText.sprintf("%" PRIu32, *pu);

			fValue = static_cast<double>(*reinterpret_cast<float *>(words.data()));
			break;
		}
		case 4: {
			auto pd = reinterpret_cast<int64_t *>(words.data());
			dText.sprintf("%" PRId64, *pd);

			auto pu = reinterpret_cast<uint64_t *>(words.data());
			uText.sprintf("%" PRIu64, *pu);
			break;
		}
		default:
			break;
	}

	auto hexString = valuesToHexString(words);
	produceValue(mPayload.wordString, hexString.isEmpty() ? QVariant() : hexString, "");
	produceValue(mPayload.floatValue, fValue, "");
	produceValue(mPayload.intString, dText.isEmpty() ? QVariant() : dText, "");
	produceValue(mPayload.uIntString, uText.isEmpty() ? QVariant() : uText, "");
}

int ModbusSpy::handleSetPayload(VeQItem *item, const QVariant &variant)
{
	auto s = variant.toString();
	if (item == mPayload.wordString) {
		auto words = stringToValues<uint16_t>(s, 0);
		setPayload(words);
	} else if (item == mPayload.floatValue) {
		auto f = variant.toFloat();
		setPayload(f);
	} else if (item == mPayload.intString) {
		switch(count()) {
			case 1:
				setPayload(stringToValue<int16_t>(s, 0));
				break;
			case 2:
				setPayload(stringToValue<int32_t>(s, 0));
				break;
			case 4:
			default:
				setPayload(stringToValue<int64_t>(s, 0));
				break;
		}
	} else if (item == mPayload.uIntString) {
		switch(count()) {
			case 1:
				setPayload(stringToValue<uint16_t>(s, 0));
				break;
			case 2:
				setPayload(stringToValue<uint32_t>(s, 0));
				break;
			case 4:
			default:
				setPayload(stringToValue<uint64_t>(s, 0));
				break;
		}
	} else {
		return -1;
	}

	setCount(mPayload.words.size());
	setCommand();
	setStatus();

	return 0;
}

