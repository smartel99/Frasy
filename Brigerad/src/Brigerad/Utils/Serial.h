/**
 * @file    Serial
 * @author  Samuel Martel
 * @p       https://github.com/smartel99
 * @date    9/20/2020 11:33:25 AM
 *
 * @brief
 ******************************************************************************
 * Copyright (C) 2020  Samuel Martel
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *****************************************************************************/
/**
 * https://github.com/wjwwood/serial
 * The MIT License
 *
 * Copyright (c) 2012 William Woodall, John Harrison
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

/*********************************************************************************************************************/
// [SECTION] Includes
/*********************************************************************************************************************/
#include "Brigerad.h"

#include "serial/serial.h"

namespace Brigerad
{
/*********************************************************************************************************************/
// [SECTION] Defines
/*********************************************************************************************************************/

/*********************************************************************************************************************/
// [SECTION] Types
/*********************************************************************************************************************/



/*********************************************************************************************************************/
// [SECTION] Class Declarations
/*********************************************************************************************************************/
class Serial
{
public:
    enum class Baudrates
    {
        Baud110,
        Baud300,
        Baud600,
        Baud1200,
        Baud2400,
        Baud4800,
        Baud9600,
        Baud14400,
        Baud19200,
        Baud38400,
        Baud56000,
        Baud57600,
        Baud115200,
        Baud128000,
        Baud153600,
        Baud230400,
        Baud256000,
        Baud460800,
        Baud500000,
        Baud921600
    };

    enum class ByteSizes
    {
        FiveBits  = 5,
        SixBits   = 6,
        SevenBits = 7,
        EightBits = 8
    };

    enum class Parities
    {
        None  = 0,
        Odd   = 1,
        Even  = 2,
        Mark  = 3,
        Space = 4
    };

    enum class StopBits
    {
        One = 1,
        Two = 2,
        OnePointFive
    };

    enum class FlowControls
    {
        None = 0,
        Software,
        Hardware
    };

    using Timeout  = serial::Timeout;
    using PortInfo = serial::PortInfo;

    /**
     * Creates a Serial object and opens the port if a port is specified,
     * otherwise it remains closed until Serial::open is called.
     *
     * \param port std::string containing the address of the serial port,
     *        which would be something like 'COM1' on Windows and '/dev/ttyS0'
     *        on Linux.
     * \param baudrate The baudrate to be used by the serial port.
     * \param timeout A Serial::Timeout struct that defines the timeout conditions
     *        of the serial port.
     * \param byteSize Size of each byte in the serial transmission of data,
     *        default is eight bits.
     * \param parity Method of parity, default is None.
     * \param stopBit Number of stop bits used, default is One.
     * \param flowControl Type of flow control used, default is none.
     *
     * \throw PortNotOpenedException
     * \throw IOException
     * \throw InvalidArgument
     */
    Serial(const std::string& port,
           Baudrates          baudrate    = Baudrates::Baud9600,
           Timeout            timeout     = Timeout(),
           ByteSizes          byteSize    = ByteSizes::EightBits,
           Parities           parity      = Parities::None,
           StopBits           stopBit     = StopBits::One,
           FlowControls       flowControl = FlowControls::None)
    : m_port(port,
             (uint32_t)baudrate,
             timeout,
             (serial::bytesize_t)byteSize,
             (serial::parity_t)parity,
             (serial::stopbits_t)stopBit,
             (serial::flowcontrol_t)flowControl)
    {
    }
    ~Serial() {}

    static std::vector<PortInfo> ListPorts() { return serial::list_ports(); }

    /**
     * Opens the serial port as long as the port is set and the port isn't
     * already open.
     *
     * If the port is provided to the constructor then an explicit call to open
     * is not needed.
     *
     * \return true if the port is successfully open.
     * \return false if the port couldn't be opened.
     *
     * \throw InvalidArgument
     * \throw SerialException
     * \throw IOException
     */
    bool Open()
    {
        m_port.open();
        return m_port.isOpen();
    }

    /**
     * Get the open status of the serial port.
     *
     * \return true if the port is open
     * \return false if the port is closed
     */
    bool IsOpen() const { return m_port.isOpen(); }

    /** close the serial port. */
    void Close() { m_port.close(); }

    /** Return the number of characters in the buffer */
    size_t BytesAvailable() { return m_port.available(); }

    /**
     * Block until there is serial data to read or read_timeout_constant number
     * of milliseconds have elapsed.
     *
     * \return true when the function exits with the port in a readable state
     * \return false otherwise (due to timeout or select interruption)
     */
    bool WaitForByte() { return m_port.waitReadable(); }

    /**
     * Block for a period of time corresponding to the transmission time of
     * nb characters at the current serial settings. This may be used in con-junction
     * with WaitReadable to read larger blocks of data from the port.
     *
     * \param nb The number of bytes to wait for.
     *
     * \note This method does not wait until nb bytes of data are received, but
     *       rather waits for the time it would take to receive nb bytes.
     *       There is thus no guarantee that nb bytes of data are received.
     */
    void WaitForBytes(size_t nb) { m_port.waitByteTimes(nb); }

    /**
     * Read a given amount of bytes from the serial port into a given buffer.
     *
     * The read function will return in one of three cases:
     *  - The number of requested bytes was read.
     *      - In this case, the number of bytes requested will match the size_t
     *        returned by this method.
     *  - A timeout occurred, in this case the number of bytes read will not
     *    match the amount requested, but no exception will be thrown. One of two
     *    possible timeouts occurred:
     *      - The inner byte timeout expired. This means that number of milliseconds
     *        elapsed between receiving bytes from the serial port exceeded the
     *        inter byte timeout.
     *      - The total timeout expired, which is calculated by multiplying the
     *        read timeout multiplier by the number of requested bytes and then
     *        added to the read timeout constant. If that total number of milliseconds
     *        elapses after the initial call to read, a timeout will occur.
     *  - An exception occurred, in this case an actual exception will be thrown.
     *
     * \param buffer A pointer to the output buffer. The memory must already be allocated.
     * \param size The number of bytes to read.
     * \return The number of bytes read.
     *
     * \throw PortNotOpenedException
     * \throw SerialException
     */
    size_t Read(uint8_t* buffer, size_t size) { return m_port.read(buffer, size); }

    /** Read a given amount of bytes from the serial port into a give buffer.
     *
     * \param buffer A reference to a std::vector of uint8_t.
     * \param size The number of bytes to read.
     *
     * \return The number of bytes read.
     *
     * \throw serial::PortNotOpenedException
     * \throw serial::SerialException
     */
    size_t Read(std::vector<uint8_t>& buffer, size_t size = 1) { return m_port.read(buffer, size); }

    /**
     * Read a given amount of bytes from the serial port into a given buffer.
     *
     * \param buffer A reference to a std::string.
     * \param size The number of bytes to read.
     *
     * \return The number of bytes read.
     *
     * \throw serial::PortNotOpenedException
     * \throw serial::SerialException
     */
    size_t Read(std::string& buffer, size_t size = 1) { return m_port.read(buffer, size); }

    /**
     * Read a given amount of bytes from the serial port into a given buffer.
     *
     * \param size The number of bytes to read.
     *
     * \return A std::string containing the data read from the port.
     *
     * \throw PortNotOpenedException
     * \throw SerialException
     */
    std::string Read(size_t size = 1) { return m_port.read(size); }

    /**
     * Read in a line or until a given delimiter has been processed.
     *
     * Reads from the serial port until a single line has been read.
     *
     * \param buffer A reference to a std::string used to store the data.
     * \param size The maximum length of a line, defaults to 65536.
     * \param eol The delimiter, defaults to "\n".
     *
     * \return The number of bytes read.
     *
     * \throw PortNotOpenedException
     * \throw SerialException
     */
    size_t ReadLine(std::string& buffer, size_t size = 65536, const std::string& eol = "\n")
    {
        return m_port.readline(buffer, size, eol);
    }

    /**
     * Read in a line or until a given delimiter has been processed.
     *
     * Reads from the serial port until a single line has been read.
     *
     * \param size The maximum length of a line, defaults to 65536.
     * \param eol The delimiter, defaults to "\n".
     *
     * \return A std::string containing the line received from the serial port.
     *
     * \throw PortNotOpenedException
     * \throw SerialException
     */
    std::string ReadLine(size_t size = 65536, const std::string& eol = "\n")
    {
        return m_port.readline(size, eol);
    }

    /**
     * Read in multiple lines until the serial port times out.
     *
     * This requires a timeout > 0 before it can be used.
     * It will read until a timeout occurs and return a list of strings.
     *
     * \param size The maximum number of lines that should be read, defaults to 65536.
     * \param eol A string that indicates the end of a line, defaults to "\n"
     *
     * \return A std::vector of std::string containing every lines read.
     *
     * \throw PortNotOpenedException
     * \throw SerialException
     */
    std::vector<std::string> ReadLines(size_t maxLines = 65536, const std::string& eol = "\n")
    {
        return m_port.readlines(maxLines, eol);
    }

    /**
     * Write a string to the serial port.
     *
     * \param data A pointer to the data to be written to the serial port.
     * \param size The number of bytes that are to be written to the serial port.
     *
     * \return The number of bytes that actually were written on the port.
     *
     * \throw PortNotOpenedException
     * \throw SerialException
     * \throw IOException
     */
    size_t Write(const uint8_t* data, size_t len) { return m_port.write(data, len); }

    /**
     * Write a string to the serial port.
     *
     * \param data A reference to a vector containing the data to be written.
     *
     * \return The number of bytes that actually were written on the port.
     *
     * \throw PortNotOpenedException
     * \throw SerialException
     * \throw IOException
     */
    size_t Write(const std::vector<uint8_t>& data) { return m_port.write(data); }

    /**
     * Write a string to the serial port.
     *
     * \param data A reference to a string containing the data to be written.
     *
     * \return The number of bytes that actually were written on the port.
     *
     * \throw PortNotOpenedException
     * \throw SerialException
     * \throw IOException
     */
    size_t Write(const std::string& data) { return m_port.write(data); }

    /**
     * Set the serial port identifier.
     *
     * \param port A string containing the address of the serial port, which would
     *             be something like "COM1" on Windows and "/dev/ttyS0" on Linux.
     *
     * \throw InvalidArgument
     */
    void SetPort(const std::string& port) { m_port.setPort(port); }

    /**
     * Get the serial port identifier.
     *
     * \see Serial::SetPort
     *
     * \throw InvalidArgument
     */
    std::string GetPort() const { return m_port.getPort(); }

    /**
     * Sets the timeout for reads and writes using the Timeout struct.
     *
     * There are two timeout conditions described here:
     *  * The inter byte timeout:
     *    * The inter_byte_timeout component of serial::Timeout defines the
     *      maximum amount of time, in milliseconds, between receiving bytes on
     *      the serial port that can pass before a timeout occurs.  Setting this
     *      to zero will prevent inter byte timeouts from occurring.
     *  * Total time timeout:
     *    * The constant and multiplier component of this timeout condition,
     *      for both read and write, are defined in serial::Timeout.  This
     *      timeout occurs if the total time since the read or write call was
     *      made exceeds the specified time in milliseconds.
     *    * The limit is defined by multiplying the multiplier component by the
     *      number of requested bytes and adding that product to the constant
     *      component.  In this way if you want a read call, for example, to
     *      timeout after exactly one second regardless of the number of bytes
     *      you asked for then set the read_timeout_constant component of
     *      serial::Timeout to 1000 and the read_timeout_multiplier to zero.
     *      This timeout condition can be used in conjunction with the inter
     *      byte timeout condition with out any problems, timeout will simply
     *      occur when one of the two timeout conditions is met.  This allows
     *      users to have maximum control over the trade-off between
     *      responsiveness and efficiency.
     *
     * Read and write functions will return in one of three cases.  When the
     * reading or writing is complete, when a timeout occurs, or when an
     * exception occurs.
     *
     * A timeout of 0 enables non-blocking mode.
     *
     * \param timeout A serial::Timeout struct containing the inter byte
     * timeout, and the read and write timeout constants and multipliers.
     */
    void SetTimeout(Timeout& timeout) { m_port.setTimeout(timeout); }

    /*! Sets the timeout for reads and writes. */
    void setTimeout(uint32_t inter_byte_timeout,
                    uint32_t read_timeout_constant,
                    uint32_t read_timeout_multiplier,
                    uint32_t write_timeout_constant,
                    uint32_t write_timeout_multiplier)
    {
        Timeout timeout(inter_byte_timeout,
                        read_timeout_constant,
                        read_timeout_multiplier,
                        write_timeout_constant,
                        write_timeout_multiplier);
        return SetTimeout(timeout);
    }

    /**
     * Get the timeout for reads in seconds.
     *
     * \return A Timeout struct containing the inter_byte_timeout, and read and write
     *         timeout constants and multipliers.
     */
    Timeout GetTimeout() const { return m_port.getTimeout(); }

    /**
     * Sets the baudrate for the serial port.
     *
     * \param baudrate An integer that sets the baud rate for the serial port.
     *
     * \throw InvalidArgument
     */
    void SetBaudrate(Baudrates baudrate) { m_port.setBaudrate((uint32_t)baudrate); }

    /**
     * Get the baudrate for the serial port.
     *
     * \return The baud rate of the serial port.
     *
     * \see Serial::SetBaudrate
     *
     * \throw InvalidArguments
     */
    Baudrates GetBaudrate() const { return (Baudrates)m_port.getBaudrate(); }

    /**
     * Set the byte size for the serial port.
     *
     * \param size Size of each byte in the serial transmission of data.
     *             Defaults to 8 bits.
     *
     * \throw InvalidArgument
     */
    void SetByteSize(ByteSizes size) { m_port.setBytesize((serial::bytesize_t)size); }

    /**
     * Get the byte size for the serial port.
     *
     * \see Serial::SetByteSize
     *
     * \throw InvalidArgument
     */
    ByteSizes GetByteSize() const { return (ByteSizes)m_port.getBytesize(); }

    /**
     * Set the parity for the serial port.
     *
     * \param parity Method of parity. Defaults to None.
     *
     * \throw InvalidArgument
     */
    void SetParity(Parities parity) { m_port.setParity((serial::parity_t)parity); }

    /**
     * Get the type of parity used by the serial port.
     *
     * \see Serial::SetParity
     *
     * \throw InvalidArgument
     */
    Parities GetParity() const { return (Parities)m_port.getParity(); }

    /**
     * Set the number of stop bits for the serial port.
     *
     * \param stopbits Number of stop bits used. Defaults to 1.
     *
     * \throw InvalidArgument
     */
    void SetStopBits(StopBits stopbits) { m_port.setStopbits((serial::stopbits_t)stopbits); }

    /**
     * Get the number of stop bits by the serial port.
     *
     * \see Serial::SetStopBits
     *
     * \throw InvalidArgument
     */
    StopBits GetStopBits() const { return (StopBits)m_port.getStopbits(); }

    /**
     * Set the method of flow control used by the serial port.
     *
     * \param method The type of flow control used. Defaults to none.
     *
     * \throw InvalidArgument
     */
    void SetFlowControl(FlowControls method)
    {
        m_port.setFlowcontrol((serial::flowcontrol_t)method);
    }

    /**
     * Get the method of flow control used by the serial port.
     *
     * \see Serial::SetFlowControl
     *
     * \throw InvalidArgument
     */
    FlowControls GetFlowControl() const { return (FlowControls)m_port.getFlowcontrol(); }

    /**
     * Flush the input and output buffers.
     */
    void Flush() { m_port.flush(); }

    /**
     * Flush only the input buffer.
     */
    void FlushInput() { m_port.flushInput(); }

    /**
     * Flush only the output buffer.
     */
    void FlushOutput() { m_port.flushOutput(); }

    /**
     * Sends the RS-232 break signal.
     *
     * See tcsendbreak(3).
     */
    void SendBreak(int duration) { m_port.sendBreak(duration); }

    /**
     * Set the break condition to a given level.
     * Defaults to true.
     */
    void SetBreak(bool level = true) { m_port.setBreak(level); }

    /**
     * Set the RTS handshaking line to the given level.
     * Defaults to true.
     */
    void SetRTS(bool level = true) { m_port.setRTS(level); }

    /**
     * Set the DTR handshaking line to the given level.
     * Defaults to true.
     */
    void SetDTR(bool level = true) { m_port.setDTR(level); }

    /**
     * Blocks until CTS, DSR, RI or CD changes or something interrupts it.
     *
     * Can throw an exception if an error occurs while waiting.
     * You can check the status of CTS, DSR, RI and CD once this method returns.
     * Uses TIOCMIWAIT via ioctl if available (mostly on Linux) with a resolution
     * of less than +/-1ms and as good as +/-0.2ms.
     * Otherwise a polling method is used which can give a +/-2ms resolution.
     *
     * \return true if one of the lines changed.
     * \return false if something else occurred.
     *
     * \throw SerialException
     */
    bool WaitForChange() { return m_port.waitForChange(); }

    /**
     * Get the current status of the CTS line.
     */
    bool GetCTS() { return m_port.getCTS(); }

    /**
     * Get the current status of the DSR line.
     */
    bool GetDSR() { return m_port.getDSR(); }

    /**
     * Get the current status of the RI line.
     */
    bool GetRI() { return m_port.getRI(); }

    /**
     * Get the current status of the CD line.
     */
    bool GetCD() { return m_port.getCD(); }


private:
    serial::Serial m_port;
};
}    // namespace Brigerad
