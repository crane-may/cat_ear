import sys, os, time, thread, urllib2, json, socket, math
import glib, gobject
import pygst, alsaaudio
pygst.require("0.10")
import gst

print "begin load control_center"
import control_center
print "loaded"

asdf,asdf

socket.setdefaulttimeout(10.0)
alsamixer = alsaaudio.Mixer("PCM")

def convert_s(t):
  m,s = divmod(t, 60)
  return "%02i:%02i" %(m,s)
  
def write_led2(s):
  f = open("/dev/shm/led_ce.buf","wb")
  f.write(s)
  f.close()
  
def on_playing(obj):
  global alsamixer
  
  if control_center.ccchange("volume"):
    alsamixer.setvolume( int(math.log10(control_center.ccget("volume")*100.0) / 0.02) )
    
  if control_center.ccget("next"):
    control_center.ccset("next",False)
    obj.next()
  
  if control_center.ccchange("playing"):
    if control_center.ccget("playing"):
      obj.resume()
    else:
      obj.pause()
      write_led2("p")
  
  if control_center.ccchange("channel"):
    obj.play_list = []
    obj.next()
  
  try:
    dur = obj.cur_song['length']
    if dur == 0:
      dur = int(obj.player.query_duration(gst.FORMAT_TIME, None)[0] / 1000000000)
    
    pos = int(obj.player.query_position(gst.FORMAT_TIME, None)[0] / 1000000000)
  except Exception, e:
    print "+++++++++++++++",e
    dur,pos = 0,0
  
  print "%s / %s"%(convert_s(pos), convert_s(dur))
  
  if control_center.ccget("playing"):
    write_led2("%d"%(dur - pos))

def on_next_song(obj):
  if obj.play_list == [] :
    request = urllib2.Request(control_center.ccget("channel"))
    request.add_header('User-Agent', 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_6_8) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.101 Safari/537.11')
    request.add_header('Accept-Encoding','deflate')
    request.add_header('Accept','text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8')
    request.add_header('Connection','close')
    request.add_header('Cookie',control_center.ccget("cookie"))
    
    response = urllib2.urlopen(request)
    ret = response.read()
    print ret
    obj.play_list = json.loads(ret)["song"]
    
  obj.cur_song = obj.play_list.pop()
  control_center.ccset("db_cur_song",json.dumps(obj.cur_song))
  
  return obj.cur_song["url"]

#######################################################
  
class CLI_Main:
  def __init__(self):
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
    
    while self.is_playing:
      on_playing(self)
      time.sleep(0.2)
  
  def next(self):
    self.player.set_state(gst.STATE_NULL)
    self.is_playing = False
  
  def resume(self):
    self.player.set_state(gst.STATE_PLAYING)
    
  def pause(self):
    self.player.set_state(gst.STATE_PAUSED)
  
  def on_message(self, bus, message):
    print message
    t = message.type
    if t == gst.MESSAGE_EOS:
        self.next()
    elif t == gst.MESSAGE_ERROR:
      print "Error: ", message.parse_error()
      self.next()
      
  def start(self):
    while True:
      try:
        #self.play("file:///root/p1394944.mp3")
        self.play(on_next_song(self))
      except Exception, e:
        print "="*100
        print e
        time.sleep(10)
            
    loop.quit()

mainclass = CLI_Main()
thread.start_new_thread(mainclass.start, ())
gobject.threads_init()
loop = glib.MainLoop()
loop.run()