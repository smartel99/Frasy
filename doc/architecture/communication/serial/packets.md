# Packets exchanged between Frasy and the Hardware

[Source Code](../../../../Frasy/src/utils/communication/serial/packet.h)

Packets are exchanged between Frasy and hardware devices using the following protocol:

| `0x16` | `0x01` | Header (18 bytes) | `0x02` | Payload (up to 254 bytes) | `0x03` | CRC | `0x04` |
|:------:|:------:|:-----------------:|:------:|:-------------------------:|:------:|:---:|:------:|

- Data is encoded to ASCII, meaning that the data `0xA5` will be transmitted as the string
  "A5".
- The start of the packet is indicated using the `SYN` ASCII character (`0x16`).
- The start of the header is indicated using the `SOH` ASCII character (`0x01`).
- The start of the payload section is indicated using the `STX` ASCII character (`0x02`).
- The end of the payload is indicated using the `ETX` ASCII character (`0x03`).
- The end of the packet is indicated using the `EOT` ASCII character (`0x04`).

All packets include a header. This header is composed of the following fields:

1. **Transaction ID** (`uint32_t`): ID of the transaction.\
   Multiple packets can have the same ID. This is used for identifying the destination of a
   response packet, or to identify a group of packets belonging to the same transaction.
2. **Command ID** (`uint16_t`): ID of the command.
3. **Modifiers** (`uint8_t`): Modifiers applied to the packet, acting as metadata.
    1. Bit 0 indicates whether the packet is a command (when 0) or a response to a command (when 1).
    2. Bit 1 indicates, when it is set, that the packet is complete and that the processing can take place.
4. **Block ID** (`uint8_t`): ID of the packet in the transaction.\
   Whilst it is always present, its value is only useful when a transaction is split across multiple packets.
5. **Payload Size** (`uint8_t`): Number of bytes (not the number of ASCII characters) contained in the payload.
