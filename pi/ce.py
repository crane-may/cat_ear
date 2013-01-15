# sudo apt-get install alsa
# sudo apt-get install gstreamer0.10-alsa 
# sudo apt-get install gstreamer0.10-nice 
# sudo apt-get install python-gst0.10-dev
# sudo apt-get install gstreamer0.10-good
# sudo apt-get install gstreamer0.10-plugins-good
# sudo apt-get install gstreamer0.10-plugins-ugly
# sudo apt-get install python-gobject
# sudo apt-get install python-gst0.10-dev

import sys, os, time, thread, urllib2, json
import glib, gobject
import pygst
pygst.require("0.10")
import gst

class CLI_Main:
    def __init__(self):
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
        while self.is_playing:
            time.sleep(1)
    
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
        request = urllib2.Request("http://douban.fm/j/mine/playlist")
        request.add_header('User-Agent', 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_6_8) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.101 Safari/537.11')
        request.add_header('Accept-Encoding','deflate')
        request.add_header('Accept','text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8')
        request.add_header('Connection','keep-alive')

        response = urllib2.urlopen(request)
        self.play_list = json.loads(response.read())["song"]
    
    def start(self):
        while True:
            if len(self.play_list) == 0:
                self.fetch_playlist()
            sng = self.play_list.pop()
            print "play ===",sng
            self.play(sng["url"])
        # self.play("file:///home/pi/p1877389.mp3")
        #self.play("http://mr4.douban.com/201301160214/c910d6687e042d6a7edffb2d3ff0c27c/view/song/small/p1028605_192k.mp3")
        time.sleep(1)
        loop.quit()

mainclass = CLI_Main()
thread.start_new_thread(mainclass.start, ())
gobject.threads_init()
loop = glib.MainLoop()
loop.run()