import subprocess,os,re,urllib2,json
import fcntl, socket, struct
import time, thread

# ==== Control ====
#               type        default
# playing       bool        True
# volume        int         0.005
# channel       string      {"url":"http://douban.fm/j/mine/playlist","headers":{}}
# next          bool        False
# like          bool        False
# unlike        bool        False

# ==== Status ====
# douban        string

store = {}
store_last = {}
store_dirty = set([])
store_default = {
    "playing"   : True,
    "volume"    : 0.005,
    "channel"   : 'http://douban.fm/j/mine/playlist',
    "next"      : False,
    "like"      : False,
    "unlike"    : False
}

did = None
def get_did():
    global did
    if did == None:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', "eth0"[:15]))
        did = "did_"+''.join(['%02x' % ord(char) for char in info[18:24]])
    #did = "did_b827ebaba73a"
    return did  

def ccsync():
    global store
    global store_dirty
    store_set = {"last_update":"pi"}
    
    if len(store_dirty) > 0:
        for dk in store_dirty:
            store_set[dk] = store[dk]
        store_dirty = set([])
        
        request = urllib2.Request("http://radio.miaocc.com/devices/update_attr/%s"%get_did())
        request.add_header('User-Agent', 'Mozilla/5.0 (pi)')
        request.add_header('Accept-Encoding','deflate')
        request.add_header('Accept','application/json')
        request.add_header('Connection','close')
        
        response = urllib2.urlopen(request,json.dumps(store_set))
        print response.read()
    request = urllib2.Request("http://radio.miaocc.com/devices/get_attr/%s?unless=db_cur_song"%get_did())
    request.add_header('User-Agent', 'Mozilla/5.0 (pi)')
    request.add_header('Accept-Encoding','deflate')
    request.add_header('Accept','application/json')
    request.add_header('Connection','close')
    
    response = urllib2.urlopen(request)
    ret = response.read()
    print "###",ret
    
    store.update(json.loads(ret))

    
def ccget(key):
    if store.has_key(key):
        if store[key] == "true":
          return True
        elif store[key] == "false":
          return False
        elif re.match(r"^\d+([.]\d+)?$",store[key]):
          return float(store[key])
        else:
          return store[key]
    elif store_default.has_key(key):
        return store_default[key]
    else:
        return None

def ccchange(key):
    global store_last
    if store_last.has_key(key) and ccget(key) != store_last[key] or not store_last.has_key(key):
        store_last[key] = ccget(key)
        return True
    else:
        store_last[key] = ccget(key)
        return False
    
def ccset(key,v):
    store_dirty.add(key)
    store[key] = v
    ccsync()

	
import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setup(15, GPIO.IN, pull_up_down=GPIO.PUD_UP)

def controller():
    last = True
    while True:
        cur = GPIO.input(15)
        if cur and not last:
            ccset("playing",not ccget("playing"))
            
        time.sleep(0.1)
        last = cur
        
thread.start_new_thread(controller, ())

def remote_sync():
    while True:
        time.sleep(3)
        ccsync()
        
while True:
  try:
    ccsync()
  except Exception,e:
    print "====",e
    time.sleep(3)
    continue
  
  break
  
thread.start_new_thread(remote_sync, ())

if __name__ == "__main__" :
  while True:
    print ccget("channel")
    time.sleep(1)
