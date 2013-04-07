import subprocess,os,re,urllib2
import fcntl, socket, struct
import RPi.GPIO as GPIO
import time
GPIO.setmode(GPIO.BCM)
GPIO.setup(14, GPIO.IN, pull_up_down=GPIO.PUD_UP)

def wait4btn():
    last = True
    while True:
        cur = GPIO.input(14)
        if cur and not last:
            return
            
        time.sleep(0.1)
        last = cur

def qr_decode():
    write_led_search()
    #os.system("fswebcam -q --no-timestamp --no-banner --greyscale --png 2 /dev/shm/web-cam-shot.png")
    os.system("cd /dev/shm")
    os.system("rm -f *.png")
    os.system("mplayer tv:// -tv driver=v4l2:device=/dev/video0:width=320:height=240:outfmt=rgb24 -frames 3 -vo png:z=1 -framedrop")
    os.system("ls -S -1 | sed -e '2,$d' -e '/png/a shot.png' | xargs mv")
    handle = subprocess.Popen('/home/pi/zbar-0.10/examples/scan_image /dev/shm/shot.png', 
                            stdout=subprocess.PIPE, shell=True)

    return handle.stdout.read().strip()

def write_led2(s):
  f = open("/dev/shm/led_camera.buf","wb")
  f.write(s)
  f.close()
  
def write_wait():
    f = open("/dev/shm/led_nw.buf","wb")
    f.write(chr(0b00000000))
    f.write(chr(0b00000000))
    f.write(chr(0b00111100))
    f.write(chr(0b00100100))
    f.write(chr(0b00100100))
    f.write(chr(0b00111100))
    f.write(chr(0b00000010))
    f.write(chr(0b00000000))
    f.close()
    write_led2("0")
    
ani = True
def write_led_search():
    global ani
    f = open("/dev/shm/led_nw.buf","wb")
    f.write(chr(0b00000000))
    f.write(chr(0b00000000))
    f.write(chr(0b00111100))
    if ani:
        f.write(chr(0b00110100))
        f.write(chr(0b00101100))
    else:
        f.write(chr(0b00101100))
        f.write(chr(0b00110100))
    f.write(chr(0b00111100))
    f.write(chr(0b00000010))
    f.write(chr(0b00000000))
    f.close()
    ani = not ani
    write_led2("1")
    
def write_success():
    f = open("/dev/shm/led_nw.buf","wb")
    f.write(chr(0b00000000))
    f.write(chr(0b00000010))
    f.write(chr(0b00000100))
    f.write(chr(0b10001000))
    f.write(chr(0b01010000))
    f.write(chr(0b00100000))
    f.write(chr(0b00000000))
    f.write(chr(0b00000000))
    f.close()
    write_led2("2")
    
def wait4decode():
    ret = ""
    while ret == "":
        ret = qr_decode()
    write_success()
    
    print ret
    if   ret.startswith("wifi:"):
        try:
            wifi_cmd(ret)
        except:
            pass
            
    elif ret.startswith("pair:"):
        # try:
            # pair_cmd(ret)
        # except:
            # pass
        pair_cmd(ret)
    time.sleep(2)
    return ret

def decode_cmd(cmd_head_len,ret):
    ret = ret[(cmd_head_len+1):]
    cmds = []
    while ret != "":
        num = ret.split("$")[0]
        cmds.append(ret[len(num)+1:len(num)+int(num)+1])
        ret = ret[len(num)+int(num)+1:]
        
    print cmds
    return cmds
    
def wifi_cmd(ret):
    cmds = decode_cmd(4,ret)
    
    if len(cmds) != 2:
        return
    
    f = open("/etc/wpa_supplicant/wpa_supplicant.conf","a")
    f.write("network={\n"+
              "  ssid=\""+cmds[0]+"\"\n"+
              "  psk=\""+cmds[1]+"\"\n}\n")
    f.close()
    os.system("wpa_cli reconfigure")
    
def pair_cmd(ret):
    cmds = decode_cmd(4,ret)
    
    if len(cmds) != 1:
        return
    
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', "eth0"[:15]))
    did = "did_"+''.join(['%02x' % ord(char) for char in info[18:24]])
    
    print did
    
    request = urllib2.Request("http://radio.miaocc.com/devices/create_pair/%s"%did)
    request.add_header('User-Agent', 'Mozilla/5.0 (pi)')
    request.add_header('Accept-Encoding','deflate')
    request.add_header('Accept','application/json')
    request.add_header('Connection','close')

    response = urllib2.urlopen(request,"pair_key=%s"%cmds[0])
    print response.read()
    
while True:
    write_wait()
    wait4btn()
    wait4decode()
    