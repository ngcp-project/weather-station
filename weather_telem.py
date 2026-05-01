import serial
import time

ser = serial.Serial('COM12', baudrate=115200)
time.sleep(2)

while True:
    if ser.in_waiting > 0:
        line = ser.readline().decode('utf-8').rstrip()
        print(f"ESP32 says: {line}")
