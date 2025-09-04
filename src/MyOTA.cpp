#include "MyOTA.h"
MyOTA ota;

MyOTA::MyOTA(int port) : server(port) {}

void MyOTA::begin(const char *ssid, const char *password) {
    // 连接 WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
    wifiEnabled = true;

    // 看esp32是否准备好了
    server.on("/OTA", HTTP_GET, [this]() {
        // 检查设备是否准备好接受OTA更新
        if (WiFi.status() == WL_CONNECTED) {
            server.send(200, "text/plain", "1"); // 准备就绪
        } else {
            server.send(500, "text/plain", "0"); // 未准备好
        }
    });

    // 注册 OTA 接口
    server.on(
        "/update", HTTP_POST,
        [this]() {
            if (Update.hasError()) {
                server.send(500, "text/plain", "OTA Failed");
            } else {
                server.send(200, "text/plain", "OTA Success. Rebooting...");
                delay(500);
                this->end();   // 关闭 WiFi 和 OTA 服务
                ESP.restart(); // 重启设备
            }
        },
        [this]() {
            HTTPUpload &upload = server.upload();
            if (upload.status == UPLOAD_FILE_START) {
                Serial.printf("Update: %s\n", upload.filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_WRITE) {
                if (Update.write(upload.buf, upload.currentSize) !=
                    upload.currentSize) {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_END) {
                if (Update.end(true)) {
                    Serial.printf("Update Success: %u bytes\n",
                                  upload.totalSize);
                } else {
                    Update.printError(Serial);
                }
            }
        });

    server.begin();
}

void MyOTA::handle() {
    if (wifiEnabled) {
        server.handleClient();
    }
}

void MyOTA::end() {
    if (wifiEnabled) {
        server.stop();
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        btStop();
        wifiEnabled = false;
        Serial.println("WiFi disabled.");
    }
}
