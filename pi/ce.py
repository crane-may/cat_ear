# sudo apt-get install alsa
# sudo apt-get install gstreamer0.10-alsa 
# sudo apt-get install gstreamer0.10-nice 
# sudo apt-get install python-gst0.10-dev
# sudo apt-get install gstreamer0.10-plugins-good
# sudo apt-get install gstreamer0.10-plugins-ugly
# sudo apt-get install python-gobject
# sudo apt-get install python-gst0.10-dev
# sudo apt-get install python-alsaaudio

import sys, os, time, thread, urllib2, json, socket, math
import control_center
import glib, gobject
import pygst, alsaaudio
pygst.require("0.10")
import gst

socket.setdefaulttimeout(10.0)

time.sleep(4)

import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setup(18, GPIO.OUT)

def _unmute():
    time.sleep(1.2)
    alsamixer.setmute(False)
    GPIO.output(18, True)

alsamixer = None
muted = True
def mute(is_mute):
    global alsamixer
    global muted
    if alsamixer == None:
        alsamixer = alsaaudio.Mixer("Speaker")
    if is_mute:
      alsamixer.setmute(is_mute)
      GPIO.output(18, not is_mute)
      muted = True
      time.sleep(0.8)
    elif muted:
      muted = False
      thread.start_new_thread(_unmute, ())

def volume(vol):
    alsamixer.setvolume( int(math.log10(vol*100.0) / 0.02) )

def write_led2(s):
  f = open("/dev/shm/led_ce.buf","wb")
  f.write(s)
  f.close()

def write_prograss(r):
    r = int(r*64.0)
    f = open("/dev/shm/led_se.buf","wb")
    for i in range(0,64,8):
        if r > i and r <= i + 8:
            f.write(chr( (1 << (r-i)) - 1 ))
        elif r <= i:
            f.write(chr(0b00000000))
        else:
            f.write(chr(0b11111111))
    
    f.close()

control_center.sync_interval()
        
class CLI_Main:
    def __init__(self):
        mute(True)
        self.play_list = []
        self.player = gst.element_factory_make("playbin", "player")

        bus = self.player.get_bus()
        bus.add_signal_watch()
        bus.connect("message", self.on_message)
    
    def play(self,uri):
        self.player.set_state(gst.STATE_NULL)
        self.player.set_property("uri", uri)
        self.player.set_state(gst.STATE_PLAYING)
        self.is_playing = True
        self.is_pause = False
        
        waiting = 0
        while self.is_playing:
            if control_center.ccchange("volume"):
                volume(control_center.ccget("volume"))
            
            if control_center.ccget("next"):
                control_center.ccset("next",False)
                mute(True)
                self.player.set_state(gst.STATE_NULL)
                self.is_playing = False
            
            if control_center.ccchange("playing"):
                if control_center.ccget("playing"):
                    self.player.set_state(gst.STATE_PLAYING)
                    self.is_pause = False
                    mute(False)
                else:
                    self.player.set_state(gst.STATE_PAUSED)
                    self.is_pause = True
                    mute(True)
            
            if control_center.ccchange("channel"):
                self.play_list = []
                mute(True)
                self.player.set_state(gst.STATE_NULL)
                self.is_playing = False
            
            if waiting % 10 == 0:
                try:
                    dur = self.cur_song['length']
                    if dur == 0:
                        dur = int(self.player.query_duration(gst.FORMAT_TIME, None)[0] / 1000000000)
                    
                    pos = int(self.player.query_position(gst.FORMAT_TIME, None)[0] / 1000000000)
                    
                    print "%s / %s %f @%f"%(self.convert_s(pos), self.convert_s(dur), 1.0 - pos * 1.0 / dur, control_center.ccget("volume"))
                    
                    if not self.is_pause:
                      write_led2("%d"%(dur - pos))
                      mute(False)
                    else:
                      write_led2("p")
                except Exception, e:
                    print "+++++++++++++++",e
                    print "00:00 / 00:00"
                    write_led2("")
                    
            time.sleep(0.1)
            waiting += 1
    
    def convert_ns(self, t):
        return self.convert_s(divmod(t, 1000000000)[0])
    
    def convert_s(self, t):
        m,s = divmod(t, 60)
        return "%02i:%02i" %(m,s)
    
    def on_message(self, bus, message):
        print message
        t = message.type
        if t == gst.MESSAGE_EOS:
            mute(True)
            self.player.set_state(gst.STATE_NULL)
            self.is_playing = False
        elif t == gst.MESSAGE_ERROR:
            self.player.set_state(gst.STATE_NULL)
            err, debug = message.parse_error()
            print "Error: %s" % err, debug
            self.is_playing = False
    
    def fetch_playlist(self):
        write_led2("")
        request = urllib2.Request(control_center.ccget("channel"))
        request.add_header('User-Agent', 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_6_8) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.101 Safari/537.11')
        request.add_header('Accept-Encoding','deflate')
        request.add_header('Accept','text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8')
        request.add_header('Connection','close')
        request.add_header('Cookie',control_center.ccget("cookie"))

        response = urllib2.urlopen(request)
        ret = response.read()
        print ret
        self.play_list = json.loads(ret)["song"]
    
    def start(self):
        control_center.ccchange("channel")
        while True:
            try:
                if len(self.play_list) == 0:
                    self.fetch_playlist()
                self.cur_song = self.play_list.pop()
                
                control_center.ccset("db_cur_song",json.dumps(self.cur_song))
                
                print "play ===",self.cur_song
                write_led2("")
                self.play(self.cur_song["url"])
            except Exception, e:
                print "="*100
                print e
                time.sleep(10)
                
        loop.quit()

write_led2("")
mainclass = CLI_Main()
thread.start_new_thread(mainclass.start, ())
gobject.threads_init()
loop = glib.MainLoop()
loop.run()
