import json,urllib2,ce2,control_center

play_list = []

def fetch_playlist():
	global play_list
	#write_fetch()
	chl = json.loads(control_center.ccget("channel"))
	request = urllib2.Request(chl['url'])
	request.add_header('User-Agent', 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_6_8) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.101 Safari/537.11')
	request.add_header('Accept-Encoding','deflate')
	request.add_header('Accept','text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8')
	request.add_header('Connection','close')
	
	for (k,v) in chl['headers'].items():
		request.add_header(k,v)

	response = urllib2.urlopen(request)
	play_list = json.loads(response.read())["song"]
	

while True:
	if len(play_list) == 0:
		fetch_playlist()
	
	cur_song = play_list.pop()
	ce2.playit(cur_song["url"])