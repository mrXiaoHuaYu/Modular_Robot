import serial
import time

def send_hex_data(port, baudrate, hex_str, delay=0):
    """
    通过串口发送十六进制数据

    :param port: 串口名，比如 COM3 或 /dev/ttyUSB0
    :param baudrate: 波特率，比如 9600
    :param hex_str: 十六进制字符串，如 "01 04 12 DE AD BE EF"
    :param delay: 发送后延迟（秒）
    """
    # 解析十六进制字符串，去除空格，转换为字节数组
    hex_str = hex_str.replace(' ', '')
    if len(hex_str) % 2 != 0:
        raise ValueError("十六进制字符串长度必须为偶数")

    data = bytes.fromhex(hex_str)

    # 打开串口
    with serial.Serial(port, baudrate, timeout=1) as ser:
        print(f"打开串口 {port}，波特率 {baudrate}")
        ser.write(data)
        print(f"发送数据: {data.hex(' ')}")
        if delay > 0:
            time.sleep(delay)

if __name__ == '__main__':
    # 修改成你的串口和波特率
    port = 'COM24'           # Windows 示例，Linux/Mac 下改成 /dev/ttyUSB0
    baudrate = 9600

    # 你要发送的十六进制数据字符串
    # hex_data = "01 04 12 DE AD BE EF"
    hex_data = "01 15 12 DE AD BE EF"

    send_hex_data(port, baudrate, hex_data)
