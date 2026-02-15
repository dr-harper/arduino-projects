#ifndef HTML_PAGES_H
#define HTML_PAGES_H

#include <Arduino.h>

// ─── Shared CSS ─────────────────────────────────────────────────────────────
static const char SHARED_CSS[] PROGMEM = R"rawliteral(
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;
  background:#1a1a2e;color:#e0e0e0;min-height:100vh}
.container{max-width:520px;margin:0 auto;padding:16px}
h1{font-size:1.4em;margin-bottom:12px;color:#f0f0f0}
h2{font-size:1.1em;margin-bottom:8px;color:#c0c0ff}
.card{background:#16213e;border-radius:12px;padding:16px;margin-bottom:14px;
  box-shadow:0 2px 8px rgba(0,0,0,0.3)}
label{display:block;font-size:0.85em;color:#a0a0c0;margin-bottom:4px;margin-top:10px}
input[type=text],input[type=password],input[type=number],select{
  width:100%;padding:10px;border:1px solid #2a2a4a;border-radius:8px;
  background:#0f1a30;color:#e0e0e0;font-size:0.95em}
input[type=range]{width:100%;accent-color:#7b68ee}
button,.btn{display:inline-block;padding:10px 20px;border:none;border-radius:8px;
  font-size:0.95em;cursor:pointer;text-align:center;text-decoration:none;
  transition:background 0.2s}
.btn-primary{background:#7b68ee;color:#fff}
.btn-primary:hover{background:#6a5acd}
.btn-danger{background:#e74c3c;color:#fff}
.btn-danger:hover{background:#c0392b}
.btn-secondary{background:#34495e;color:#fff}
.btn-secondary:hover{background:#2c3e50}
.btn-sm{padding:6px 12px;font-size:0.82em}
.btn-block{display:block;width:100%;margin-top:10px}
.row{display:flex;gap:10px;align-items:center}
.row>*{flex:1}
.status-dot{display:inline-block;width:10px;height:10px;border-radius:50%;
  margin-right:6px}
.dot-green{background:#2ecc71}
.dot-amber{background:#f39c12}
.dot-red{background:#e74c3c}
.toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);
  background:#2ecc71;color:#fff;padding:12px 24px;border-radius:8px;
  font-size:0.9em;opacity:0;transition:opacity 0.3s;z-index:999;
  pointer-events:none}
.toast.show{opacity:1}
.toast.error{background:#e74c3c}
a{color:#7b68ee;text-decoration:none}
a:hover{text-decoration:underline}
.footer{text-align:center;font-size:0.75em;color:#666;margin-top:20px;padding:10px}
.gallery{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));gap:8px;margin-top:10px}
.gallery-item{position:relative;border-radius:8px;overflow:hidden;cursor:pointer;
  aspect-ratio:4/3;background:#0f1a30}
.gallery-item img{width:100%;height:100%;object-fit:cover}
.gallery-item .meta{position:absolute;bottom:0;left:0;right:0;padding:4px 6px;
  background:linear-gradient(transparent,rgba(0,0,0,0.8));font-size:0.7em;color:#ccc}
.stream-wrap{position:relative;border-radius:8px;overflow:hidden;
  background:#000;aspect-ratio:4/3;margin-bottom:10px}
.stream-wrap img{width:100%;height:100%;object-fit:contain}
.tag-pill{display:inline-block;padding:4px 10px;border-radius:12px;
  background:#2a2a4a;color:#c0c0ff;font-size:0.8em;margin:2px}
.confidence-bar{height:8px;background:#0f1a30;border-radius:4px;overflow:hidden;margin:2px 0}
.confidence-fill{height:100%;background:#7b68ee;border-radius:4px}
)rawliteral";

// ─── Login Page ─────────────────────────────────────────────────────────────
static const char LOGIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>AI Camera</title>
<style>%CSS%
.login-box{max-width:320px;margin:80px auto;text-align:center}
.login-box h1{font-size:2em;margin-bottom:4px}
.login-box .subtitle{color:#888;margin-bottom:24px;font-size:0.9em}
.login-box .icon{font-size:3em;margin-bottom:12px}
.error-msg{color:#e74c3c;font-size:0.85em;margin-top:8px;display:none}
</style></head><body>
<div class="login-box">
  <div class="icon">&#128247;</div>
  <h1>AI Camera</h1>
  <p class="subtitle">Enter password to continue</p>
  <form method="POST" action="/login">
    <input type="password" name="password" placeholder="Password"
           autofocus autocomplete="current-password">
    <button type="submit" class="btn btn-primary btn-block">Log in</button>
  </form>
  <p class="error-msg" id="err">%ERROR%</p>
</div>
</body></html>
)rawliteral";

// ─── Dashboard Page ─────────────────────────────────────────────────────────
static const char DASHBOARD_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>AI Camera</title>
<style>%CSS%</style></head><body>
<div class="container">
  <h1>&#128247; AI Camera</h1>

  <!-- Live Stream -->
  <div class="card">
    <h2>Live View</h2>
    <div class="stream-wrap">
      <img id="streamImg" alt="Camera stream">
    </div>
    <div class="row">
      <button class="btn btn-primary" onclick="startStream()">Start Stream</button>
      <button class="btn btn-secondary" onclick="stopStream()">Stop Stream</button>
      <button class="btn btn-primary" onclick="captureImage()">&#128248; Capture</button>
    </div>
  </div>

  <!-- Status -->
  <div class="card">
    <h2>Status</h2>
    <p><span class="status-dot" id="timeDot"></span>
       Time: <strong id="curTime">--:--:--</strong></p>
    <p style="margin-top:4px">
       <span class="status-dot" id="camDot"></span>
       Camera: <strong id="camStatus">—</strong></p>
    <p style="margin-top:4px">
       <span class="status-dot" id="mlDot"></span>
       ML: <strong id="mlStatus">—</strong></p>
    <p style="margin-top:4px">
       Images: <strong id="imgCount">0</strong> / <strong id="imgCap">0</strong></p>
  </div>

  <!-- Gallery -->
  <div class="card">
    <h2>Recent Captures</h2>
    <div class="gallery" id="gallery">
      <p style="color:#666;font-size:0.85em">No captures yet</p>
    </div>
    <button class="btn btn-danger btn-sm" style="margin-top:10px"
      onclick="if(confirm('Clear all images?'))clearAll()">Clear All</button>
  </div>

  <div class="footer">
    <a href="/settings">Settings</a> &middot;
    <a href="/update">Firmware Update</a> &middot;
    <a href="/logout">Log out</a><br>
    <span id="ipAddr"></span> &middot;
    <span id="fwVersion"></span> &middot;
    <span id="uptime"></span> &middot;
    <span id="freeHeap"></span> &middot;
    PSRAM: <span id="freePsram"></span>
  </div>
</div>

<div class="toast" id="toast"></div>

<script>
var streaming=false;
function escapeHtml(s){
  return s.replace(/&/g,'&amp;').replace(/</g,'&lt;')
          .replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}
function toast(msg,err){
  var t=document.getElementById('toast');
  t.textContent=msg;
  t.className=err?'toast error show':'toast show';
  setTimeout(function(){t.className='toast'},2500);
}
function startStream(){
  if(streaming)return;
  var img=document.getElementById('streamImg');
  img.src='/stream?'+Date.now();
  streaming=true;
  toast('Stream started');
}
function stopStream(){
  var img=document.getElementById('streamImg');
  img.src='';
  streaming=false;
  toast('Stream stopped');
}
function captureImage(){
  toast('Capturing...');
  fetch('/api/capture',{method:'POST'}).then(r=>{
    if(!r.ok)throw new Error('Capture failed');
    return r.json();
  }).then(d=>{
    toast('Captured! Image #'+d.id);
    fetchGallery();
  }).catch(e=>toast(e.message,true));
}
function clearAll(){
  fetch('/api/clear-images',{method:'POST'}).then(()=>{
    toast('Images cleared');
    fetchGallery();
  });
}
function fetchGallery(){
  fetch('/api/images').then(r=>r.json()).then(d=>{
    var g=document.getElementById('gallery');
    document.getElementById('imgCount').textContent=d.count;
    document.getElementById('imgCap').textContent=d.capacity;
    if(!d.images||d.images.length===0){
      g.innerHTML='<p style="color:#666;font-size:0.85em">No captures yet</p>';
      return;
    }
    // Sort by time descending (most recent first)
    var sorted=d.images.slice().sort(function(a,b){
      return b.time.localeCompare(a.time);
    });
    var html='';
    sorted.forEach(function(img){
      var timeShort=img.time.substring(11,19)||img.time;
      var labelStr='';
      if(img.topLabel&&img.topConf>0){
        var lbl=img.topLabel.length>14?img.topLabel.substring(0,14)+'\u2026':img.topLabel;
        labelStr=' \u00b7 '+escapeHtml(lbl);
      }
      html+='<div class="gallery-item" onclick="viewImage('+img.id+')">'
        +'<img src="/api/image/'+img.id+'" loading="lazy">'
        +'<div class="meta">'+timeShort+labelStr
        +(img.numTags>0?' &#127991;':'')
        +'</div></div>';
    });
    g.innerHTML=html;
  }).catch(function(){});
}
function viewImage(id){
  window.location.href='/image?id='+id;
}
function fetchStatus(){
  fetch('/api/status').then(r=>r.json()).then(d=>{
    document.getElementById('curTime').textContent=d.time||'--:--:--';
    var td=document.getElementById('timeDot');
    td.className='status-dot '+(d.synced?'dot-green':'dot-amber');
    var cd=document.getElementById('camDot');
    var cs=document.getElementById('camStatus');
    cd.className='status-dot '+(d.cameraReady?'dot-green':'dot-red');
    cs.textContent=d.cameraReady?(d.streamActive?'Streaming':'Ready'):'Offline';
    var md=document.getElementById('mlDot');
    var ms=document.getElementById('mlStatus');
    md.className='status-dot '+(d.mlReady?'dot-green':'dot-red');
    ms.textContent=d.mlReady?'Ready'+(d.autoClassify?' (auto)':''):'Not loaded';
    document.getElementById('ipAddr').textContent=d.ip||'';
    if(d.version)document.getElementById('fwVersion').textContent='v'+d.version;
    if(d.uptime)document.getElementById('uptime').textContent='Up: '+d.uptime;
    if(d.freeHeap)document.getElementById('freeHeap').textContent=Math.round(d.freeHeap/1024)+'KB';
    if(d.freePsram)document.getElementById('freePsram').textContent=Math.round(d.freePsram/1024)+'KB';
  }).catch(function(){});
}
fetchStatus();
fetchGallery();
setInterval(fetchStatus,5000);
setInterval(fetchGallery,10000);
</script>
</body></html>
)rawliteral";

// ─── Image Detail Page ──────────────────────────────────────────────────────
static const char IMAGE_DETAIL_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Image — AI Camera</title>
<style>%CSS%
.detail-img{width:100%;border-radius:8px;margin-bottom:10px}
.result-row{display:flex;align-items:center;gap:8px;margin:4px 0;font-size:0.85em}
.result-label{min-width:120px;color:#e0e0e0}
.result-pct{min-width:40px;text-align:right;color:#a0a0c0}
.tag-input{display:flex;gap:6px;margin-top:6px}
.tag-input input{flex:1}
</style></head><body>
<div class="container">
  <div style="display:flex;justify-content:space-between;align-items:center">
    <a href="/">&larr; Back</a>
    <div class="row" style="gap:6px;flex:unset">
      <button class="btn btn-secondary btn-sm" id="prevBtn" onclick="navImage(-1)">&larr; Prev</button>
      <button class="btn btn-secondary btn-sm" id="nextBtn" onclick="navImage(1)">Next &rarr;</button>
    </div>
  </div>
  <h1 id="pageTitle">Image</h1>

  <div class="card">
    <img id="detailImg" class="detail-img" alt="Captured image">
    <p style="font-size:0.85em;color:#a0a0c0">
      <span id="imgTime">—</span> &middot;
      <span id="imgSize">—</span> &middot;
      <span id="imgDims">—</span>
    </p>
    <div class="row" style="margin-top:8px">
      <a class="btn btn-secondary btn-sm" id="downloadLink" href="#" download>Download</a>
      <button class="btn btn-danger btn-sm"
        onclick="if(confirm('Delete this image?'))deleteThisImage()">Delete</button>
    </div>
  </div>

  <!-- Classification Results -->
  <div class="card" id="classCard" style="display:none">
    <h2>&#128300; Classification</h2>
    <div id="classResults"></div>
    <p id="classTime" style="font-size:0.75em;color:#666;margin-top:6px"></p>
    <button class="btn btn-primary btn-sm" style="margin-top:8px"
      onclick="classifyImage()">Re-classify</button>
  </div>
  <div class="card" id="classifyPrompt" style="display:none">
    <h2>&#128300; Classification</h2>
    <p style="color:#a0a0c0;font-size:0.85em">No classification results yet.</p>
    <button class="btn btn-primary" onclick="classifyImage()">Classify Image</button>
  </div>

  <!-- Tags -->
  <div class="card">
    <h2>Tags</h2>
    <div id="tagList"></div>
    <div class="tag-input">
      <input type="text" id="newTag" placeholder="Add a tag..." maxlength="31"
             onkeydown="if(event.key==='Enter')addTag()">
      <button class="btn btn-primary btn-sm" onclick="addTag()">Add</button>
    </div>
    <button class="btn btn-primary btn-block" onclick="saveTags()">Save Tags</button>
  </div>
</div>

<div class="toast" id="toast"></div>

<script>
var imgId=-1;
var tags=[];
var imageIds=[];
function escapeHtml(s){
  return s.replace(/&/g,'&amp;').replace(/</g,'&lt;')
          .replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}
function toast(msg,err){
  var t=document.getElementById('toast');
  t.textContent=msg;
  t.className=err?'toast error show':'toast show';
  setTimeout(function(){t.className='toast'},2500);
}
function getParam(name){
  var url=new URL(window.location.href);
  return url.searchParams.get(name);
}
function renderTags(){
  var html='';
  tags.forEach(function(t,i){
    html+='<span class="tag-pill">'+escapeHtml(t)
      +' <span style="cursor:pointer;margin-left:4px" onclick="removeTag('+i+')">&times;</span>'
      +'</span>';
  });
  if(tags.length===0)html='<span style="color:#666;font-size:0.85em">No tags</span>';
  document.getElementById('tagList').innerHTML=html;
}
function addTag(){
  var inp=document.getElementById('newTag');
  var tag=inp.value.trim();
  if(tag&&tags.length<5){
    tags.push(tag);
    inp.value='';
    renderTags();
  }
}
function removeTag(i){
  tags.splice(i,1);
  renderTags();
}
function saveTags(){
  fetch('/api/image/'+imgId+'/tags',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify({tags:tags})
  }).then(r=>{
    if(r.ok)toast('Tags saved!');
    else toast('Save failed',true);
  }).catch(()=>toast('Save failed',true));
}
function loadImage(){
  imgId=parseInt(getParam('id'));
  if(isNaN(imgId)){window.location.href='/';return;}
  document.getElementById('pageTitle').textContent='Image #'+imgId;
  document.getElementById('detailImg').src='/api/image/'+imgId;
  document.getElementById('downloadLink').href='/api/image/'+imgId+'/download';

  fetch('/api/images').then(r=>r.json()).then(d=>{
    imageIds=d.images.slice().sort(function(a,b){
      return a.time.localeCompare(b.time);
    }).map(function(x){return x.id;});
    updateNavButtons();

    var img=d.images.find(function(x){return x.id===imgId});
    if(!img)return;
    document.getElementById('imgTime').textContent=img.time;
    document.getElementById('imgSize').textContent=Math.round(img.size/1024)+'KB';
    document.getElementById('imgDims').textContent=img.width+'x'+img.height;
  });

  // Load existing tags from server
  fetch('/api/image/'+imgId+'/tags').then(r=>r.json()).then(d=>{
    if(d.tags&&d.tags.length>0){
      tags=d.tags;
      renderTags();
    }
  }).catch(function(){});

  // Load classification results
  loadResults();
}
function updateNavButtons(){
  var idx=imageIds.indexOf(imgId);
  document.getElementById('prevBtn').disabled=(idx<=0);
  document.getElementById('nextBtn').disabled=(idx<0||idx>=imageIds.length-1);
}
function navImage(dir){
  var idx=imageIds.indexOf(imgId);
  if(idx<0)return;
  var newIdx=idx+dir;
  if(newIdx<0||newIdx>=imageIds.length)return;
  window.location.href='/image?id='+imageIds[newIdx];
}
function deleteThisImage(){
  fetch('/api/image/'+imgId,{method:'DELETE'}).then(r=>{
    if(!r.ok)throw new Error('Delete failed');
    toast('Image deleted');
    setTimeout(function(){window.location.href='/';},500);
  }).catch(e=>toast(e.message,true));
}
function renderResults(results,inferenceMs){
  if(!results||results.length===0){
    document.getElementById('classCard').style.display='none';
    document.getElementById('classifyPrompt').style.display='block';
    return;
  }
  document.getElementById('classCard').style.display='block';
  document.getElementById('classifyPrompt').style.display='none';
  var html='';
  results.forEach(function(r){
    var pct=Math.round(r.confidence*100);
    var canAdd=tags.indexOf(r.label)<0&&tags.length<5;
    html+='<div class="result-row">'
      +'<span class="result-label">'+escapeHtml(r.label)+'</span>'
      +'<div class="confidence-bar" style="flex:1"><div class="confidence-fill" style="width:'+pct+'%"></div></div>'
      +'<span class="result-pct">'+pct+'%</span>'
      +(canAdd?'<span style="cursor:pointer;font-size:0.9em;margin-left:4px" title="Add as tag" class="add-tag-btn" data-label="'+escapeHtml(r.label)+'">+</span>':'')
      +'</div>';
  });
  document.getElementById('classResults').innerHTML=html;
  // Bind click handlers for "add as tag" buttons via event delegation
  document.getElementById('classResults').onclick=function(e){
    var btn=e.target.closest('.add-tag-btn');
    if(btn)addTagFromLabel(btn.getAttribute('data-label'));
  };
  if(inferenceMs)document.getElementById('classTime').textContent='Inference: '+inferenceMs+'ms';
}
function addTagFromLabel(label){
  if(tags.indexOf(label)>=0||tags.length>=5)return;
  tags.push(label);
  renderTags();
  loadResults();
  toast('Tag added: '+label);
}
function loadResults(){
  fetch('/api/image/'+imgId+'/results').then(r=>r.json()).then(d=>{
    renderResults(d.results);
    if(!d.mlReady){
      document.getElementById('classifyPrompt').style.display='none';
    }
  }).catch(function(){});
}
function classifyImage(){
  toast('Classifying...');
  fetch('/api/image/'+imgId+'/classify',{method:'POST'}).then(r=>{
    if(r.status===503){toast('ML not ready',true);return Promise.reject();}
    if(!r.ok)throw new Error('Classification failed');
    return r.json();
  }).then(d=>{
    if(d)renderResults(d.results,d.inferenceMs);
    toast('Classified!');
  }).catch(e=>{if(e&&e.message)toast(e.message,true);});
}
loadImage();
renderTags();
</script>
</body></html>
)rawliteral";

// ─── Settings Page ──────────────────────────────────────────────────────────
static const char SETTINGS_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Settings — AI Camera</title>
<style>%CSS%</style></head><body>
<div class="container">
  <p><a href="/">&larr; Back to dashboard</a></p>
  <h1>&#9881; Settings</h1>

  <div class="card">
    <h2>Camera</h2>
    <label>JPEG Quality (5=best, 63=worst)</label>
    <input type="number" id="jpegQuality" min="5" max="63" value="12">
    <label style="display:flex;align-items:center;gap:8px;margin-top:12px;cursor:pointer">
      <input type="checkbox" id="autoClassify" style="width:auto;accent-color:#7b68ee">
      Auto-classify on capture
    </label>
    <p id="mlStatus" style="font-size:0.78em;color:#666;margin-top:4px"></p>
  </div>

  <div class="card">
    <h2>General</h2>
    <label>Timezone</label>
    <select id="tzSelect">
      <option value="GMT0BST,M3.5.0/1,M10.5.0">UK — GMT / BST</option>
      <option value="CET-1CEST,M3.5.0,M10.5.0/3">Central Europe — CET / CEST</option>
      <option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Eastern Europe — EET / EEST</option>
      <option value="EST5EDT,M3.2.0,M11.1.0">US Eastern — EST / EDT</option>
      <option value="CST6CDT,M3.2.0,M11.1.0">US Central — CST / CDT</option>
      <option value="MST7MDT,M3.2.0,M11.1.0">US Mountain — MST / MDT</option>
      <option value="PST8PDT,M3.2.0,M11.1.0">US Pacific — PST / PDT</option>
      <option value="AEST-10AEDT,M10.1.0,M4.1.0/3">Australia Eastern — AEST / AEDT</option>
      <option value="NZST-12NZDT,M9.5.0,M4.1.0/3">New Zealand — NZST / NZDT</option>
      <option value="IST-5:30">India — IST (no DST)</option>
      <option value="JST-9">Japan — JST (no DST)</option>
      <option value="CST-8">China — CST (no DST)</option>
    </select>
    <label>Or enter custom POSIX TZ string</label>
    <input type="text" id="tzCustom" placeholder="e.g. GMT0BST,M3.5.0/1,M10.5.0">
    <label>Change password</label>
    <input type="password" id="newPass" placeholder="New password">
    <button class="btn btn-primary btn-block" onclick="saveSettings()">
      Save Settings</button>
  </div>

  <div class="card">
    <h2>System</h2>
    <p style="font-size:0.85em;color:#a0a0c0;margin-bottom:8px">
      Free heap: <strong id="freeHeap">—</strong> &middot;
      Free PSRAM: <strong id="freePsram">—</strong></p>
    <p style="font-size:0.85em;color:#a0a0c0;margin-bottom:8px">
      Images stored: <strong id="imgCount">—</strong></p>
    <button class="btn btn-danger btn-sm"
      onclick="if(confirm('Clear all captured images?'))fetch('/api/clear-images',{method:'POST'}).then(()=>toast('Images cleared'))">
      Clear All Images</button>
  </div>

  <div class="footer">
    <a href="/update">Firmware Update</a> &middot;
    <a href="/logout">Log out</a> &middot;
    <span id="ipAddr"></span><br>
    <span id="fwVersion"></span> &middot;
    <span id="uptime"></span>
  </div>
</div>

<div class="toast" id="toast"></div>

<script>
function toast(msg,err){
  var t=document.getElementById('toast');
  t.textContent=msg;
  t.className=err?'toast error show':'toast show';
  setTimeout(function(){t.className='toast'},2500);
}
function fetchStatus(){
  fetch('/api/status').then(r=>r.json()).then(d=>{
    if(d.timezone){
      var sel=document.getElementById('tzSelect');
      var found=false;
      for(var i=0;i<sel.options.length;i++){
        if(sel.options[i].value===d.timezone){sel.selectedIndex=i;found=true;break;}
      }
      if(!found)document.getElementById('tzCustom').value=d.timezone;
    }
    document.getElementById('jpegQuality').value=d.jpegQuality||12;
    document.getElementById('autoClassify').checked=d.autoClassify||false;
    var mlEl=document.getElementById('mlStatus');
    if(d.mlReady){mlEl.textContent='ML engine ready (last: '+d.lastInferenceMs+'ms)';mlEl.style.color='#2ecc71';}
    else{mlEl.textContent='ML engine not loaded';mlEl.style.color='#e74c3c';}
    document.getElementById('ipAddr').textContent=d.ip||'';
    if(d.version)document.getElementById('fwVersion').textContent='v'+d.version;
    if(d.uptime)document.getElementById('uptime').textContent='Up: '+d.uptime;
    if(d.freeHeap)document.getElementById('freeHeap').textContent=Math.round(d.freeHeap/1024)+'KB';
    if(d.freePsram)document.getElementById('freePsram').textContent=Math.round(d.freePsram/1024)+'KB';
    document.getElementById('imgCount').textContent=d.imageCount+'/'+d.imageCapacity;
  }).catch(function(){});
}
function saveSettings(){
  var custom=document.getElementById('tzCustom').value;
  var tz=custom||document.getElementById('tzSelect').value;
  var pass=document.getElementById('newPass').value;
  var quality=document.getElementById('jpegQuality').value;
  var autoC=document.getElementById('autoClassify').checked?'true':'false';
  var body='timezone='+encodeURIComponent(tz)+'&jpegQuality='+quality+'&autoClassify='+autoC;
  if(pass)body+='&password='+encodeURIComponent(pass);
  fetch('/api/settings',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:body}).then(function(){toast('Settings saved');fetchStatus()})
    .catch(function(){toast('Save failed',true)});
}
fetchStatus();
setInterval(fetchStatus,5000);
</script>
</body></html>
)rawliteral";

// ─── Firmware Update Page ───────────────────────────────────────────────────
static const char UPDATE_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Update — AI Camera</title>
<style>%CSS%
.progress-bar{width:100%;height:24px;background:#0f1a30;border-radius:8px;overflow:hidden;margin-top:10px;display:none}
.progress-fill{height:100%;background:#7b68ee;width:0%;transition:width 0.3s;border-radius:8px}
</style></head><body>
<div class="container">
  <p><a href="/">&larr; Back to dashboard</a></p>
  <h1>&#128268; Firmware Update</h1>
  <div class="card">
    <h2>Upload Firmware</h2>
    <p style="font-size:0.85em;color:#a0a0c0;margin-bottom:12px">
      Current version: <strong id="curVer">—</strong></p>
    <p style="font-size:0.85em;color:#a0a0c0;margin-bottom:12px">
      Select a compiled <code>.bin</code> file from Arduino IDE
      (Sketch &rarr; Export Compiled Binary).</p>
    <input type="file" id="firmwareFile" accept=".bin"
           style="margin-bottom:10px;font-size:0.9em">
    <button class="btn btn-primary btn-block" id="uploadBtn" onclick="uploadFirmware()">
      Upload &amp; Flash</button>
    <div class="progress-bar" id="progressBar">
      <div class="progress-fill" id="progressFill"></div>
    </div>
    <p id="uploadStatus" style="margin-top:10px;font-size:0.9em"></p>
  </div>
</div>
<div class="toast" id="toast"></div>
<script>
function toast(msg,err){
  var t=document.getElementById('toast');
  t.textContent=msg;
  t.className=err?'toast error show':'toast show';
  setTimeout(function(){t.className='toast'},3000);
}
function uploadFirmware(){
  var file=document.getElementById('firmwareFile').files[0];
  if(!file){toast('No file selected',true);return;}
  if(!file.name.endsWith('.bin')){toast('Please select a .bin file',true);return;}
  var bar=document.getElementById('progressBar');
  var fill=document.getElementById('progressFill');
  var status=document.getElementById('uploadStatus');
  var btn=document.getElementById('uploadBtn');
  bar.style.display='block';fill.style.width='0%';
  btn.disabled=true;btn.textContent='Uploading...';status.textContent='';
  var form=new FormData();
  form.append('firmware',file);
  var xhr=new XMLHttpRequest();
  xhr.open('POST','/update');
  xhr.upload.onprogress=function(e){
    if(e.lengthComputable){
      var pct=Math.round(e.loaded/e.total*100);
      fill.style.width=pct+'%';
      status.textContent='Uploading: '+pct+'%';
    }
  };
  xhr.onload=function(){
    if(xhr.status===200){
      fill.style.width='100%';
      status.textContent='Update complete — rebooting...';
      status.style.color='#2ecc71';
      setTimeout(function(){location.href='/';},5000);
    }else{
      status.textContent='Update failed: '+xhr.responseText;
      status.style.color='#e74c3c';
      btn.disabled=false;btn.textContent='Upload & Flash';
    }
  };
  xhr.onerror=function(){
    status.textContent='Upload failed — connection lost';
    status.style.color='#e74c3c';
    btn.disabled=false;btn.textContent='Upload & Flash';
  };
  xhr.send(form);
}
fetch('/api/status').then(r=>r.json()).then(d=>{
  if(d.version)document.getElementById('curVer').textContent='v'+d.version;
}).catch(function(){});
</script>
</body></html>
)rawliteral";

#endif // HTML_PAGES_H
