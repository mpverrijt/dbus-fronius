#ifndef MODBUS_SPY_H
#define MODBUS_SPY_H

#include "ve_service.h"

struct ExternalModbusRequest {
	enum class Type {
		Read,
		Write
	};
	Type type;
	quint16 address;
	quint16 size;
	QVector<quint16> values;

	QString info() const {
		auto infoStr = QString("%1 [%2]").arg(address, 4, 16, static_cast<QChar>('0')).arg(size).toUpper().prepend("0x");
		return infoStr.prepend(type == Type::Read ? "read " : "write ");
	};
};

struct ExternalModbusReply {
	int error;
	QVector<quint16> values;
	QString info;
};

class ModbusSpy : public VeService
{
	Q_OBJECT
public:
	explicit ModbusSpy(VeQItem *root, QObject *parent = 0);

	int handleSetValue(VeQItem *item, const QVariant &variant) override;

signals:
	void modbusRequest(const ExternalModbusRequest& request);

public slots:
	void onModbusReply(const ExternalModbusReply& reply);

protected:
	uint16_t address() const;

	void setAddress(uint16_t addr);

	uint8_t count() const;

	void setCount(uint8_t cnt);

	QVector<uint16_t> payloadWords() const;

	void setPayload(const QVector<uint16_t>& words);

	template <typename T, typename std::enable_if<std::is_arithmetic<T>::value, bool>::type = true>
	void setPayload(T value) {
		QVector<uint16_t> words((sizeof(T)+1)/2);
		memcpy(words.data(), &value, sizeof(T));
		setPayload(words);
	};

	void setCommand(QString command = "");

	QString status() const;

	void setStatus(QString status = "");

private:
	int handleSetPayload(VeQItem *item, const QVariant &variant);

	void producePayload();

	struct Payload {
		VeQItem *wordString;
		VeQItem *floatValue;
		VeQItem *intString;
		VeQItem *uIntString;

		QVector<uint16_t> words;
	};

	VeQItem *mAddress;
	VeQItem *mCount;
	VeQItem *mCommand;
	VeQItem *mStatus;
	Payload mPayload;

};

#endif // MODBUS_SPY_H
