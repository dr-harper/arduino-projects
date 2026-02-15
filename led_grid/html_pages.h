#ifndef HTML_PAGES_H
#define HTML_PAGES_H

#include <Arduino.h>

// ─── Shared CSS ────────────────────────────────────────────────────────────

static const char SHARED_CSS[] PROGMEM = R"rawliteral(
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;
  background:#0f0f1a;color:#e0e0e8;min-height:100vh;padding:16px}
.container{max-width:480px;margin:0 auto}
h1{font-size:1.4em;margin-bottom:16px;display:flex;align-items:center;gap:8px}
h2{font-size:1.1em;margin-bottom:12px;color:#a0a0c0}
.card{background:#1a1a2e;border-radius:12px;padding:16px;margin-bottom:14px;
  border:1px solid #2a2a4a}
label{display:block;font-size:0.85em;color:#8888aa;margin-bottom:4px}
input[type=text],input[type=password],input[type=number]{width:100%;padding:10px;
  background:#0f1a30;border:1px solid #3a3a5a;border-radius:8px;color:#e0e0e8;
  font-size:1em;margin-bottom:8px}
input[type=range]{width:100%;accent-color:#7b68ee;margin:6px 0 2px}
.range-row{display:flex;align-items:center;gap:12px}
.range-row input[type=range]{flex:1}
.range-val{min-width:36px;text-align:right;font-weight:600;color:#7b68ee;font-size:0.95em}
button,.btn{display:inline-block;padding:10px 18px;border:none;border-radius:8px;
  font-size:0.95em;cursor:pointer;font-weight:600;text-decoration:none;
  transition:transform 0.1s,opacity 0.15s}
button:active,.btn:active{transform:scale(0.96)}
.btn-primary{background:#7b68ee;color:#fff}
.btn-primary:hover{background:#6a58dd}
.btn-secondary{background:#2a2a4a;color:#c0c0d8}
.btn-secondary:hover{background:#3a3a5a}
.btn-danger{background:#c0392b;color:#fff}
.btn-danger:hover{background:#a93226}
.btn-full{width:100%;text-align:center;display:block}
.btn-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin:8px 0}
.btn-effect{padding:12px;border-radius:10px;background:#16213e;color:#a0a0c0;
  border:2px solid transparent;font-size:0.9em;cursor:pointer;transition:all 0.15s}
.btn-effect.active{border-color:#7b68ee;color:#fff;background:#1e2a4a}
.btn-effect:hover{background:#1e2a4a}
.btn-game{background:#1a1040;font-size:1em;padding:14px}
.btn-game.active{border-color:#9b7dff;background:#2a1860}
.mt-12{margin-top:12px}
.status-bar{display:flex;justify-content:space-between;font-size:0.8em;color:#6666888;
  padding:8px 0}
.dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:4px}
.dot-green{background:#2ecc71}
.dot-red{background:#e74c3c}
.dot-amber{background:#f39c12}
.toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);
  background:#2ecc71;color:#fff;padding:10px 20px;border-radius:8px;
  font-size:0.9em;opacity:0;transition:opacity 0.3s;pointer-events:none;z-index:999}
.toast.error{background:#c0392b}
.toast.show{opacity:1}
.colour-swatches{display:flex;gap:6px;margin:8px 0;flex-wrap:wrap}
.swatch{width:32px;height:32px;border-radius:8px;cursor:pointer;
  border:2px solid transparent;transition:border-color 0.15s}
.swatch.active{border-color:#7b68ee}
.swatch:hover{border-color:#555}
.tabs{display:flex;gap:4px;margin-bottom:14px}
.tab{flex:1;padding:10px;text-align:center;background:#16213e;border-radius:8px 8px 0 0;
  cursor:pointer;font-size:0.85em;color:#8888aa;border:1px solid #2a2a4a;border-bottom:none}
.tab.active{background:#1a1a2e;color:#e0e0e8;font-weight:600}
.tab-content{display:none}
.tab-content.active{display:block}
.info-row{display:flex;justify-content:space-between;padding:6px 0;
  border-bottom:1px solid #2a2a4a;font-size:0.9em}
.info-label{color:#8888aa}
.info-value{color:#e0e0e8;font-weight:500}
a{color:#7b68ee;text-decoration:none}
a:hover{text-decoration:underline}
.mb-8{margin-bottom:8px}
.mb-12{margin-bottom:12px}
.mt-12{margin-top:12px}
</style>
)rawliteral";

// ─── Login Page ────────────────────────────────────────────────────────────

static const char LOGIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>LED Grid — Login</title>%CSS%</head><body>
<div class="container" style="display:flex;align-items:center;justify-content:center;min-height:80vh">
<div class="card" style="width:100%;max-width:340px;text-align:center">
<h1 style="justify-content:center">🟦 LED Grid</h1>
<p style="color:#8888aa;margin-bottom:16px">Enter password to continue</p>
<form method="POST" action="/login">
<input type="password" name="password" placeholder="Password" autofocus>
<button type="submit" class="btn-primary btn-full">Login</button>
</form>
<p id="err" style="color:#e74c3c;margin-top:10px;font-size:0.85em;display:none">
Incorrect password</p>
</div></div>
<script>if(location.search.indexOf('error')>=0)document.getElementById('err').style.display='block';</script>
</body></html>
)rawliteral";

// ─── Dashboard ─────────────────────────────────────────────────────────────

static const char DASHBOARD_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>LED Grid Control</title>%CSS%</head><body>
<div class="container">
<h1>🟦 LED Grid Control</h1>

<div class="card">
<div class="status-bar">
  <span><span class="dot dot-green" id="wsDot"></span> <span id="uptime">—</span></span>
  <span>Heap: <span id="heap">—</span></span>
  <span><span id="ip">—</span></span>
</div>
</div>

<div class="card">
<h2>Brightness</h2>
<div class="range-row">
  <input type="range" id="brightness" min="0" max="255" value="40">
  <span class="range-val" id="brightnessVal">40</span>
</div>
</div>

<div class="card" style="border-color:#7b68ee44">
<h2>🎮 Games</h2>
<div class="btn-grid">
  <button class="btn-effect btn-game" data-e="0" id="eff0">🧱 Tetris</button>
  <button class="btn-effect btn-game" data-e="17" id="eff17">🐍 Snake</button>
</div>
<a href="/tetris" class="btn btn-primary btn-full mt-12" id="playBtn">🎮 Play Manually</a>
</div>

<div class="card">
<h2>Effects</h2>
<div class="btn-grid">
  <button class="btn-effect" data-e="1" id="eff1">🌈 Rainbow</button>
  <button class="btn-effect" data-e="2" id="eff2">🎨 Colour Wash</button>
  <button class="btn-effect" data-e="3" id="eff3">📐 Diagonal</button>
  <button class="btn-effect" data-e="4" id="eff4">🌧 Rain</button>
  <button class="btn-effect" data-e="5" id="eff5">🕐 Clock</button>
  <button class="btn-effect" data-e="6" id="eff6">🔥 Fire</button>
  <button class="btn-effect" data-e="7" id="eff7">🌌 Aurora</button>
  <button class="btn-effect" data-e="8" id="eff8">🫧 Lava Lamp</button>
  <button class="btn-effect" data-e="9" id="eff9">🕯 Candle</button>
  <button class="btn-effect" data-e="10" id="eff10">✨ Twinkle</button>
  <button class="btn-effect" data-e="11" id="eff11">💚 Matrix</button>
  <button class="btn-effect" data-e="12" id="eff12">🎆 Fireworks</button>
  <button class="btn-effect" data-e="13" id="eff13">🧬 Life</button>
  <button class="btn-effect" data-e="14" id="eff14">🌀 Plasma</button>
  <button class="btn-effect" data-e="15" id="eff15">🌀 Spiral</button>
  <button class="btn-effect" data-e="16" id="eff16">💕 Valentine</button>
</div>
</div>

<div class="card" id="aiCard">
<h2>AI Tuning</h2>
<label>Skill Level</label>
<div class="range-row">
  <input type="range" id="aiSkill" min="0" max="100" value="90">
  <span class="range-val" id="aiSkillVal">90%</span>
</div>
<label>Speed</label>
<div class="range-row">
  <input type="range" id="speed" min="100" max="600" value="300">
  <span class="range-val" id="speedVal">300</span>
</div>
</div>

<div class="card">
<h2>Background Colour</h2>
<p style="font-size:0.8em;color:#6666888;margin-bottom:8px">Subtle tint for empty cells</p>
<div class="colour-swatches" id="swatches">
  <div class="swatch" data-r="0" data-g="0" data-b="0"
       style="background:#000;border:2px solid #3a3a5a" title="Off"></div>
  <div class="swatch" data-r="5" data-g="5" data-b="20"
       style="background:rgb(5,5,20)" title="Deep Blue"></div>
  <div class="swatch" data-r="15" data-g="3" data-b="20"
       style="background:rgb(15,3,20)" title="Dark Purple"></div>
  <div class="swatch" data-r="3" data-g="15" data-b="5"
       style="background:rgb(3,15,5)" title="Dark Green"></div>
  <div class="swatch" data-r="20" data-g="12" data-b="2"
       style="background:rgb(20,12,2)" title="Warm Amber"></div>
  <div class="swatch" data-r="10" data-g="2" data-b="2"
       style="background:rgb(10,2,2)" title="Dark Red"></div>
</div>
<label class="mt-12">Custom RGB (0-40 each)</label>
<div style="display:flex;gap:8px">
  <input type="number" id="bgR" min="0" max="40" value="0" style="width:60px">
  <input type="number" id="bgG" min="0" max="40" value="0" style="width:60px">
  <input type="number" id="bgB" min="0" max="40" value="0" style="width:60px">
  <button class="btn-secondary" onclick="sendBg()">Set</button>
</div>
</div>

<div class="card" style="display:flex;gap:8px;justify-content:space-between">
  <a href="/settings" class="btn btn-secondary" style="flex:1;text-align:center">⚙ Settings</a>
  <button class="btn-primary" style="flex:1" onclick="saveAll()">💾 Save</button>
</div>

</div>
<div class="toast" id="toast"></div>
<script>
var lastTouch=0;
function toast(m,e){var t=document.getElementById('toast');t.textContent=m;
  t.className=e?'toast error show':'toast show';setTimeout(function(){t.className='toast'},2500)}
function post(u,d,cb){var x=new XMLHttpRequest();x.open('POST',u);
  x.setRequestHeader('Content-Type','application/x-www-form-urlencoded');
  x.onload=function(){if(cb)cb(x.status===200)};x.send(d)}

// Brightness
var brSlider=document.getElementById('brightness');
var brVal=document.getElementById('brightnessVal');
brSlider.oninput=function(){brVal.textContent=this.value;lastTouch=Date.now()};
brSlider.onchange=function(){post('/api/brightness','value='+this.value)};

// Effect buttons
document.querySelectorAll('.btn-effect').forEach(function(b){
  b.onclick=function(){
    post('/api/effect','value='+this.dataset.e,function(ok){if(ok)toast('Effect changed')});
  }
});

// AI sliders
var skillSlider=document.getElementById('aiSkill');
var skillVal=document.getElementById('aiSkillVal');
skillSlider.oninput=function(){skillVal.textContent=this.value+'%';lastTouch=Date.now()};
skillSlider.onchange=function(){post('/api/tuning','aiSkillPct='+this.value)};

var speedSlider=document.getElementById('speed');
var speedVal=document.getElementById('speedVal');
speedSlider.oninput=function(){speedVal.textContent=this.value;lastTouch=Date.now()};
speedSlider.onchange=function(){post('/api/tuning','dropStartMs='+this.value)};

// Background swatches
document.querySelectorAll('.swatch').forEach(function(s){
  s.onclick=function(){
    document.getElementById('bgR').value=this.dataset.r;
    document.getElementById('bgG').value=this.dataset.g;
    document.getElementById('bgB').value=this.dataset.b;
    sendBg();
    document.querySelectorAll('.swatch').forEach(function(x){x.classList.remove('active')});
    this.classList.add('active');
  }
});

function sendBg(){
  var r=document.getElementById('bgR').value;
  var g=document.getElementById('bgG').value;
  var b=document.getElementById('bgB').value;
  post('/api/background','r='+r+'&g='+g+'&b='+b,function(ok){if(ok)toast('Background set')});
}

function saveAll(){
  post('/api/save','',function(ok){toast(ok?'Settings saved!':'Save failed',!ok)});
}

// Status polling
function poll(){
  fetch('/api/status').then(function(r){
    if(r.status===401){location.href='/login';return}
    return r.json();
  }).then(function(d){
    if(!d)return;
    document.getElementById('uptime').textContent=d.uptime;
    document.getElementById('heap').textContent=Math.round(d.freeHeap/1024)+'KB';
    document.getElementById('ip').textContent=d.ip;
    if(Date.now()-lastTouch>3000){
      brSlider.value=d.brightness;brVal.textContent=d.brightness;
      skillSlider.value=d.aiSkillPct;skillVal.textContent=d.aiSkillPct+'%';
      speedSlider.value=d.dropStartMs;speedVal.textContent=d.dropStartMs;
      document.getElementById('bgR').value=d.bgR;
      document.getElementById('bgG').value=d.bgG;
      document.getElementById('bgB').value=d.bgB;
    }
    var games=[0,17];
    for(var i=0;i<18;i++){
      var el=document.getElementById('eff'+i);
      if(el){
        var cls='btn-effect';
        if(games.indexOf(i)>=0)cls+=' btn-game';
        if(d.effect===i)cls+=' active';
        el.className=cls;
      }
    }
    var pb=document.getElementById('playBtn');
    if(pb){
      pb.style.display=(games.indexOf(d.effect)>=0)?'block':'none';
    }
  }).catch(function(){});
}
setInterval(poll,3000);
poll();
</script>
</body></html>
)rawliteral";

// ─── Settings Page ─────────────────────────────────────────────────────────

static const char SETTINGS_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>LED Grid — Settings</title>%CSS%</head><body>
<div class="container">
<h1><a href="/" style="text-decoration:none">←</a> Settings</h1>

<div class="tabs">
  <div class="tab active" onclick="showTab(0)">Display</div>
  <div class="tab" onclick="showTab(1)">AI Tuning</div>
  <div class="tab" onclick="showTab(2)">Network</div>
  <div class="tab" onclick="showTab(3)">System</div>
</div>

<div class="tab-content active" id="tab0">
<div class="card">
<h2>Clock</h2>
<label>Time Format</label>
<div style="display:flex;gap:8px;margin-top:4px">
  <button class="btn-secondary" id="btn24h" style="flex:1" onclick="setTimeFormat(true)">24-hour</button>
  <button class="btn-secondary" id="btn12h" style="flex:1" onclick="setTimeFormat(false)">12-hour</button>
</div>
<label class="mt-12">Digit Transition</label>
<div style="display:flex;gap:8px;margin-top:4px">
  <button class="btn-secondary" id="btnTransNone" style="flex:1" onclick="setClockOpt('clockTransition','0')">None</button>
  <button class="btn-secondary" id="btnTransFade" style="flex:1" onclick="setClockOpt('clockTransition','1')">Crossfade</button>
</div>
<div id="fadeRow" class="mt-12">
<label>Fade Duration</label>
<div class="range-row">
  <input type="range" id="s_fadeMs" min="200" max="2000" step="50" value="500">
  <span class="range-val" id="s_fadeMsVal">500ms</span>
</div>
</div>
<label class="mt-12">Digit Colour</label>
<div class="colour-swatches" style="margin-top:4px">
  <div class="swatch" style="background:#E6D2AA" data-col="0" onclick="setClockOpt('clockDigitColour','0')"></div>
  <div class="swatch" style="background:#C8DCFF" data-col="1" onclick="setClockOpt('clockDigitColour','1')"></div>
  <div class="swatch" style="background:#FFB432" data-col="2" onclick="setClockOpt('clockDigitColour','2')"></div>
  <div class="swatch" style="background:#64FF64" data-col="3" onclick="setClockOpt('clockDigitColour','3')"></div>
  <div class="swatch" style="background:#6496FF" data-col="4" onclick="setClockOpt('clockDigitColour','4')"></div>
  <div class="swatch" style="background:#FF6450" data-col="5" onclick="setClockOpt('clockDigitColour','5')"></div>
</div>
<label class="mt-12">Second Trail</label>
<div style="display:flex;gap:8px;margin-top:4px">
  <button class="btn-secondary" id="btnTrailOn" style="flex:1" onclick="setClockOpt('clockTrail','1')">On</button>
  <button class="btn-secondary" id="btnTrailOff" style="flex:1" onclick="setClockOpt('clockTrail','0')">Off</button>
</div>
<label class="mt-12">Minute Marker</label>
<div style="display:flex;gap:8px;margin-top:4px">
  <button class="btn-secondary" id="btnMinOn" style="flex:1" onclick="setClockOpt('clockMinMarker','1')">On</button>
  <button class="btn-secondary" id="btnMinOff" style="flex:1" onclick="setClockOpt('clockMinMarker','0')">Off</button>
</div>
</div>
</div>

<div class="tab-content" id="tab1">
<div class="card">
<h2>Speed Settings</h2>
<label>Drop Start Speed (ms)</label>
<input type="number" id="s_dropStart" min="100" max="600" value="300">
<label>Min Drop Speed (ms)</label>
<input type="number" id="s_dropMin" min="30" max="200" value="80">
<label>Move Interval (ms)</label>
<input type="number" id="s_moveInt" min="20" max="200" value="70">
<label>Rotate Interval (ms)</label>
<input type="number" id="s_rotInt" min="50" max="300" value="150">
</div>
<div class="card">
<h2>AI Behaviour</h2>
<label>Skill Level (%)</label>
<div class="range-row">
  <input type="range" id="s_skill" min="0" max="100" value="90">
  <span class="range-val" id="s_skillVal">90</span>
</div>
<label>Movement Jitter (%)</label>
<div class="range-row">
  <input type="range" id="s_jitter" min="0" max="50" value="20">
  <span class="range-val" id="s_jitterVal">20</span>
</div>
</div>
<button class="btn-primary btn-full" onclick="saveTuning()">Apply & Save</button>
<button class="btn-secondary btn-full mt-12" onclick="restoreDefaults()">Restore Defaults</button>
</div>

<div class="tab-content" id="tab2">
<div class="card">
<h2>Network</h2>
<div class="info-row"><span class="info-label">WiFi</span><span class="info-value" id="n_ssid">—</span></div>
<div class="info-row"><span class="info-label">IP</span><span class="info-value" id="n_ip">—</span></div>
<div class="info-row"><span class="info-label">mDNS</span><span class="info-value">tetris.local</span></div>
</div>
<div class="card">
<h2>MQTT <span id="mqttStatus" style="font-size:0.7em;font-weight:400;margin-left:8px"></span></h2>
<label style="display:flex;align-items:center;gap:8px;margin-bottom:10px;color:#e0e0e8;font-size:0.95em">
  <input type="checkbox" id="mqttEnabled" style="width:18px;height:18px;accent-color:#7b68ee"> Enable MQTT
</label>
<label>Broker Host</label>
<input type="text" id="mqttHost" placeholder="e.g. 192.168.1.100">
<label>Port</label>
<input type="number" id="mqttPort" min="1" max="65535" value="1883" style="width:120px">
<label>Username (optional)</label>
<input type="text" id="mqttUser" placeholder="Username">
<label>Password (optional)</label>
<input type="password" id="mqttPass" placeholder="Password">
<button class="btn-primary btn-full mt-12" onclick="saveMqtt()">Save MQTT Settings</button>
</div>
<div class="card">
<h2>Change Password</h2>
<input type="password" id="newPass" placeholder="New password">
<button class="btn-primary btn-full" onclick="changePass()">Update Password</button>
</div>
</div>

<div class="tab-content" id="tab3">
<div class="card">
<h2>System</h2>
<div class="info-row"><span class="info-label">Firmware</span><span class="info-value" id="sys_ver">—</span></div>
<div class="info-row"><span class="info-label">Uptime</span><span class="info-value" id="sys_up">—</span></div>
<div class="info-row"><span class="info-label">Free Heap</span><span class="info-value" id="sys_heap">—</span></div>
</div>
<div style="display:flex;gap:8px">
  <a href="/update" class="btn btn-secondary" style="flex:1;text-align:center">Firmware Update</a>
  <button class="btn-danger" style="flex:1" onclick="if(confirm('Restart device?'))post('/api/restart','')">Restart</button>
</div>
</div>

</div>
<div class="toast" id="toast"></div>
<script>
function toast(m,e){var t=document.getElementById('toast');t.textContent=m;
  t.className=e?'toast error show':'toast show';setTimeout(function(){t.className='toast'},2500)}
function post(u,d,cb){var x=new XMLHttpRequest();x.open('POST',u);
  x.setRequestHeader('Content-Type','application/x-www-form-urlencoded');
  x.onload=function(){if(cb)cb(x.status===200)};x.send(d)}

function showTab(n){
  document.querySelectorAll('.tab').forEach(function(t,i){t.className='tab'+(i===n?' active':'')});
  document.querySelectorAll('.tab-content').forEach(function(c,i){c.className='tab-content'+(i===n?' active':'')});
}

document.getElementById('s_skill').oninput=function(){document.getElementById('s_skillVal').textContent=this.value};
document.getElementById('s_jitter').oninput=function(){document.getElementById('s_jitterVal').textContent=this.value};

function updateTimeFormatBtns(is24){
  var b24=document.getElementById('btn24h'),b12=document.getElementById('btn12h');
  b24.className=is24?'btn-primary':'btn-secondary';
  b12.className=is24?'btn-secondary':'btn-primary';
}
function setTimeFormat(is24){
  post('/api/tuning','use24Hour='+(is24?'1':'0'),function(ok){
    if(ok){updateTimeFormatBtns(is24);post('/api/save','',function(){toast('Time format saved')})}
    else toast('Failed',true);
  });
}
function setClockOpt(key,val){
  post('/api/tuning',key+'='+val,function(ok){
    if(ok)post('/api/save','',function(){toast('Saved');poll()});
    else toast('Failed',true);
  });
}
function updateClockUI(d){
  var tn=document.getElementById('btnTransNone'),tf=document.getElementById('btnTransFade');
  tn.className=(d.clockTransition===0)?'btn-primary':'btn-secondary';
  tf.className=(d.clockTransition===1)?'btn-primary':'btn-secondary';
  document.getElementById('fadeRow').style.display=(d.clockTransition===1)?'block':'none';
  document.getElementById('s_fadeMs').value=d.clockFadeMs;
  document.getElementById('s_fadeMsVal').textContent=d.clockFadeMs+'ms';
  var mn=document.getElementById('btnMinOn'),mf=document.getElementById('btnMinOff');
  mn.className=d.clockMinMarker?'btn-primary':'btn-secondary';
  mf.className=d.clockMinMarker?'btn-secondary':'btn-primary';
  var swatches=document.querySelectorAll('.swatch[data-col]');
  swatches.forEach(function(sw){sw.className=sw.dataset.col==d.clockDigitColour?'swatch active':'swatch'});
  var ton=document.getElementById('btnTrailOn'),toff=document.getElementById('btnTrailOff');
  ton.className=d.clockTrail?'btn-primary':'btn-secondary';
  toff.className=d.clockTrail?'btn-secondary':'btn-primary';
}
document.getElementById('s_fadeMs').oninput=function(){
  document.getElementById('s_fadeMsVal').textContent=this.value+'ms';
  setClockOpt('clockFadeMs',this.value);
};

function saveTuning(){
  var d='dropStartMs='+document.getElementById('s_dropStart').value+
    '&dropMinMs='+document.getElementById('s_dropMin').value+
    '&moveIntervalMs='+document.getElementById('s_moveInt').value+
    '&rotIntervalMs='+document.getElementById('s_rotInt').value+
    '&aiSkillPct='+document.getElementById('s_skill').value+
    '&jitterPct='+document.getElementById('s_jitter').value;
  post('/api/tuning',d,function(ok){
    if(ok)post('/api/save','',function(){toast('Saved!')});
    else toast('Failed',true);
  });
}
function restoreDefaults(){
  if(!confirm('Restore all settings to defaults?'))return;
  post('/api/defaults','',function(ok){toast(ok?'Defaults restored':'Failed',!ok);if(ok)location.reload()});
}
function saveMqtt(){
  var d='enabled='+(document.getElementById('mqttEnabled').checked?'1':'0')+
    '&host='+encodeURIComponent(document.getElementById('mqttHost').value)+
    '&port='+document.getElementById('mqttPort').value+
    '&username='+encodeURIComponent(document.getElementById('mqttUser').value)+
    '&password='+encodeURIComponent(document.getElementById('mqttPass').value);
  post('/api/mqtt',d,function(ok){toast(ok?'MQTT settings saved':'Failed',!ok);if(ok)setTimeout(poll,2000)});
}
function changePass(){
  var pw=document.getElementById('newPass').value;
  if(!pw){toast('Enter a password',true);return}
  post('/api/save','authPassword='+encodeURIComponent(pw),function(ok){
    toast(ok?'Password updated':'Failed',!ok);document.getElementById('newPass').value='';
  });
}

function poll(){
  fetch('/api/status').then(function(r){
    if(r.status===401){location.href='/login';return}
    return r.json();
  }).then(function(d){
    if(!d)return;
    document.getElementById('n_ssid').textContent=d.ssid||'—';
    document.getElementById('n_ip').textContent=d.ip;
    document.getElementById('sys_ver').textContent=d.version;
    document.getElementById('sys_up').textContent=d.uptime;
    document.getElementById('sys_heap').textContent=Math.round(d.freeHeap/1024)+'KB';
    document.getElementById('s_dropStart').value=d.dropStartMs;
    document.getElementById('s_dropMin').value=d.dropMinMs;
    document.getElementById('s_moveInt').value=d.moveIntervalMs;
    document.getElementById('s_rotInt').value=d.rotIntervalMs;
    document.getElementById('s_skill').value=d.aiSkillPct;
    document.getElementById('s_skillVal').textContent=d.aiSkillPct;
    document.getElementById('s_jitter').value=d.jitterPct;
    document.getElementById('s_jitterVal').textContent=d.jitterPct;
    updateTimeFormatBtns(d.use24Hour);
    updateClockUI(d);
    document.getElementById('mqttEnabled').checked=d.mqttEnabled;
    document.getElementById('mqttHost').value=d.mqttHost||'';
    document.getElementById('mqttPort').value=d.mqttPort||1883;
    document.getElementById('mqttUser').value=d.mqttUsername||'';
    var ms=document.getElementById('mqttStatus');
    if(d.mqttEnabled){
      ms.innerHTML=d.mqttConnected?'<span class="dot dot-green"></span> Connected':'<span class="dot dot-red"></span> Disconnected';
    }else{ms.textContent='Disabled'}
  }).catch(function(){});
}
poll();
</script>
</body></html>
)rawliteral";

// ─── Manual Tetris Page ────────────────────────────────────────────────────

static const char TETRIS_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>LED Grid — Tetris</title>%CSS%
<style>
#grid{display:block;margin:0 auto;border-radius:8px;image-rendering:pixelated;
  border:2px solid #2a2a4a;touch-action:none;width:100%;max-width:320px;aspect-ratio:1}
.controls{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px;max-width:280px;margin:16px auto 0}
.ctrl-btn{width:100%;aspect-ratio:1;border:none;border-radius:12px;background:#1e2a4a;
  color:#e0e0e8;font-size:1.8em;cursor:pointer;display:flex;align-items:center;
  justify-content:center;-webkit-user-select:none;user-select:none;
  transition:background 0.1s,transform 0.08s;touch-action:manipulation}
.ctrl-btn:active{background:#7b68ee;transform:scale(0.92)}
.stats{display:flex;justify-content:space-around;font-size:0.95em;padding:10px 0}
.stat-val{font-size:1.4em;font-weight:700;color:#7b68ee}
</style>
</head><body>
<div class="container">
<h1 style="font-size:1.2em">
  <a href="/" style="text-decoration:none">←</a>
  🎮 Manual Tetris
  <span style="margin-left:auto"><span class="dot" id="wsDot"></span></span>
</h1>

<canvas id="grid" width="256" height="256"></canvas>

<div class="stats">
  <div style="text-align:center"><div class="stat-val" id="score">0</div><div style="color:#8888aa;font-size:0.8em">Score</div></div>
  <div style="text-align:center"><div class="stat-val" id="lines">0</div><div style="color:#8888aa;font-size:0.8em">Lines</div></div>
</div>

<div class="controls">
  <div></div>
  <button class="ctrl-btn" id="btnRot">↻</button>
  <div></div>
  <button class="ctrl-btn" id="btnL">←</button>
  <button class="ctrl-btn" id="btnD">⬇</button>
  <button class="ctrl-btn" id="btnR">→</button>
</div>

<button class="btn-secondary btn-full mt-12" onclick="backToAI()">← Back to AI Mode</button>
</div>

<script>
var canvas=document.getElementById('grid');
var ctx=canvas.getContext('2d');
var ws=null;
var wsUrl='ws://'+location.hostname+':81/';
var wsToken='';
var reconnectMs=1000;
var maxReconnectMs=16000;

function connect(){
  if(!wsToken){
    fetch('/api/ws-token').then(function(r){
      if(r.status===401){location.href='/login';return}
      return r.json();
    }).then(function(d){
      if(d&&d.token){wsToken=d.token;connect()}
    }).catch(function(){setTimeout(connect,3000)});
    return;
  }
  ws=new WebSocket(wsUrl);
  ws.onopen=function(){
    document.getElementById('wsDot').className='dot dot-amber';
    ws.send(JSON.stringify({cmd:'auth',token:wsToken}));
  };
  ws.onmessage=function(e){
    try{
      var d=JSON.parse(e.data);
      if(d.auth===true){
        document.getElementById('wsDot').className='dot dot-green';
        reconnectMs=1000;
        send('manual');
        return;
      }
      if(d.auth===false){
        wsToken='';
        ws.close();
        return;
      }
      drawGrid(d.grid);
      document.getElementById('score').textContent=d.score||0;
      document.getElementById('lines').textContent=d.lines||0;
    }catch(ex){}
  };
  ws.onclose=function(){
    document.getElementById('wsDot').className='dot dot-red';
    setTimeout(connect,reconnectMs);
    reconnectMs=Math.min(reconnectMs*2,maxReconnectMs);
  };
  ws.onerror=function(){ws.close()};
}

function send(cmd){
  if(ws&&ws.readyState===1)ws.send(JSON.stringify({cmd:cmd}));
}

function backToAI(){
  send('ai');
  setTimeout(function(){location.href='/'},300);
}

function drawGrid(grid){
  if(!grid||grid.length<256)return;
  var px=canvas.width/16;
  ctx.fillStyle='#0f0f1a';
  ctx.fillRect(0,0,canvas.width,canvas.height);
  for(var i=0;i<256;i++){
    var x=(15-i%16)*px, y=Math.floor(i/16)*px;
    var c=grid[i];
    if(c===0){continue;}
    var r=(c>>16)&0xFF, g=(c>>8)&0xFF, b=c&0xFF;
    ctx.fillStyle='rgb('+r+','+g+','+b+')';
    ctx.fillRect(x+0.5,y+0.5,px-1,px-1);
  }
}

// Swipe detection on canvas
var touchX=0,touchY=0;
canvas.addEventListener('touchstart',function(e){
  var t=e.touches[0];touchX=t.clientX;touchY=t.clientY;e.preventDefault();
},{passive:false});
canvas.addEventListener('touchend',function(e){
  var t=e.changedTouches[0];
  var dx=t.clientX-touchX, dy=t.clientY-touchY;
  var ax=Math.abs(dx), ay=Math.abs(dy);
  if(ax<30&&ay<30)return;
  if(ay>ax){
    if(dy>0)send('drop');
  }else{
    send(dx>0?'left':'right');
  }
  e.preventDefault();
},{passive:false});

// D-pad buttons — use touchstart on mobile, click on desktop, never both
var usedTouch=false;
function bindBtn(id,cmd){
  var el=document.getElementById(id);
  el.addEventListener('touchstart',function(e){usedTouch=true;send(cmd);e.preventDefault()},{passive:false});
  el.addEventListener('click',function(){if(!usedTouch)send(cmd)});
}
bindBtn('btnRot','rotate');
bindBtn('btnL','right');
bindBtn('btnD','drop');
bindBtn('btnR','left');

// Keyboard support
document.addEventListener('keydown',function(e){
  switch(e.key){
    case 'ArrowLeft':send('right');break;
    case 'ArrowRight':send('left');break;
    case 'ArrowDown':send('drop');break;
    case 'ArrowUp':case ' ':send('rotate');break;
  }
});

connect();
</script>
</body></html>
)rawliteral";

// ─── OTA Update Page ───────────────────────────────────────────────────────

static const char UPDATE_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>LED Grid — Firmware Update</title>%CSS%</head><body>
<div class="container">
<h1><a href="/settings" style="text-decoration:none">←</a> Firmware Update</h1>
<div class="card">
<p style="color:#8888aa;margin-bottom:12px">Select a .bin firmware file to upload.</p>
<form method="POST" action="/update" enctype="multipart/form-data" id="uploadForm">
<input type="file" name="firmware" accept=".bin" id="fileInput"
  style="margin-bottom:12px;color:#e0e0e8">
<button type="submit" class="btn-primary btn-full" id="uploadBtn">Upload</button>
</form>
<div id="progress" style="display:none;margin-top:12px">
  <div style="background:#2a2a4a;border-radius:6px;overflow:hidden;height:20px">
    <div id="bar" style="background:#7b68ee;height:100%;width:0%;transition:width 0.3s"></div>
  </div>
  <p style="text-align:center;margin-top:6px;font-size:0.85em" id="pctText">0%</p>
</div>
</div></div>
<script>
document.getElementById('uploadForm').onsubmit=function(e){
  e.preventDefault();
  var f=document.getElementById('fileInput').files[0];
  if(!f)return;
  var xhr=new XMLHttpRequest();
  var fd=new FormData();
  fd.append('firmware',f);
  document.getElementById('progress').style.display='block';
  document.getElementById('uploadBtn').disabled=true;
  xhr.upload.onprogress=function(e){
    if(e.lengthComputable){
      var pct=Math.round(e.loaded/e.total*100);
      document.getElementById('bar').style.width=pct+'%';
      document.getElementById('pctText').textContent=pct+'%';
    }
  };
  xhr.onload=function(){
    document.getElementById('pctText').textContent='Done! Rebooting...';
    setTimeout(function(){location.href='/'},5000);
  };
  xhr.open('POST','/update');
  xhr.send(fd);
};
</script>
</body></html>
)rawliteral";

#endif // HTML_PAGES_H
