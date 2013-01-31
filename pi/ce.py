# sudo apt-get install alsa
# sudo apt-get install gstreamer0.10-alsa 
# sudo apt-get install gstreamer0.10-nice 
# sudo apt-get install python-gst0.10-dev
# sudo apt-get install gstreamer0.10-good
# sudo apt-get install gstreamer0.10-plugins-good
# sudo apt-get install gstreamer0.10-plugins-ugly
# sudo apt-get install python-gobject
# sudo apt-get install python-gst0.10-dev

import sys, os, time, thread, urllib2, json, socket
import glib, gobject
import pygst
pygst.require("0.10")
import gst
import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setup(15, GPIO.IN, pull_up_down=GPIO.PUD_UP)

socket.setdefaulttimeout(10.0)

def write_wait():
    f = open("/dev/shm/led_se.buf","wb")
    f.write(chr(0b00000000))
    f.write(chr(0b00111100))
    f.write(chr(0b00100100))
    f.write(chr(0b00100100))
    f.write(chr(0b00100100))
    f.write(chr(0b01000100))
    f.write(chr(0b00001000))
    f.write(chr(0b00000000))
    f.close()
    
def write_fetch():
    f = open("/dev/shm/led_se.buf","wb")
    f.write(chr(0b00000000))
    f.write(chr(0b00010000))
    f.write(chr(0b00010000))
    f.write(chr(0b00010000))
    f.write(chr(0b01010100))
    f.write(chr(0b00111000))
    f.write(chr(0b00010000))
    f.write(chr(0b00000000))
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

def controller(ce_ins):
    last = True
    while True:
        cur = GPIO.input(15)
        if cur and not last:
            ce_ins.controller_toggle_pause = True
            
        time.sleep(0.1)
        last = cur
    
class CLI_Main:
    def __init__(self):
        thread.start_new_thread(controller, (self,))
        self.controller_toggle_pause = False
        self.controller_toggle_pause_status = False
    
        self.play_list = []
        self.player = gst.Pipeline("player")
        source = gst.element_factory_make("souphttpsrc", "http-source")
        decoder = gst.element_factory_make("mad", "mp3-decoder")
        conv = gst.element_factory_make("audioconvert", "converter")
        sink = gst.element_factory_make("alsasink", "alsa-output")
        
        self.player.add(source, decoder, conv, sink)
        gst.element_link_many(source, decoder, conv, sink)

        bus = self.player.get_bus()
        bus.add_signal_watch()
        bus.connect("message", self.on_message)
    
    def play(self,uri):
        self.player.set_state(gst.STATE_NULL)
        self.player.get_by_name("http-source").set_property("location", uri)
        self.player.set_state(gst.STATE_PLAYING)
        self.is_playing = True
        
        waiting = 0
        while self.is_playing:
            time.sleep(0.1)
            waiting += 1
            
            if self.controller_toggle_pause:
                self.controller_toggle_pause = False
                self.controller_toggle_pause_status = not self.controller_toggle_pause_status
                
                if self.controller_toggle_pause_status:
                    self.player.set_state(gst.STATE_PAUSED)
                else:
                    self.player.set_state(gst.STATE_PLAYING)
            
            if waiting % 10 == 0:
                try:
                    dur = self.cur_song['length']
                    if dur == 0:
                        dur = int(self.player.query_duration(gst.FORMAT_TIME, None)[0] / 1000000000)
                    
                    pos = int(self.player.query_position(gst.FORMAT_TIME, None)[0] / 1000000000)
                    
                    print "%s / %s %f"%(self.convert_s(pos), self.convert_s(dur), 1.0 - pos * 1.0 / dur)
                    write_prograss(1.0 - pos * 1.0 / dur)
                except Exception, e:
                    print "00:00 / 00:00"
                    write_prograss(0)
    
    def convert_ns(self, t):
		return self.convert_s(divmod(t, 1000000000)[0])
    
    def convert_s(self, t):
		m,s = divmod(t, 60)
		return "%02i:%02i" %(m,s)
    
    def on_message(self, bus, message):
        print message
        t = message.type
        if t == gst.MESSAGE_EOS:
            self.player.set_state(gst.STATE_NULL)
            self.is_playing = False
        elif t == gst.MESSAGE_ERROR:
            self.player.set_state(gst.STATE_NULL)
            err, debug = message.parse_error()
            print "Error: %s" % err, debug
            self.is_playing = False
    
    def fetch_playlist(self):
        write_fetch()
        request = urllib2.Request("http://douban.fm/j/mine/playlist")
        request.add_header('User-Agent', 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_6_8) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.101 Safari/537.11')
        request.add_header('Accept-Encoding','deflate')
        request.add_header('Accept','text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8')
        request.add_header('Connection','close')

        response = urllib2.urlopen(request)
        self.play_list = json.loads(response.read())["song"]
    
    def start(self):
        while True:
            try:
                if len(self.play_list) == 0:
                    self.fetch_playlist()
                self.cur_song = self.play_list.pop()
                print "play ===",self.cur_song
                write_wait()
                self.play(self.cur_song["url"])
            except Exception, e:
                print e
                time.sleep(1)
                
        loop.quit()

write_wait()
mainclass = CLI_Main()
thread.start_new_thread(mainclass.start, ())
gobject.threads_init()
loop = glib.MainLoop()
loop.run()