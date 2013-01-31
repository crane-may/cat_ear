import RPi.GPIO as GPIO
import time
GPIO.setmode(GPIO.BCM)

addr_a = 27
addr_b = 17
addr_c = 4
addr_d = 3

lat = 11
clk = 9
oe = 22
data = 10

GPIO.setup(addr_a, GPIO.OUT)
GPIO.setup(addr_b, GPIO.OUT)
GPIO.setup(addr_c, GPIO.OUT)
GPIO.setup(addr_d, GPIO.OUT)
GPIO.setup(lat, GPIO.OUT)
GPIO.setup(clk, GPIO.OUT)
GPIO.setup(oe, GPIO.OUT)
GPIO.setup(data, GPIO.OUT)

def print_led(mrx):
    for j in range(16):
        mrx_l = mrx[j]
        
        GPIO.output(lat,False)
        GPIO.output(clk,False)
        
        for i in range(16):
            GPIO.output(data,not (mrx_l & 1))        
            GPIO.output(clk,True)
            GPIO.output(clk,False)
            mrx_l = (mrx_l >> 1)

        GPIO.output(oe,True)
        GPIO.output(addr_a,not not (j & 0b0001))
        GPIO.output(addr_b,not not (j & 0b0010))
        GPIO.output(addr_c,not not (j & 0b0100))
        GPIO.output(addr_d,not not (j & 0b1000))

        GPIO.output(lat,True)
        GPIO.output(oe,False)
        GPIO.output(lat,False)

        

try:
    while True:
        print_led([
        0b1000000000000000,
        0b0100000000000000,
        0b0010000000000000,
        0b0001000000000000,
        0b0000100000000000,
        0b0000010000000000,
        0b0000001000000000,
        0b0000000100000000,
        0b0000000010000000,
        0b0000000001000000,
        0b0000000000100000,
        0b0000000000010000,
        0b0000000000001000,
        0b0000000000000100,
        0b0000000000000010,
        0b0000000000000001,
        ])
        time.sleep(0.01)
except:
    pass
GPIO.cleanup()