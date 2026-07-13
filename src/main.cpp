#include <Arduino.h>
#include <stdio.h>

// ESP32          ---      MP3
// GPIO17(TX2)     ->      RX
// GPIO16(RX2)     <-      TX
// GPIO27          ->      BUSY

// MP3   ---   3.5mm
// DAC_L  ->   Tip
// DAC_R  ->   Ring
// GND    ->   Sleeve

const int MP3_RX_PIN = 16;
const int MP3_TX_PIN = 17;
const int MP3_BUSY_PIN = 27;

constexpr uint32_t DEBUG_BAUD = 115200;
constexpr uint32_t MP3_BAUD = 9600;

enum class Mp3Error
{
  Ok,
  InvalidVolume,
  InvalidTrackNumber,
};

const char *mp3ErrorToString(Mp3Error error)
{
  switch (error)
  {
  case Mp3Error::Ok:
    return "ok";
  case Mp3Error::InvalidVolume:
    return "invalid volume";
  case Mp3Error::InvalidTrackNumber:
    return "invalid track number";
  default:
    return "unknown error";
  }
}

HardwareSerial mp3Serial(2);

void sendMp3Command(uint8_t command, uint16_t parameter, bool feedback = false)
{
  uint8_t frame[10];

  frame[0] = 0x7E;
  frame[1] = 0xFF;
  frame[2] = 0x06;
  frame[3] = command;
  frame[4] = feedback ? 0x01 : 0x00;
  frame[5] = (parameter >> 8) & 0xFF;
  frame[6] = parameter & 0xFF;

  uint16_t checksum = 0;
  checksum += frame[1];
  checksum += frame[2];
  checksum += frame[3];
  checksum += frame[4];
  checksum += frame[5];
  checksum += frame[6];

  checksum = 0 - checksum;

  frame[7] = (checksum >> 8) & 0xFF;
  frame[8] = checksum & 0xFF;
  frame[9] = 0xEF;

  mp3Serial.write(frame, sizeof(frame));
}

// 选择 TF 卡
void selectTfCard()
{
  // 0x09 = 指定播放设备
  // 0x0002 = TF / SD 卡
  sendMp3Command(0x09, 0x0002);
}

// 设置音量，范围 0-30
Mp3Error setVolume(int volume)
{
  if (volume < 0 || volume > 30)
  {
    return Mp3Error::InvalidVolume;
  }

  // 0x06 = 指定音量
  sendMp3Command(0x06, volume);
  return Mp3Error::Ok;
}

struct TrackNameResult
{
  Mp3Error error;
  char value[14];
};

TrackNameResult formatTrackName(int trackNumber)
{
  TrackNameResult result;
  result.error = Mp3Error::Ok;
  result.value[0] = '\0';

  if (trackNumber < 1 || trackNumber > 9999)
  {
    result.error = Mp3Error::InvalidTrackNumber;
    return result;
  }

  snprintf(result.value, sizeof(result.value), "/MP3/%04d.mp3", trackNumber);
  return result;
}

TrackNameResult playMp3Track(int trackNumber)
{
  TrackNameResult trackName = formatTrackName(trackNumber);
  if (trackName.error != Mp3Error::Ok)
  {
    return trackName;
  }

  // 0x12 = 指定 MP3 文件夹中的曲目
  // trackNumber = 1 表示 /MP3/0001.mp3
  // trackNumber = 2 表示 /MP3/0002.mp3
  sendMp3Command(0x12, static_cast<uint16_t>(trackNumber));
  return trackName;
}

void stopPlayback()
{
  // 0x16 = 停止播放
  sendMp3Command(0x16, 0);
}

bool isPlaying()
{
  return digitalRead(MP3_BUSY_PIN) == LOW;
}

void printHelp()
{
  Serial.println();
  Serial.println("MP3-TF-16P test");
  Serial.println("TF card file example:");
  Serial.println("  /MP3/0001.mp3");
  Serial.println("  /MP3/0002.mp3");
  Serial.println();
  Serial.println("Serial commands:");
  Serial.println("  1        play /MP3/0001.mp3");
  Serial.println("  2        play /MP3/0002.mp3");
  Serial.println("  15       play /MP3/0015.mp3");
  Serial.println("  v 20     set volume to 20");
  Serial.println("  s        stop");
  Serial.println("  b        show BUSY pin state");
  Serial.println();
}

void handleSerialCommand(String line)
{
  line.trim();

  if (line.length() == 0)
  {
    return;
  }

  if (line == "s")
  {
    stopPlayback();
    Serial.println("Stop command sent.");
    return;
  }

  if (line == "b")
  {
    Serial.print("BUSY pin state: ");
    if (isPlaying())
    {
      Serial.println("LOW, playing");
    }
    else
    {
      Serial.println("HIGH, idle");
    }
    return;
  }

  if (line.startsWith("v "))
  {
    int volume = line.substring(2).toInt();
    Mp3Error error = setVolume(volume);
    if (error != Mp3Error::Ok)
    {
      Serial.print("Error: ");
      Serial.println(mp3ErrorToString(error));
      return;
    }

    Serial.print("Volume set to ");
    Serial.println(volume);
    return;
  }

  int trackNumber = line.toInt();

  TrackNameResult trackName = playMp3Track(trackNumber);
  if (trackName.error != Mp3Error::Ok)
  {
    Serial.print("Error: ");
    Serial.println(mp3ErrorToString(trackName.error));
  }
  else
  {
    Serial.print("Play command sent: ");
    Serial.println(trackName.value);
  }
}

void printMp3Responses()
{
  while (mp3Serial.available())
  {
    uint8_t value = mp3Serial.read();

    Serial.print("MP3 response: 0x");
    if (value < 0x10)
    {
      Serial.print("0");
    }
    Serial.println(value, HEX);
  }
}

void setup()
{
  pinMode(MP3_BUSY_PIN, INPUT_PULLUP);

  Serial.begin(DEBUG_BAUD);
  mp3Serial.begin(MP3_BAUD, SERIAL_8N1, MP3_RX_PIN, MP3_TX_PIN);

  delay(500);

  printHelp();

  Serial.println("Waiting for MP3 module startup...");
  delay(1500);

  Serial.println("Selecting TF card...");
  selectTfCard();

  // 文档建议: 指定播放设备后, 等待约 200ms, 在发播放指令
  delay(200);

  Serial.println("Setting volume to 30...");
  Mp3Error error = setVolume(30);

  delay(100);

  Serial.print("playing initial file: ");
  TrackNameResult trackName = playMp3Track(1);
  Serial.println(trackName.value);
}

void loop()
{
  if (Serial.available())
  {
    String line = Serial.readStringUntil('\n');
    handleSerialCommand(line);
  }

  printMp3Responses();
}
