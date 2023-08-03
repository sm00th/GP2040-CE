/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2023 Artem Savkov
 */

#ifndef AUTH_PASSTHROUGH_H_
#define AUTH_PASSTHROUGH_H_

#include "gpaddon.h"
#include "tusb.h"
#include "BoardConfig.h"
#include "tusb_types.h"

#ifndef HAS_PS4_AUTH_PASSTHROUGH
#define HAS_PS4_AUTH_PASSTHROUGH 0
#endif /* HAS_PS4_AUTH_PASSTHROUGH */

#define PS4AuthPassthroughName "PS4AuthPassThrough"

#define CH375_DEFAULT_BAUDRATE 9600
#define CH375_USB_DESCRIPTOR_DEVICE 0x01
#define DS4_CHUNK_SIZE 56
#define CONNECTION_CHECK_DELAY 500

struct signState {
        uint8_t reportId;
        uint8_t sequenceCounter;
        uint8_t state;
        uint8_t padding[9];
        uint32_t crc32;
};

struct signChallenge {
        uint8_t reportId;
        uint8_t sequenceCounter;
        uint8_t reportCounter;
        uint8_t zero;
        uint8_t data[DS4_CHUNK_SIZE];
        uint32_t crc32;
};

struct signResponse {
        uint8_t reportId;
        uint8_t sequenceCounter;
        uint8_t reportCounter;
        uint8_t zero;
        uint8_t data[DS4_CHUNK_SIZE];
        uint32_t crc32;
};

typedef enum {
        epControl = 0x00,
        epOut     = 0x03,
        epIn      = 0x84,
} DS4Endpoint;

typedef enum {
        deviceDisabled        = 0x00,
        deviceEnabledExternal,
        deviceEnabledInternal,
        hostDisabled          = 0x04,
        hostEnabled,
        hostEnabledSOF,
        hostEnabledReset,
} CH375UsbMode;

typedef enum {
        setBaudRate = 0x02,
        resetAll    = 0x05,
        checkExists = 0x06,
        setUsbMode  = 0x15,
        testConnect = 0x16,
        setEndp6    = 0x1c,
        setEndp7    = 0x1d,
        getStatus   = 0x22,
        rdUsbData0  = 0x27,
        rdUsbData   = 0x28,
        wrUsbData   = 0x2b,
        getDescr    = 0x46,
        autoSetup   = 0x4d,
        issueToken  = 0x4f
} CH375Cmd;

typedef enum {
        success = 0x51,
        abrt    = 0x5f
} CH375Retcode;

typedef enum {
        out   = 0x01,
        in    = 0x09,
        setup = 0x0d
} CH375TokenPid;

typedef enum {
        data0 = 0x80,
        data1 = 0xc0
} CH375WMode;

typedef enum {
        intSuccess = 0x14,
        connect,
        disconnect,
        bufOver,
        usbReady
} CH375IntStatus;

class PS4AuthPassThroughAddon : public GPAddon {
public:
	virtual bool available();
	virtual void setup();
	virtual void preprocess() {}
	virtual void process();
	virtual std::string name() { return PS4AuthPassthroughName; }
private:
        void ch375SendCmd(CH375Cmd cmd);
        void ch375SendData(uint8_t data);
        uint8_t ch375GetChar(void);
        void ch375Read(uint8_t *buf, uint8_t len);
        bool ch375SetBaudRate(uint32_t baudRate);
        bool ch375CheckExists(void);
        void ch375TestConnect(void);
        bool ch375SetUsbMode(CH375UsbMode mode);
        bool ch375SendRequest(tusb_control_request_t *request, uint8_t *buf);
        bool ch375SendDataRequest(tusb_control_request_t *request, uint8_t *data, uint16_t dataSize);
        bool ch375IssueToken(uint8_t epAddr, CH375TokenPid pid);
        bool ch375SendChallenge(void);
        bool ch375GetSignState(void);
        bool ch375GetDateTime(void);
        bool ch375GetStatus(void);
        bool ch375GetSignatureChunk(uint8_t *authBuffer);
        bool ch375GetSignature(uint8_t *authBuffer);
        bool ch375SetChallengeChunk(struct signChallenge *chunk);
        //bool ch376SetTest(void);
        uart_parity_t getParity(uint8_t data, bool isCmd);
        uint8_t countBits(uint8_t data);

        bool initFailed;
        bool challengeSent;
        uint32_t lastConnectCheck;
        uint32_t sequenceCounter;
        CH375IntStatus curIntStatus;
};

#endif /* AUTH_PASSTHROUGH_H_ */
