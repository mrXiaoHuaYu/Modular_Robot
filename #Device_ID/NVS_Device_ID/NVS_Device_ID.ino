// 将机器人设备ID写入NVS
// 第一次烧录时写入，之后不再修改
// 针对不同机器人，修改DEVICE_ID的值
#include <Preferences.h>

Preferences prefs;

#define DEVICE_ID "SR_01"

void setup() {
    Serial.begin(9600);
    prefs.begin("robot", false); //false：读写模式，true：只读，”robot“：命名空间
    prefs.putString("DEVICE_ID", DEVICE_ID); // 只在第一次烧录时写入
    String id = prefs.getString("DEVICE_ID", "unknown");
    Serial.println("Device ID: " + id);
    prefs.end();
}

void loop(){

}
