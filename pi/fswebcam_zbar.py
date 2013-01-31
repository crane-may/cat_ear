import subprocess,os

def decode():
    os.system("fswebcam -q --no-timestamp --no-banner --greyscale --png 2 /dev/shm/web-cam-shot.png")
    handle = subprocess.Popen('/home/pi/zbar-0.10/examples/scan_image /dev/shm/web-cam-shot.png', 
                            stdout=subprocess.PIPE, shell=True)

    return handle.stdout.read().strip()

ret = ""
while ret == "" :
    ret = decode()
    
print ret