#include <SPI.h>

// ============================================================
// ESP32 STK500v1 AVR ISP PROGRAMMER
//
// Target:
//   ATmega32U4 / Arduino Leonardo
//
// ESP32:
//   GPIO23 -> MOSI
//   GPIO19 <- MISO
//   GPIO18 -> SCK
//   GPIO5  -> RESET
//
// Target power:
//   ESP32 3V3 -> ATmega32U4 VCC
//   ESP32 GND -> ATmega32U4 GND
//
// PC serial:
//   19200 baud
//
// AVR ISP:
//   10 kHz
//
// IMPORTANT:
//   - No automatic fuse programming
//   - No automatic lock-bit programming
//   - CHIP ERASE is intentionally rejected
//   - Flash programming is supported
//   - Flash reading is supported
//   - Signature reading is supported
// ============================================================


// ------------------------------------------------------------
// ESP32 pins
// ------------------------------------------------------------

#define ISP_MOSI   23
#define ISP_MISO   19
#define ISP_SCK    18
#define ISP_RESET  5

#define ISP_CLOCK_HZ 10000


// ------------------------------------------------------------
// STK500v1 protocol
// ------------------------------------------------------------

#define STK_OK                    0x10
#define STK_FAILED                0x11
#define STK_UNKNOWN               0x12
#define STK_NODEVICE              0x13

#define STK_INSYNC                0x14
#define STK_NOSYNC                0x15

#define CRC_EOP                   0x20

#define STK_GET_SYNC              0x30
#define STK_GET_SIGN_ON           0x31

#define STK_SET_PARAMETER         0x40
#define STK_GET_PARAMETER         0x41
#define STK_SET_DEVICE_PARAMETER  0x42
#define STK_SET_DEVICE_EXT        0x45

#define STK_ENTER_PROGMODE        0x50
#define STK_LEAVE_PROGMODE        0x51
#define STK_CHIP_ERASE            0x52
#define STK_CHECK_AUTOINC         0x53
#define STK_LOAD_ADDRESS          0x55
#define STK_UNIVERSAL             0x56
#define STK_UNIVERSAL_MULTI       0x57

#define STK_PROG_PAGE             0x64
#define STK_READ_PAGE             0x74
#define STK_READ_SIGN             0x75

#define STK_SW_MAJOR              0x80
#define STK_SW_MINOR              0x81

#define STK_HW_VER                0x82
#define STK_SW_MAJOR_VERSION      0x83
#define STK_SW_MINOR_VERSION      0x84


// ------------------------------------------------------------
// AVR
// ------------------------------------------------------------

SPISettings ispSettings(
  ISP_CLOCK_HZ,
  MSBFIRST,
  SPI_MODE0
);


// STK500 current address is a WORD address.
uint32_t currentAddress = 0;


// ============================================================
// Read one byte from avrdude
// ============================================================

uint8_t readByte()
{
  while (!Serial.available())
  {
    delay(1);
  }

  return Serial.read();
}


// ============================================================
// Wait for CRC_EOP
// ============================================================

bool waitForEOP()
{
  uint8_t value = readByte();

  return value == CRC_EOP;
}


// ============================================================
// STK500 success response
// ============================================================

void sendOK()
{
  Serial.write(STK_INSYNC);
  Serial.write(STK_OK);
}


// ============================================================
// STK500 failure response
// ============================================================

void sendFailed()
{
  Serial.write(STK_INSYNC);
  Serial.write(STK_FAILED);
}


// ============================================================
// AVR ISP transaction
// ============================================================

void ispTransaction(
  uint8_t b1,
  uint8_t b2,
  uint8_t b3,
  uint8_t b4,
  uint8_t &r1,
  uint8_t &r2,
  uint8_t &r3,
  uint8_t &r4
)
{
  SPI.beginTransaction(ispSettings);

  r1 = SPI.transfer(b1);
  r2 = SPI.transfer(b2);
  r3 = SPI.transfer(b3);
  r4 = SPI.transfer(b4);

  SPI.endTransaction();
}


// ============================================================
// AVR command
// ============================================================

uint8_t avrCommand(
  uint8_t b1,
  uint8_t b2,
  uint8_t b3,
  uint8_t b4
)
{
  uint8_t r1;
  uint8_t r2;
  uint8_t r3;
  uint8_t r4;

  ispTransaction(
    b1,
    b2,
    b3,
    b4,
    r1,
    r2,
    r3,
    r4
  );

  return r4;
}


// ============================================================
// Enter ISP programming mode
// ============================================================

bool enterISP()
{
  pinMode(ISP_RESET, OUTPUT);

  digitalWrite(ISP_RESET, HIGH);

  delay(100);

  digitalWrite(ISP_RESET, LOW);

  delay(100);


  uint8_t r1;
  uint8_t r2;
  uint8_t r3;
  uint8_t r4;


  ispTransaction(
    0xAC,
    0x53,
    0x00,
    0x00,
    r1,
    r2,
    r3,
    r4
  );


  return r3 == 0x53;
}


// ============================================================
// Leave ISP
// ============================================================

void leaveISP()
{
  digitalWrite(ISP_RESET, HIGH);
}


// ============================================================
// Read flash byte
// ============================================================

uint8_t readFlashByte(
  uint32_t wordAddress,
  bool highByte
)
{
  uint8_t command =
    highByte ? 0x28 : 0x20;


  uint8_t addressHigh =
    (wordAddress >> 8) & 0xFF;

  uint8_t addressLow =
    wordAddress & 0xFF;


  return avrCommand(
    command,
    addressHigh,
    addressLow,
    0x00
  );
}


// ============================================================
// Write one flash page
//
// ATmega32U4 page size:
//   128 bytes
//
// STK500 supplies the page data.
//
// AVR ISP:
//   0x40 = Load Program Memory Page, low byte
//   0x48 = Load Program Memory Page, high byte
//   0x4C = Write Program Memory Page
// ============================================================

bool writeFlashPage(
  uint32_t wordAddress,
  const uint8_t *data,
  uint16_t length
)
{
  if (length == 0)
  {
    return true;
  }


  if (length > 128)
  {
    return false;
  }


  // ----------------------------------------------------------
  // Load bytes into temporary page buffer
  // ----------------------------------------------------------

  for (
    uint16_t i = 0;
    i < length;
    i += 2
  )
  {
    uint32_t word =
      wordAddress + (i / 2);


    uint8_t addressHigh =
      (word >> 8) & 0xFF;

    uint8_t addressLow =
      word & 0xFF;


    uint8_t lowByte =
      data[i];


    uint8_t highByte =
      0xFF;


    if ((i + 1) < length)
    {
      highByte =
        data[i + 1];
    }


    // Load low byte
    avrCommand(
      0x40,
      addressHigh,
      addressLow,
      lowByte
    );


    // Load high byte
    avrCommand(
      0x48,
      addressHigh,
      addressLow,
      highByte
    );
  }


  // ----------------------------------------------------------
  // Write page
  // ----------------------------------------------------------

  uint8_t pageAddressHigh =
    (wordAddress >> 8) & 0xFF;

  uint8_t pageAddressLow =
    wordAddress & 0xFF;


  avrCommand(
    0x4C,
    pageAddressHigh,
    pageAddressLow,
    0x00
  );


  // ----------------------------------------------------------
  // Flash write time
  // ----------------------------------------------------------

  delay(10);


  return true;
}


// ============================================================
// Read flash page
// ============================================================

void readFlashPage(
  uint32_t wordAddress,
  uint16_t length
)
{
  Serial.write(STK_INSYNC);


  for (
    uint16_t i = 0;
    i < length;
    i++
  )
  {
    uint32_t byteAddress =
      (wordAddress * 2UL) + i;


    uint32_t word =
      byteAddress / 2;


    bool highByte =
      (byteAddress & 1) != 0;


    uint8_t value =
      readFlashByte(
        word,
        highByte
      );


    Serial.write(value);
  }


  Serial.write(STK_OK);
}


// ============================================================
// Setup
// ============================================================

void setup()
{
  Serial.begin(19200);


  SPI.begin(
    ISP_SCK,
    ISP_MISO,
    ISP_MOSI,
    -1
  );


  pinMode(ISP_RESET, OUTPUT);

  digitalWrite(ISP_RESET, HIGH);

  delay(100);
}


// ============================================================
// STK500 main loop
// ============================================================

void loop()
{
  if (!Serial.available())
  {
    delay(1);
    return;
  }


  uint8_t command =
    Serial.read();


  switch (command)
  {

    // --------------------------------------------------------
    // GET SYNC
    // --------------------------------------------------------

    case STK_GET_SYNC:
    {
      if (!waitForEOP())
      {
        Serial.write(STK_NOSYNC);
        break;
      }

      sendOK();

      break;
    }


    // --------------------------------------------------------
    // GET SIGN ON
    // --------------------------------------------------------

    case STK_GET_SIGN_ON:
    {
      if (!waitForEOP())
      {
        Serial.write(STK_NOSYNC);
        break;
      }


      Serial.write(STK_INSYNC);

      Serial.write(8);

      Serial.print("AVRISP_");

      Serial.write(STK_OK);

      break;
    }


    // --------------------------------------------------------
    // SET PARAMETER
    // --------------------------------------------------------

    case STK_SET_PARAMETER:
    {
      uint8_t parameter =
        readByte();

      uint8_t value =
        readByte();

      (void)parameter;
      (void)value;


      if (!waitForEOP())
      {
        Serial.write(STK_NOSYNC);
        break;
      }


      sendOK();

      break;
    }


    // --------------------------------------------------------
    // GET PARAMETER
    // --------------------------------------------------------

    case STK_GET_PARAMETER:
    {
      uint8_t parameter =
        readByte();


      if (!waitForEOP())
      {
        Serial.write(STK_NOSYNC);
        break;
      }


      Serial.write(STK_INSYNC);


      switch (parameter)
      {
        case STK_SW_MAJOR:
          Serial.write(2);
          break;

        case STK_SW_MINOR:
          Serial.write(1);
          break;

        case STK_HW_VER:
          Serial.write(3);
          break;

        case STK_SW_MAJOR_VERSION:
          Serial.write(2);
          break;

        case STK_SW_MINOR_VERSION:
          Serial.write(1);
          break;

        default:
          Serial.write(0);
          break;
      }


      Serial.write(STK_OK);

      break;
    }


    // --------------------------------------------------------
    // SET DEVICE PARAMETER
    // --------------------------------------------------------

    case STK_SET_DEVICE_PARAMETER:
    {
      for (int i = 0; i < 20; i++)
      {
        readByte();
      }


      if (!waitForEOP())
      {
        Serial.write(STK_NOSYNC);
        break;
      }


      sendOK();

      break;
    }


    // --------------------------------------------------------
    // SET DEVICE EXTENDED PARAMETERS
    // --------------------------------------------------------

    case STK_SET_DEVICE_EXT:
    {
      while (true)
      {
        uint8_t value =
          readByte();


        if (value == CRC_EOP)
        {
          break;
        }
      }


      sendOK();

      break;
    }


    // --------------------------------------------------------
    // ENTER PROGRAM MODE
    // --------------------------------------------------------

    case STK_ENTER_PROGMODE:
    {
      if (!waitForEOP())
      {
        Serial.write(STK_NOSYNC);
        break;
      }


      if (enterISP())
      {
        sendOK();
      }
      else
      {
        sendFailed();
      }


      break;
    }


    // --------------------------------------------------------
    // LEAVE PROGRAM MODE
    // --------------------------------------------------------

    case STK_LEAVE_PROGMODE:
    {
      if (!waitForEOP())
      {
        Serial.write(STK_NOSYNC);
        break;
      }


      leaveISP();

      sendOK();

      break;
    }


    // --------------------------------------------------------
    // CHIP ERASE
    //
    // Deliberately disabled.
    //
    // We will use avrdude -D.
    // --------------------------------------------------------

    case STK_CHIP_ERASE:
    {
      if (!waitForEOP())
      {
        Serial.write(STK_NOSYNC);
        break;
      }


      sendFailed();

      break;
    }


    // --------------------------------------------------------
    // CHECK AUTOINCREMENT
    // --------------------------------------------------------

    case STK_CHECK_AUTOINC:
    {
      if (!waitForEOP())
      {
        Serial.write(STK_NOSYNC);
        break;
      }


      sendOK();

      break;
    }


    // --------------------------------------------------------
    // LOAD ADDRESS
    //
    // STK500 uses WORD addresses.
    // --------------------------------------------------------

    case STK_LOAD_ADDRESS:
    {
      uint8_t low =
        readByte();

      uint8_t high =
        readByte();


      currentAddress =
        ((uint32_t)high << 8) |
        low;


      if (!waitForEOP())
      {
        Serial.write(STK_NOSYNC);
        break;
      }


      sendOK();

      break;
    }


    // --------------------------------------------------------
    // UNIVERSAL
    // --------------------------------------------------------

    case STK_UNIVERSAL:
    {
      uint8_t b1 =
        readByte();

      uint8_t b2 =
        readByte();

      uint8_t b3 =
        readByte();

      uint8_t b4 =
        readByte();


      if (!waitForEOP())
      {
        Serial.write(STK_NOSYNC);
        break;
      }


      uint8_t result =
        avrCommand(
          b1,
          b2,
          b3,
          b4
        );


      Serial.write(STK_INSYNC);

      Serial.write(result);

      Serial.write(STK_OK);

      break;
    }


    // --------------------------------------------------------
    // UNIVERSAL MULTI
    // --------------------------------------------------------

    case STK_UNIVERSAL_MULTI:
    {
      uint8_t count =
        readByte();


      for (
        uint8_t i = 0;
        i < count;
        i++
      )
      {
        readByte();
      }


      if (!waitForEOP())
      {
        Serial.write(STK_NOSYNC);
        break;
      }


      Serial.write(STK_INSYNC);
      Serial.write(0x00);
      Serial.write(STK_OK);

      break;
    }


    // --------------------------------------------------------
    // PROGRAM PAGE
    // --------------------------------------------------------

    case STK_PROG_PAGE:
    {
      uint8_t lengthHigh =
        readByte();

      uint8_t lengthLow =
        readByte();


      uint16_t length =
        ((uint16_t)lengthHigh << 8) |
        lengthLow;


      uint8_t memType =
        readByte();


      if (
        length == 0 ||
        length > 128
      )
      {
        for (
          uint16_t i = 0;
          i < length;
          i++
        )
        {
          readByte();
        }


        waitForEOP();

        sendFailed();

        break;
      }


      uint8_t pageData[128];


      for (
        uint16_t i = 0;
        i < length;
        i++
      )
      {
        pageData[i] =
          readByte();
      }


      if (!waitForEOP())
      {
        Serial.write(STK_NOSYNC);
        break;
      }


      // Flash only
      if (
        memType != 'F' &&
        memType != 'F'
      )
      {
        sendFailed();
        break;
      }


      bool result =
        writeFlashPage(
          currentAddress,
          pageData,
          length
        );


      if (result)
      {
        sendOK();
      }
      else
      {
        sendFailed();
      }


      currentAddress +=
        (length + 1) / 2;


      break;
    }


    // --------------------------------------------------------
    // READ PAGE
    // --------------------------------------------------------

    case STK_READ_PAGE:
    {
      uint8_t lengthHigh =
        readByte();

      uint8_t lengthLow =
        readByte();


      uint16_t length =
        ((uint16_t)lengthHigh << 8) |
        lengthLow;


      uint8_t memType =
        readByte();


      if (!waitForEOP())
      {
        Serial.write(STK_NOSYNC);
        break;
      }


      if (
        memType != 'F' ||
        length > 128
      )
      {
        sendFailed();
        break;
      }


      readFlashPage(
        currentAddress,
        length
      );


      currentAddress +=
        (length + 1) / 2;


      break;
    }


    // --------------------------------------------------------
    // READ SIGNATURE
    // --------------------------------------------------------

    case STK_READ_SIGN:
    {
      if (!waitForEOP())
      {
        Serial.write(STK_NOSYNC);
        break;
      }


      uint8_t sig0 =
        avrCommand(
          0x30,
          0x00,
          0x00,
          0x00
        );


      uint8_t sig1 =
        avrCommand(
          0x30,
          0x00,
          0x01,
          0x00
        );


      uint8_t sig2 =
        avrCommand(
          0x30,
          0x00,
          0x02,
          0x00
        );


      Serial.write(STK_INSYNC);

      Serial.write(sig0);
      Serial.write(sig1);
      Serial.write(sig2);

      Serial.write(STK_OK);

      break;
    }


    // --------------------------------------------------------
    // UNKNOWN
    // --------------------------------------------------------

    default:
    {
      Serial.write(STK_NOSYNC);

      break;
    }
  }
}
