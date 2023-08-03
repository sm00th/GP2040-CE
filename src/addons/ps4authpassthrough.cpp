/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2023 Artem Savkov
 */

#include "addons/ps4authpassthrough.h"
#include "buzzerspeaker.h"
#include "class/hid/hid.h"
#include "config.pb.h"
#include "hardware/uart.h"
#include "helper.h"
#include "storagemanager.h"
#include "gamepad.h"
#include "CRC32.h"

#include "ps4_driver.h"
#include "tusb_types.h"

uint8_t PS4AuthPassThroughAddon::countBits(uint8_t data)
{
        int ret = 0;
        while (data > 0) {
                if (data & 1)
                        ret++;
                data >>= 1;
        }

        return ret;
}

uart_parity_t PS4AuthPassThroughAddon::getParity(uint8_t data, bool isCmd)
{
        uart_parity_t retval;
        uint8_t numBits = countBits(data);

        if  (isCmd) {
                retval = (numBits % 2) == 0 ? UART_PARITY_ODD : UART_PARITY_EVEN;
        } else {
                retval = (numBits % 2) == 0 ? UART_PARITY_EVEN : UART_PARITY_ODD;
        }

        return retval;
}

void PS4AuthPassThroughAddon::ch375SendCmd(CH375Cmd cmd)
{
        uart_set_format(uart0, 8, 1, getParity(cmd, true));
        uart_putc_raw(uart0, cmd);
}

void PS4AuthPassThroughAddon::ch375SendData(uint8_t data)
{
        uart_set_format(uart0, 8, 1, getParity(data, false));
        uart_putc_raw(uart0, data);
}

uint8_t PS4AuthPassThroughAddon::ch375GetChar(void)
{
        return uart_getc(uart0);
}


void PS4AuthPassThroughAddon::ch375Read(uint8_t *buf, uint8_t len)
{
        for (uint8_t i=0; i < len; i++) {
                buf[i] = ch375GetChar();
        }
}

bool PS4AuthPassThroughAddon::ch375SetUsbMode(CH375UsbMode mode)
{
        ch375SendCmd(CH375Cmd::setUsbMode);
        ch375SendData(mode);

        if (ch375GetChar() != CH375Retcode::success) {
                return false;
        }
        return true;
}

bool PS4AuthPassThroughAddon::ch375SetBaudRate(uint32_t baudRate)
{
        uint8_t coefficient = 0x00;
        uint8_t constant    = 0x00;

        switch(baudRate) {
                case 9600:
                        coefficient = 0x02;
                        constant = 0xb2;
                        break;
                case 19200:
                        coefficient = 0x02;
                        constant = 0xd9;
                        break;
                case 57600:
                        coefficient = 0x03;
                        constant = 0x98;
                        break;
                case 115200:
                        coefficient = 0x03;
                        constant = 0xcc;
                        break;
                default:
                        return false;
        };
        ch375SendCmd(CH375Cmd::setBaudRate);
        ch375SendData(coefficient);
        ch375SendData(constant);
        uart_tx_wait_blocking(uart0);

        uart_set_baudrate(uart0, baudRate);

        return ch375GetChar() != CH375Retcode::success;
}

bool PS4AuthPassThroughAddon::ch375CheckExists(void)
{
        uint8_t testByte = rand();
        ch375SendCmd(CH375Cmd::checkExists);
        ch375SendData(testByte);
        uart_tx_wait_blocking(uart0);

        return ch375GetChar() == (~testByte & 0xff);
}

bool PS4AuthPassThroughAddon::ch375IssueToken(uint8_t epAddr, CH375TokenPid pid)
{
        ch375SendCmd(CH375Cmd::issueToken);
        ch375SendData(epAddr << 4 | pid);
        sleep_us(5);

        ch375SendCmd(CH375Cmd::getStatus);
        return ch375GetChar() == CH375IntStatus::intSuccess;
}

bool PS4AuthPassThroughAddon::ch375SendDataRequest(tusb_control_request_t *request, uint8_t *data, uint16_t dataSize)
{
        uint8_t len = 0;

        ch375SendCmd(CH375Cmd::wrUsbData);
        ch375SendData(sizeof(*request));

        for (uint16_t i = 0; i < sizeof(*request); i++) {
                ch375SendData((reinterpret_cast<uint8_t*>(request))[i]);
        }

        ch375SendCmd(CH375Cmd::setEndp7);
        ch375SendData(CH375WMode::data0);
        sleep_us(6);

        if (!ch375IssueToken(DS4Endpoint::epControl, CH375TokenPid::setup)) {
                return false;
        }

        ch375SendCmd(CH375Cmd::wrUsbData);
        ch375SendData(dataSize);
        for (uint16_t i = 0; i < dataSize; i++) {
                ch375SendData(data[i]);
        }

        ch375SendCmd(CH375Cmd::setEndp7);
        ch375SendData(CH375WMode::data1);
        sleep_us(6);

        if (!ch375IssueToken(DS4Endpoint::epControl, CH375TokenPid::out)) {
                return false;
        }

        ch375SendCmd(CH375Cmd::setEndp6);
        ch375SendData(CH375WMode::data1);
        sleep_us(6);
        if (!ch375IssueToken(DS4Endpoint::epControl, CH375TokenPid::in)) {
                return false;
        }

        return true;
}

bool PS4AuthPassThroughAddon::ch375SendRequest(tusb_control_request_t *request, uint8_t *buf)
{
        uint8_t len = 0;

        ch375SendCmd(CH375Cmd::wrUsbData);
        ch375SendData(sizeof(*request));

        for (uint16_t i = 0; i < sizeof(*request); i++) {
                ch375SendData((reinterpret_cast<uint8_t*>(request))[i]);
        }

        ch375SendCmd(CH375Cmd::setEndp7);
        ch375SendData(CH375WMode::data0);
        sleep_us(6);

        if (!ch375IssueToken(DS4Endpoint::epControl, CH375TokenPid::setup)) {
                return false;
        }

        ch375SendCmd(CH375Cmd::setEndp6);
        ch375SendData(CH375WMode::data1);
        sleep_us(6);

        if (!ch375IssueToken(DS4Endpoint::epControl, CH375TokenPid::in)) {
                return false;
        }

        ch375SendCmd(CH375Cmd::rdUsbData0);
        len = ch375GetChar();
        if (len != request->wLength) {
                return false;
        }

        ch375Read(buf, len);

        ch375SendCmd(CH375Cmd::wrUsbData);
        ch375SendData(0x00);
        ch375SendCmd(CH375Cmd::setEndp7);
        ch375SendData(CH375WMode::data1);
        sleep_us(6);
        if (!ch375IssueToken(DS4Endpoint::epControl, CH375TokenPid::out)) {
                return false;
        }

        return true;
}

bool PS4AuthPassThroughAddon::ch375GetSignState(void)
{
        struct signState state;
        tusb_control_request_t signStateRequest = {
                .bmRequestType = 0b10100001,
                .bRequest      = HID_REQ_CONTROL_GET_REPORT,
                .wValue        = static_cast<uint16_t>(HID_REPORT_TYPE_FEATURE << 8 | 0xf2),
                .wIndex        = DS4Endpoint::epControl,
                .wLength       = sizeof(state)
        };

        ch375SendRequest(&signStateRequest, reinterpret_cast<uint8_t*>(&state));
        return state.state == 0x00;
}

bool PS4AuthPassThroughAddon::ch375GetSignatureChunk(uint8_t *authBuffer)
{
        struct signResponse response;
        tusb_control_request_t signChunkRequest = {
                .bmRequestType = 0b10100001,
                .bRequest      = HID_REQ_CONTROL_GET_REPORT,
                .wValue        = static_cast<uint16_t>(HID_REPORT_TYPE_FEATURE << 8 | 0xf1),
                .wIndex        = DS4Endpoint::epOut,
                .wLength       = sizeof(response)
        };

        ch375SendRequest(&signChunkRequest, reinterpret_cast<uint8_t*>(&response));
        memcpy(authBuffer + (response.reportCounter * DS4_CHUNK_SIZE),
               response.data, DS4_CHUNK_SIZE);
        return true;
}

bool PS4AuthPassThroughAddon::ch375GetSignature(uint8_t *authBuffer)
{
        for (uint8_t i = 0; i <= 0x12; i++) {
                if (!ch375GetSignatureChunk(authBuffer)) {
                        return false;
                }
        }
        return true;
}

bool PS4AuthPassThroughAddon::ch375SetChallengeChunk(struct signChallenge *chunk)
{
        tusb_control_request_t signChallengeRequest = {
                .bmRequestType = 0b00100001,
                .bRequest      = HID_REQ_CONTROL_SET_REPORT,
                .wValue        = static_cast<uint16_t>(HID_REPORT_TYPE_FEATURE << 8 | 0xf0),
                .wIndex        = DS4Endpoint::epOut,
                .wLength       = sizeof(*chunk)
        };

        return ch375SendDataRequest(&signChallengeRequest, reinterpret_cast<uint8_t*>(chunk), sizeof(*chunk));
}

bool PS4AuthPassThroughAddon::ch375GetDateTime(void)
{
        uint8_t buf[49];
        tusb_control_request_t datetimeRequest = {
                .bmRequestType = 0b10100001,
                .bRequest      = HID_REQ_CONTROL_GET_REPORT,
                .wValue        = static_cast<uint16_t>(HID_REPORT_TYPE_FEATURE << 8 | 0xa3),
                .wIndex        = DS4Endpoint::epOut,
                .wLength       = sizeof(buf)
        };

        ch375SendRequest(&datetimeRequest, buf);
        return true;
}

bool PS4AuthPassThroughAddon::ch375GetStatus(void)
{
        uint8_t buf[2];
        tusb_control_request_t datetimeRequest = {
                .bmRequestType = 0x80,
                .bRequest      = 0x00,
                .wValue        = 0x00,
                .wIndex        = DS4Endpoint::epControl,
                .wLength       = sizeof(buf)
        };

        ch375SendRequest(&datetimeRequest, buf);
        return true;
}

bool PS4AuthPassThroughAddon::ch375SendChallenge(void)
{
        uint8_t *nonceBuffer = PS4Data::getInstance().nonce_buffer;
        uint8_t copyBytes = DS4_CHUNK_SIZE;
        struct signChallenge challenge;
        memset(&challenge, 0, sizeof(challenge));
        challenge.reportId = 0xf0;
        challenge.sequenceCounter = sequenceCounter;
        for (uint8_t reportCounter = 0; reportCounter <= 0x04; reportCounter++) {
                challenge.reportCounter = reportCounter;
                if (reportCounter == 0x04) {
                        copyBytes = 32;
                        memset(challenge.data, 0, DS4_CHUNK_SIZE);
                }
                memcpy(challenge.data,
                       nonceBuffer + (reportCounter * DS4_CHUNK_SIZE),
                       copyBytes);
                challenge.crc32 =  CRC32::calculate(reinterpret_cast<uint8_t*>(&challenge), sizeof(challenge) - 4);
                if (!ch375SetChallengeChunk(&challenge)) {
                        return false;
                }
        }
        return true;
}

void PS4AuthPassThroughAddon::ch375TestConnect(void)
{
        CH375IntStatus newStatus;
        ch375SendCmd(CH375Cmd::testConnect);

        newStatus = static_cast<CH375IntStatus>(ch375GetChar());

        if (curIntStatus != newStatus) {
                switch(newStatus) {
                        case CH375IntStatus::disconnect:
                                ch375SetUsbMode(CH375UsbMode::hostEnabled);
                                break;
                        case CH375IntStatus::connect:
                                uint8_t len;
                                if (!ch375SetUsbMode(CH375UsbMode::hostEnabledReset)) {
                                        break;
                                }
                                if (!ch375SetUsbMode(CH375UsbMode::hostEnabledSOF)) {
                                        break;
                                }
                                ch375SendCmd(CH375Cmd::getDescr);
                                ch375SendData(CH375_USB_DESCRIPTOR_DEVICE);
                                sleep_us(10);
                                ch375SendCmd(CH375Cmd::getStatus);
                                if (ch375GetChar() != CH375IntStatus::intSuccess) {
                                        break;
                                }

                                tusb_desc_device_t deviceDescriptor;
                                ch375SendCmd(CH375Cmd::rdUsbData0);
                                len = ch375GetChar();
                                if (len != sizeof(deviceDescriptor)) {
                                        break;
                                }
                                ch375Read(reinterpret_cast<uint8_t*>(&deviceDescriptor), len);

                                if (deviceDescriptor.idVendor != 0x054c ||
                                        (deviceDescriptor.idProduct != 0x09cc && deviceDescriptor.idProduct != 0x05c4)) {
                                        break;
                                }

                                ch375SendCmd(CH375Cmd::autoSetup);
                                sleep_us(5);

                                ch375SendCmd(CH375Cmd::getStatus);
                                if (ch375GetChar() != CH375IntStatus::intSuccess) {
                                        break;
                                }

                                break;
                        case CH375IntStatus::usbReady:
                        default:
                                break;
                };
                curIntStatus = newStatus;
        }
}

bool PS4AuthPassThroughAddon::available() {
        const UARTOptions& options = Storage::getInstance().getUARTOptions();
        return options.enabled &&
                isValidPin(options.rxPin) &&
                isValidPin(options.txPin);
}

void PS4AuthPassThroughAddon::setup() {
        uint32_t baudRate;
        initFailed = false;
        const UARTOptions& options = Storage::getInstance().getUARTOptions();

        // Other baud rates don't work so far.
        //baudRate = options.Baudrate;
        baudRate = 9600;

        srand(getMillis());

        gpio_set_function(options.txPin, GPIO_FUNC_UART);
        gpio_set_function(options.rxPin, GPIO_FUNC_UART);

        uart_init(uart0, CH375_DEFAULT_BAUDRATE);

        ch375SendCmd(CH375Cmd::resetAll);
        uart_tx_wait_blocking(uart0);
        sleep_ms(80);

        if (!ch375CheckExists()) {
                initFailed = true;
                return;
        }

        if (baudRate != CH375_DEFAULT_BAUDRATE) {
                if (!ch375SetBaudRate(baudRate) || !ch375CheckExists()) {
                        initFailed = true;
                        return;
                }
        }

        if (!ch375SetUsbMode(CH375UsbMode::hostEnabled)) {
                initFailed = true;
                return;
        }
        curIntStatus = CH375IntStatus::disconnect;
        lastConnectCheck = 0;
        challengeSent = false;
        sequenceCounter = 1;
}

void PS4AuthPassThroughAddon::process() {
        if (initFailed) {
                return;
        }

        uint32_t cTime = getMillis();

        if (!lastConnectCheck || (cTime - lastConnectCheck) > CONNECTION_CHECK_DELAY) {
                ch375TestConnect();
                lastConnectCheck = cTime;
        }

	if (PS4Data::getInstance().ps4State == PS4State::nonce_ready) {
                if (!challengeSent) {
                        if (ch375SendChallenge()) {
                                challengeSent = true;
                        }
                } else {
                        if (ch375GetSignState()) {
                                uint8_t *authBuffer = PS4Data::getInstance().ps4_auth_buffer;
                                if (ch375GetSignature(authBuffer)) {
                                        PS4Data::getInstance().ps4State = PS4State::signed_nonce_ready;
                                        challengeSent = false;
                                        sequenceCounter++;
                                }
                        }
                }
        }
}
