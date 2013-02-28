import os,time,urllib2,subprocess,Queue,thread,re,signal,control_center,alsaaudio

alsamixer = None
def mute(is_mute):
    global alsamixer
    # if alsamixer == None:
        # alsamixer = alsaaudio.Mixer("PCM")
    # alsamixer.setmute(is_mute)

sig_play_over = False
def stdout_thread(p,length):
	global sig_play_over
	ret = ""
	is_mute = True
	
	while True:
		c = p.stdout.read(1)
			
		#print c,
		if sig_play_over:
			break
			
		if c == "[":
			mat = re.search("Time: (\d+):(\d+).\d+ $",ret)
			if mat != None:
				t = int(mat.group(1)) * 60 + int(mat.group(2))
				print "%d / %d" % (t,length)				
						
				if t > 0 and is_mute:
					mute(False)
					is_mute = False
				
				if length - t < 1 :
					sig_play_over = True
					break
			ret = ""
		else:
			ret += c
		
def play_thread(p, mp3_buf):
	global sig_play_over
	while True:
		if not mp3_buf.empty():
			p.stdin.write(mp3_buf.get())
		else:
			time.sleep(0.1)
		if sig_play_over:
			break

p = None
			
def playit(url,length):
	global sig_play_over
	global p
	print "play === ", url
	sig_play_over = False
	
	if length == 0:
		return
	
	mute(True)
	p=subprocess.Popen(["mpg123","-q","-v","-"], universal_newlines=True, stdin=subprocess.PIPE,stdout=subprocess.PIPE, stderr=subprocess.STDOUT)  
	mp3_buf = Queue.Queue()
		
	thread.start_new_thread(play_thread, (p, mp3_buf))
	thread.start_new_thread(stdout_thread, (p,length))
	
	request = urllib2.Request(url)
	request.add_header('User-Agent', 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_6_8) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.101 Safari/537.11')
	request.add_header('Accept-Encoding','deflate')
	request.add_header('Accept','*/*')
	request.add_header('Connection','close')
	request.add_header('Referer','http://douban.fm/')
	
	response = urllib2.urlopen(request)
	
	s = "dummy"
	while len(s) > 0:
		control()
		s = response.read(4096)
		mp3_buf.put(s)
		if sig_play_over:
			break
	
	while not sig_play_over:
		control()
		time.sleep(0.1)
	
	time.sleep(1)
	p.kill()

def stop():
	global sig_play_over
	sig_play_over = True
	time.sleep(1)

is_pause = False
def pause():
	global is_pause
	is_pause = True
	if p != None:
		p.send_signal(signal.SIGTSTP)

def resume():
	global is_pause
	if p != None and is_pause:
		p.send_signal(signal.SIGCONT)
	is_pause = False
	
def control():
	global sig_play_over
	if control_center.ccget("next"):
		control_center.ccset("next",False)
		sig_play_over = True
		
	if control_center.ccchange("playing"):
		if control_center.ccget("playing"):
			resume()
		else:
			pause()
		
if __name__ == "__main__":
	#playit('http://mr5.douban.com/201302141038/38e3d59ec8a6d000a2be02635a8c513d/view/song/small/p154461_192k.mp3')
	playit('http://mr4.douban.com/201302141533/a56e98c539270c8256e9ce4a94b1057d/view/song/small/p1741803_192k.mp3',299)