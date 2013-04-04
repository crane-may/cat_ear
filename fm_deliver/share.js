var lk = document.getElementById("fm-download-link");
var tar = document.createElement("a");
tar.innerHTML = "推送";
var sep = document.createElement("span");
sep.innerHTML = "|";

document.getElementById("fm-download").insertBefore(tar,document.getElementById("download-frame"));
document.getElementById("fm-download").insertBefore(sep,document.getElementById("download-frame"));

setInterval(function() {
  tar.setAttribute("href",lk.getAttribute("href"));
},1000);