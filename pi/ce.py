# sudo apt-get install alsa
# sudo apt-get install gstreamer0.10-alsa 
# sudo apt-get install gstreamer0.10-nice 
# sudo apt-get install python-gst0.10-dev
# sudo apt-get install gstreamer0.10-good
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

alsamixer = None
def mute(is_mute):
    global alsamixer
    if alsamixer == None:
        alsamixer = alsaaudio.Mixer("PCM")
    alsamixer.setmute(is_mute)

def volume(vol):
    alsamixer.setvolume( int(math.log10(vol*100.0) / 0.02) )
    
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

def remote_sync(ce_ins):
    while True:
        control_center.ccsync()
        time.sleep(5)
        
class CLI_Main:
    def __init__(self):
        thread.start_new_thread(remote_sync, (self,))
    
        mute(True)
        self.play_list = []
        self.player = gst.Pipeline("player")
        source = gst.element_factory_make("souphttpsrc", "http-source")
        #volume = gst.element_factory_make("volume", "volume")
        decoder = gst.element_factory_make("mad", "mp3-decoder")
        conv = gst.element_factory_make("audioconvert", "converter")
        sink = gst.element_factory_make("alsasink", "alsa-output")
        
        #self.player.add(source, volume, decoder, conv, sink)
        #gst.element_link_many(source, decoder, conv, volume, sink)
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
            if control_center.ccchange("volume"):
                #self.player.get_by_name("volume").set_property('volume', control_center.ccget("volume"))
                volume(control_center.ccget("volume"))
            
            if control_center.ccget("next"):
                control_center.ccset("next",False)
                #self.player.get_by_name("volume").set_property('mute', True)
                mute(True)
                self.player.set_state(gst.STATE_NULL)
                self.is_playing = False
            
            if control_center.ccchange("playing"):
                if control_center.ccget("playing"):
                    self.player.set_state(gst.STATE_PLAYING)
                else:
                    self.player.set_state(gst.STATE_PAUSED)
            
            if waiting % 10 == 0:
                try:
                    dur = self.cur_song['length']
                    if dur == 0:
                        dur = int(self.player.query_duration(gst.FORMAT_TIME, None)[0] / 1000000000)
                    
                    pos = int(self.player.query_position(gst.FORMAT_TIME, None)[0] / 1000000000)
                    
                    print "%s / %s %f @%f"%(self.convert_s(pos), self.convert_s(dur), 1.0 - pos * 1.0 / dur, control_center.ccget("volume"))
                    write_prograss(1.0 - pos * 1.0 / dur)
                    
                    mute(False)
                except Exception, e:
                    print "00:00 / 00:00"
                    write_prograss(0)
                    
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
        write_fetch()
        chl = json.loads(control_center.ccget("channel"))
        request = urllib2.Request(chl['url'])
        request.add_header('User-Agent', 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_6_8) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.101 Safari/537.11')
        request.add_header('Accept-Encoding','deflate')
        request.add_header('Accept','text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8')
        request.add_header('Connection','close')
        
        for (k,v) in chl['headers'].items():
            request.add_header(k,v)

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
                print "="*100
                print e
                time.sleep(10)
                
        loop.quit()

write_wait()
mainclass = CLI_Main()
thread.start_new_thread(mainclass.start, ())
gobject.threads_init()
loop = glib.MainLoop()
loop.run()