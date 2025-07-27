import time
import serial
import argparse
import psutil
import math

def main():
    parser = argparse.ArgumentParser(prog='Nixie Clock Driver')
    parser.add_argument('serPort')

    args = parser.parse_args()

    s = serial.Serial(args.serPort, baudrate=115200, timeout=0.5)

    s.write(b"\x02x")       # clear display
    s.write(b"\x02\x2E")       # 50% brightness
    s.write(b"\x02h")       # set so go to home on '\n'

    s.write(b"\x02\x30")
    s.write(b"CPU: ")
    s.write(b"\x02\x3A")
    s.write(b"RAM: ")
    time.sleep(1)

    # b = 0
    while True:
        try:
            cpuUsage = psutil.cpu_percent(interval=None)
            ramUsage = psutil.virtual_memory().percent

            s.write(b"\x02\x34")
            s.write(f"{int(cpuUsage):>3d}%".encode())

            s.write(b"\x02\x3e")
            s.write(f"{int(ramUsage):>3d}%".encode())

            s.write(b"\x02\x44")
            cpuUsageD = (cpuUsage / 100) * 20
            # for all full percentage, sent the filled character
            for i in range(int(cpuUsageD)):
                s.write(b'\x14')
            # for the fraction, send partial
            s.write((0x10+int((cpuUsageD % 1)*5)).to_bytes(1, 'big'))
            # for the rest, fill with 0
            for i in range(math.ceil(cpuUsageD), 20):
                s.write(b'\x00')

            # s.write(b'\x02' + (0x2C+b).to_bytes(1, 'big'))
            # b += 1
            # if b > 3:
            #     b = 0

            time.sleep(1)
        except KeyboardInterrupt:
            break

    s.write(b"\x02x")       # clear display
    s.close()

if __name__ == "__main__":
    main()
