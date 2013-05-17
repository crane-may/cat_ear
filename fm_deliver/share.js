var lk = document.getElementById("fm-download-link");
var tar = document.createElement("a");
tar.innerHTML = "推送";
tar.setAttribute("href","javascript:;")
var sep = document.createElement("span");
sep.innerHTML = "|";

document.getElementById("fm-download").insertBefore(tar,document.getElementById("download-frame"));
document.getElementById("fm-download").insertBefore(sep,document.getElementById("download-frame"));

tar.addEventListener("click",function() {
  var url = "http://douban.fm"+lk.getAttribute("href");
  
  try{
    var xml = new XMLHttpRequest();
    xml.open("POST","http://radio.miaocc.com/devices/update_attr/did_b827ebaba73a",true);
    xml.send('{"deliver":"'+url+'"}');
    
    alert("推送成功\n"+url);
  }catch(e){
    console.log(e);
  }
});