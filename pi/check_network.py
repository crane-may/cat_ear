import socket,time
import fcntl
import struct
 
def get_ip_address(ifname):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        return socket.inet_ntoa(fcntl.ioctl(
            s.fileno(),
            0x8915,  # SIOCGIFADDR
            struct.pack('256s', ifname[:15])
        )[20:24])
    except :
        return None

def write_led():
    f = open("/dev/shm/led_ne.buf","wb")
    ip = get_ip_address("wlan0")
    if ip != None :
        f.write(chr(0b00000000))
        f.write(chr(0b00000000))
        for s in ip.split("."):
            f.write(chr(int(s)))
        f.write(chr(0b00000000))
        f.write(chr(0b00000000))
    else:
        f.write(chr(0b00000000))
        f.write(chr(0b01000010))
        f.write(chr(0b00100100))
        f.write(chr(0b00011000))
        f.write(chr(0b00011000))
        f.write(chr(0b00100100))
        f.write(chr(0b01000010))
        f.write(chr(0b00000000))
    f.close()

def write_led2():
  f = open("/dev/shm/led_network.buf","wb")
  ip = get_ip_address("wlan0")
  if ip != None :
    f.write("0")
  else:
    f.write("1")
  f.close()
    
while True:
    write_led2()
    time.sleep(10)