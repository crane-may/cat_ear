import subprocess,os,re,urllib2,json
#import fcntl, socket, struct
import time

# ==== Control ====
#        		type		default
# playing 		bool		True
# volume		int			30
# channel		string		{"url":"http://douban.fm/j/mine/playlist","Headers":{}}
# next			toggle		False
# like			toggle		False
# unlike		toggle		False

# ==== Status ====
# douban		string

store = {}
store_dirty = set([])
store_default = {
	"playing"	: True,
	"volume"	: 30,
	"channel"	: '{"url":"http://douban.fm/j/mine/playlist","Headers":{}}',
	"next"		: False,
	"like"		: False,
	"unlike"	: False
}

def get_did():
	# s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', "eth0"[:15]))
    # did = "did_"+''.join(['%02x' % ord(char) for char in info[18:24]])
	did = "did_b827ebaba73a"
	return did	

def ccsync():
	global store
	global store_dirty
	store_set = {}
	
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

	request = urllib2.Request("http://radio.miaocc.com/devices/get_attr/%s"%get_did())
	request.add_header('User-Agent', 'Mozilla/5.0 (pi)')
	request.add_header('Accept-Encoding','deflate')
	request.add_header('Accept','application/json')
	request.add_header('Connection','close')
	
	response = urllib2.urlopen(request)
	ret = response.read()
	print ret
	
	store = json.loads(ret)

	
def ccget(key):
	if store.has_key(key):
		return store[key]
	elif store_default.has_key(key):
		return store_default[key]
	else:
		return None

def ccset(key,v):
	store_dirty.add(key)
	store[key] = v
	ccsync()

if __name__ == "__main__" :
	print ccget("playing")
	ccset("playing",False)
	print ccget("playing")
	