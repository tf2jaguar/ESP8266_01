#define BLINKER_PRINT Serial
#define BLINKER_WIFI
#define BLINKER_DUEROS_SENSOR

#include <Blinker.h>
#include <DHT.h>

// 定义引脚
int RELAY_IO = 0;
int DHT_IO = 2;

char auth[] = "32550a78bc02";
char ssid[] = "xxxx"; // 这里要换成你家的wifi名称
char pswd[] = "xxxx"; // 这里要换成你家的wifi密码

// 数据组件
BlinkerButton Button1("light_switch"); // 定义开关按钮
BlinkerNumber HUMI("humi");            // 定义湿度数据键名
BlinkerNumber TEMP("temp");            // 定义温度数据键名
BlinkerNumber BETTV("bettv");          // 定义温度数据键名

// 生成DHT对象，参数是引脚和DHT的类型
DHT dht(DHT_IO, DHT11);
ADC_MODE(ADC_VCC);

// 定义浮点型全局变量 储存传感器读取的温湿度数据
float humi_read = 0, temp_read = 0, bettv_read = 0;
int32_t light_status = 0;

// 反馈开关状态
void button1_status(int32_t status)
{
  if (0 == status)
  {
    Button1.text("关闭");
    Button1.color("#94b8b8");
    Button1.print("off");
  }
  else if (1 == status)
  {
    Button1.text("打开");
    Button1.color("#ffcc00");
    Button1.print("on");
  }
}

// light 按钮回调函数
void button1_callback(const String &state)
{
  BLINKER_LOG("light status:", state);
  if (state == BLINKER_CMD_ON)
  {
    digitalWrite(RELAY_IO, LOW);
    light_status = 1;
    button1_status(light_status);
  }
  else if (state == BLINKER_CMD_OFF)
  {
    digitalWrite(RELAY_IO, HIGH);
    light_status = 0;
    button1_status(light_status);
  }
}

// app 发送指令会触发
void dataRead(const String &data)
{
  BLINKER_LOG("Blinker readString: ", data);
}

// app定时向设备发送心跳包
void heartbeat()
{
  HUMI.print(humi_read);
  TEMP.print(temp_read);
  BETTV.print(bettv_read);
  button1_status(light_status);
}

// 云存储温湿度数据函数
void dataStorage()
{
  Blinker.dataStorage("temp", temp_read);
  Blinker.dataStorage("humi", humi_read);
  Blinker.dataStorage("bettv", bettv_read);
}

// 小度电源类操作的回调函数:
void duerPowerState(const String &state)
{
  BLINKER_LOG("need set power state:  ", state);
  if (state == BLINKER_CMD_ON)
  {
    digitalWrite(RELAY_IO, LOW);
    light_status = 1;
    BlinkerDuerOS.powerState("on");
    BlinkerDuerOS.print();
  }
  else if (state == BLINKER_CMD_OFF)
  {
    digitalWrite(RELAY_IO, HIGH);
    light_status = 0;
    BlinkerDuerOS.powerState("off");
    BlinkerDuerOS.print();
  }
}

// 小度同学语音命令反馈
void duerQuery(int32_t queryCode)
{
  BLINKER_LOG("Query codes: ", queryCode);
  switch (queryCode)
  {
  case BLINKER_CMD_QUERY_HUMI_NUMBER:
    BLINKER_LOG("DuerOS Query HUMI");
    int humi_read_int;
    humi_read_int = (int)humi_read;
    BlinkerDuerOS.humi(humi_read_int);
    BlinkerDuerOS.print();
    break;
  case BLINKER_CMD_QUERY_TEMP_NUMBER:
    BLINKER_LOG("DuerOS Query TEMP");
    BlinkerDuerOS.temp(temp_read);
    BlinkerDuerOS.print();
    break;
  case BLINKER_CMD_QUERY_TIME_NUMBER:
    BLINKER_LOG("DuerOS Query Time");
    BlinkerDuerOS.time(millis());
    BlinkerDuerOS.print();
  default:
    BlinkerDuerOS.temp(20);
    BlinkerDuerOS.humi(20);
    BlinkerDuerOS.pm25(20);
    BlinkerDuerOS.pm10(20);
    BlinkerDuerOS.co2(20);
    BlinkerDuerOS.aqi(20);
    BlinkerDuerOS.time(millis());
    BlinkerDuerOS.print();
    break;
  }
}

void setup()
{
  // 初始化串口
  Serial.begin(115200);
  BLINKER_DEBUG.stream(Serial);
  BLINKER_DEBUG.debugAll();

  // 初始化有LED的IO, 接通时开关打开(不然会有闪烁)
  pinMode(RELAY_IO, OUTPUT);
  digitalWrite(RELAY_IO, LOW);
  light_status = 1;

  // 初始化blinker
  Blinker.begin(auth, ssid, pswd);
  Blinker.attachData(dataRead);

  Button1.attach(button1_callback);
  Blinker.attachDataStorage(dataStorage);
  Blinker.attachHeartbeat(heartbeat);

  BlinkerDuerOS.attachQuery(duerQuery);
  BlinkerDuerOS.attachPowerState(duerPowerState);

  // 初始化DHT传感器
  dht.begin();
}

void loop()
{
  Blinker.run();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t))
  {
    BLINKER_LOG("Failed to read from DHT sensor!");
  }
  else
  {
    BLINKER_LOG("Humidity: ", h, " %");
    BLINKER_LOG("Temperature: ", t, " °C");

    humi_read = h; // 将读取到的湿度赋值给全局变量humi_read
    temp_read = t; // 将读取到的温度赋值给全局变量temp_read
  }
  bettv_read = ((float)ESP.getVcc() / 1024.0f);
  BLINKER_LOG("BETTV: ", bettv_read, " v");

  button1_status(light_status);

  Blinker.delay(10000); // 延时函数
}
