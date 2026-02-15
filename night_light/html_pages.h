#ifndef HTML_PAGES_H
#define HTML_PAGES_H

#include <Arduino.h>

// ─── Shared CSS ─────────────────────────────────────────────────────────────
static const char SHARED_CSS[] PROGMEM = R"rawliteral(
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;
  background:#1a1a2e;color:#e0e0e0;min-height:100vh}
.container{max-width:480px;margin:0 auto;padding:16px}
h1{font-size:1.4em;margin-bottom:12px;color:#f0f0f0}
h2{font-size:1.1em;margin-bottom:8px;color:#c0c0ff}
.card{background:#16213e;border-radius:12px;padding:16px;margin-bottom:14px;
  box-shadow:0 2px 8px rgba(0,0,0,0.3)}
.card h2{margin-bottom:10px}
label{display:block;font-size:0.85em;color:#a0a0c0;margin-bottom:4px;margin-top:10px}
input[type=text],input[type=password],input[type=number],input[type=time],
select{width:100%;padding:10px;border:1px solid #2a2a4a;border-radius:8px;
  background:#0f1a30;color:#e0e0e0;font-size:0.95em}
input[type=color]{width:60px;height:36px;border:none;border-radius:8px;
  background:transparent;cursor:pointer}
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
.swatch{display:inline-block;width:24px;height:24px;border-radius:6px;
  border:1px solid #444;vertical-align:middle;margin-right:6px}
.toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);
  background:#2ecc71;color:#fff;padding:12px 24px;border-radius:8px;
  font-size:0.9em;opacity:0;transition:opacity 0.3s;z-index:999;
  pointer-events:none}
.toast.show{opacity:1}
.toast.error{background:#e74c3c}
.toggle{position:relative;display:inline-block;width:48px;height:26px}
.toggle input{opacity:0;width:0;height:0}
.toggle .slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;
  background:#444;border-radius:26px;transition:.3s}
.toggle .slider:before{content:"";position:absolute;height:20px;width:20px;
  left:3px;bottom:3px;background:#fff;border-radius:50%;transition:.3s}
.toggle input:checked+.slider{background:#7b68ee}
.toggle input:checked+.slider:before{transform:translateX(22px)}
.slot-card{background:#0f1a30;border-radius:8px;padding:12px;margin-bottom:10px;
  border:1px solid #2a2a4a}
.slot-header{display:flex;justify-content:space-between;align-items:center;
  margin-bottom:8px}
a{color:#7b68ee;text-decoration:none}
a:hover{text-decoration:underline}
.footer{text-align:center;font-size:0.75em;color:#666;margin-top:20px;padding:10px}
.tl-wrap{background:#0f1a30;border-radius:8px;overflow:hidden;position:relative;height:36px}
.tl-bar{position:absolute;top:0;height:100%;border-radius:3px;opacity:0.85;
  min-width:2px;transition:left 0.2s,width 0.2s}
.tl-bar.disabled{opacity:0.25}
.tl-hours{display:flex;justify-content:space-between;font-size:0.7em;color:#666;
  padding:2px 0 0;user-select:none}
.tl-now{position:absolute;top:0;height:100%;width:2px;background:#7b68ee;z-index:2;
  border-radius:1px}
.tl-label{position:absolute;top:50%;transform:translateY(-50%);left:4px;
  font-size:0.65em;color:#fff;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;
  pointer-events:none;text-shadow:0 1px 2px rgba(0,0,0,0.6)}
)rawliteral";

// ─── Login Page ─────────────────────────────────────────────────────────────
static const char LOGIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Night Light</title>
<style>%CSS%
.login-box{max-width:320px;margin:80px auto;text-align:center}
.login-box h1{font-size:2em;margin-bottom:4px}
.login-box .subtitle{color:#888;margin-bottom:24px;font-size:0.9em}
.login-box .icon{font-size:3em;margin-bottom:12px}
.error-msg{color:#e74c3c;font-size:0.85em;margin-top:8px;display:none}
</style></head><body>
<div class="login-box">
  <div class="icon">&#127769;</div>
  <h1>Night Light</h1>
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
<title>Night Light</title>
<style>%CSS%</style></head><body>
<div class="container">
  <h1>&#127769; Night Light</h1>

  <!-- Status -->
  <div class="card">
    <h2>Status</h2>
    <p><span class="status-dot" id="timeDot"></span>
       Time: <strong id="curTime">--:--:--</strong></p>
    <p style="margin-top:6px">
      <span class="swatch" id="curSwatch"></span>
      Active: <strong id="curSlot">—</strong></p>
    <div style="margin-top:10px">
      <label class="toggle">
        <input type="checkbox" id="masterToggle" onchange="toggleLeds()">
        <span class="slider"></span>
      </label>
      <span style="margin-left:8px">LEDs enabled</span>
    </div>
  </div>

  <!-- Pretty mode banner -->
  <div class="card" id="prettyBanner" style="display:none;background:#5b4a9e;text-align:center">
    <strong>Manual override active</strong>
    <button class="btn btn-secondary btn-sm" style="margin-left:10px"
      onclick="sendEffect('-1')">Return to schedule</button>
  </div>

  <!-- Quick Controls -->
  <div class="card">
    <h2>Quick Controls</h2>
    <div class="row">
      <div><label>Colour</label>
        <input type="color" id="colourPick" value="#ff8c14"
               onchange="userTouched();sendColour(this.value)"></div>
      <div><label>Warm White (<span id="wVal">0</span>)</label>
        <input type="range" id="wSlider" min="0" max="255" value="0"
               oninput="userTouched();document.getElementById('wVal').textContent=this.value;sendWhite(this.value)"></div>
    </div>
    <label>Brightness</label>
    <input type="range" id="briSlider" min="0" max="255" value="128"
           oninput="userTouched();sendBrightness(this.value)">
    <label>Effect Override</label>
    <select id="effectSel" onchange="userTouched();sendEffect(this.value)">
      <option value="-1">— Use schedule —</option>
      <option value="0">Solid</option>
      <option value="1">Breathing</option>
      <option value="2">Soft Glow</option>
      <option value="3">Starry Twinkle</option>
      <option value="4">Rainbow</option>
      <option value="5">Candle Flicker</option>
      <option value="6">Sunrise</option>
      <option value="7">Christmas</option>
      <option value="8">Halloween</option>
      <option value="9">Birthday</option>
    </select>
  </div>

  <!-- Schedule Summary -->
  <div class="card">
    <h2>Schedule</h2>
    <div class="tl-wrap" id="dashTimeline"></div>
    <div class="tl-hours" id="dashTlHours"></div>
    <div id="schedSummary" style="margin-top:8px">Loading...</div>
    <a href="/schedule" class="btn btn-secondary btn-block" style="margin-top:10px">
      Edit Schedule</a>
  </div>

  <!-- Special Dates -->
  <div class="card">
    <h2>Special Dates</h2>
    <p style="font-size:0.85em;color:#a0a0c0;margin-bottom:10px">
      Set birthdays or holidays for automatic themed effects.</p>
    <div id="specialDatesContainer"></div>
    <button class="btn btn-secondary btn-sm" id="addDateBtn" onclick="addSpecialDate()">
      + Add Date</button>
    <button class="btn btn-primary btn-block" onclick="saveSpecialDates()">
      Save Special Dates</button>
  </div>

  <div class="footer">
    <a href="/settings">Settings</a> &middot;
    <a href="/update">Firmware Update</a> &middot;
    <a href="/logout">Log out</a> &middot;
    <span id="ipAddr"></span><br>
    <span id="wifiInd"></span> &middot;
    <span id="mqttInd"></span>
    <span id="fwVersion"></span> &middot;
    <span id="uptime"></span> &middot;
    <span id="freeHeap"></span> &middot;
    <span id="chipTemp"></span>
  </div>
</div>

<div class="toast" id="toast"></div>

<script>
var lastUserInput=0;
function userTouched(){lastUserInput=Date.now();}
function recentlyTouched(){return Date.now()-lastUserInput<3000;}
function toast(msg, err){
  var t=document.getElementById('toast');
  t.textContent=msg;
  t.className=err?'toast error show':'toast show';
  setTimeout(function(){t.className='toast'},2500);
}
function pad0(n){return n<10?'0'+n:''+n}
function renderDashTimeline(slots,timeStr){
  var tl=document.getElementById('dashTimeline');
  var total=1440;
  var html='';
  if(slots){
    slots.forEach(function(s){
      var sp=s.start.split(':'),ep=s.end.split(':');
      var sm=parseInt(sp[0])*60+parseInt(sp[1]);
      var em=parseInt(ep[0])*60+parseInt(ep[1]);
      var col='rgb('+s.r+','+s.g+','+s.b+')';
      if(sm===em)return;
      if(sm<em){
        var left=(sm/total*100).toFixed(2);
        var width=((em-sm)/total*100).toFixed(2);
        html+='<div class="tl-bar" style="left:'+left+'%;width:'+width+'%;background:'+col+'">'
          +'<span class="tl-label">'+s.label+'</span></div>';
      } else {
        var left1=(sm/total*100).toFixed(2);
        var width1=((total-sm)/total*100).toFixed(2);
        var width2=(em/total*100).toFixed(2);
        html+='<div class="tl-bar" style="left:'+left1+'%;width:'+width1+'%;background:'+col+';border-radius:3px 0 0 3px">'
          +'<span class="tl-label">'+s.label+'</span></div>';
        html+='<div class="tl-bar" style="left:0;width:'+width2+'%;background:'+col+';border-radius:0 3px 3px 0"></div>';
      }
    });
  }
  if(timeStr&&timeStr.indexOf('--')<0){
    var tp=timeStr.split(':');
    var nowMin=parseInt(tp[0])*60+parseInt(tp[1]);
    var nowPct=(nowMin/total*100).toFixed(2);
    html+='<div class="tl-now" style="left:'+nowPct+'%"></div>';
  }
  tl.innerHTML=html;
  var hrs=document.getElementById('dashTlHours');
  if(hrs.childElementCount===0){
    var h='';
    for(var i=0;i<=24;i+=3){h+='<span>'+pad0(i%24)+'</span>';}
    hrs.innerHTML=h;
  }
}
function fetchStatus(){
  fetch('/api/status').then(r=>r.json()).then(d=>{
    document.getElementById('curTime').textContent=d.time||'--:--:--';
    var dot=document.getElementById('timeDot');
    dot.className='status-dot '+(d.synced?'dot-green':'dot-amber');
    document.getElementById('curSlot').textContent=d.slotLabel||'None';
    var sw=document.getElementById('curSwatch');
    sw.style.background=d.slotColour||'#333';
    document.getElementById('masterToggle').checked=d.ledsOn;
    if(!recentlyTouched()){
      document.getElementById('briSlider').value=d.brightness||0;
      if(d.slotColour)document.getElementById('colourPick').value=d.slotColour;
      document.getElementById('wSlider').value=d.activeWhite||0;
      document.getElementById('wVal').textContent=d.activeWhite||0;
      if(d.manualOverride){
        document.getElementById('effectSel').value=d.activeEffect;
      }else{
        document.getElementById('effectSel').value='-1';
      }
    }
    // Pretty mode banner
    document.getElementById('prettyBanner').style.display=d.manualOverride?'block':'none';
    // Footer info
    document.getElementById('ipAddr').textContent=d.ip||'';
    if(d.version)document.getElementById('fwVersion').textContent='v'+d.version;
    if(d.uptime)document.getElementById('uptime').textContent='Up: '+d.uptime;
    if(d.freeHeap)document.getElementById('freeHeap').textContent=Math.round(d.freeHeap/1024)+'KB free';
    if(d.chipTemp!==undefined){
      var te=document.getElementById('chipTemp');
      te.textContent=d.chipTemp.toFixed(1)+'\u00B0C';
      te.style.color=d.thermalThrottle?'#e74c3c':(d.chipTemp>55?'#f39c12':'#a0a0c0');
    }
    // WiFi/MQTT connection indicators
    var wi=document.getElementById('wifiInd');
    wi.textContent='WiFi';
    wi.style.color=d.wifiConnected?'#2ecc71':'#e74c3c';
    var mi=document.getElementById('mqttInd');
    if(d.mqttEnabled){
      mi.textContent='MQTT';
      mi.style.color=d.mqttConnected?'#2ecc71':'#e74c3c';
    }else{mi.textContent='';}
    // Populate special dates only on first load
    if(!specialDatesLoaded && d.specialDates){
      specialDatesLoaded=true;
      specialDates=d.specialDates;
      renderSpecialDates();
    }
    // Schedule timeline + summary
    renderDashTimeline(d.slots,d.time);
    var html='';
    if(d.slots){
      d.slots.forEach(function(s,i){
        html+='<div style="margin:4px 0"><span class="swatch" style="background:rgb('
          +s.r+','+s.g+','+s.b+')"></span>'
          +'<strong>'+s.label+'</strong> '
          +s.start+' – '+s.end
          +(s.active?' &#9664;':'')+'</div>';
      });
    }
    document.getElementById('schedSummary').innerHTML=html||'No slots configured';
  }).catch(function(){});
}
function toggleLeds(){
  var on=document.getElementById('masterToggle').checked;
  fetch('/api/settings',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'ledsOn='+(on?'1':'0')}).then(function(){toast(on?'LEDs on':'LEDs off')});
}
function sendBrightness(v){
  fetch('/api/brightness',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'brightness='+v});
}
function sendColour(hex){
  var r=parseInt(hex.substr(1,2),16);
  var g=parseInt(hex.substr(3,2),16);
  var b=parseInt(hex.substr(5,2),16);
  fetch('/api/colour',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'r='+r+'&g='+g+'&b='+b});
}
function sendWhite(v){
  fetch('/api/colour',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'w='+v});
}
function sendEffect(v){
  fetch('/api/effect',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'effect='+v}).then(function(){
      toast(v=='-1'?'Using schedule':'Effect override set');
    });
}
var specialDates=[];
var specialDatesLoaded=false;
function hex2(n){return ('0'+(n||0).toString(16)).slice(-2)}
function hexToRgb2(h){
  return {r:parseInt(h.substr(1,2),16),g:parseInt(h.substr(3,2),16),b:parseInt(h.substr(5,2),16)};
}
function renderSpecialDates(){
  var html='';
  specialDates.forEach(function(sd,i){
    html+='<div class="slot-card" style="margin-bottom:8px">'
      +'<div class="slot-header">'
      +'<strong>#'+(i+1)+'</strong>'
      +'<label class="toggle"><input type="checkbox" '
      +(sd.enabled?'checked':'')+' onchange="specialDates['+i+'].enabled=this.checked">'
      +'<span class="slider"></span></label>'
      +'<button class="btn btn-danger btn-sm" onclick="removeSpecialDate('+i+')">Delete</button>'
      +'</div>'
      +'<label>Label</label>'
      +'<input type="text" value="'+(sd.label||'')+'" maxlength="19" '
      +'onchange="specialDates['+i+'].label=this.value">'
      +'<div class="row">'
      +'<div><label>Month</label><select onchange="specialDates['+i+'].month=+this.value">';
    for(var m=1;m<=12;m++){
      html+='<option value="'+m+'"'+(sd.month==m?' selected':'')+'>'+m+'</option>';
    }
    html+='</select></div>'
      +'<div><label>Day</label><select onchange="specialDates['+i+'].day=+this.value">';
    for(var d=1;d<=31;d++){
      html+='<option value="'+d+'"'+(sd.day==d?' selected':'')+'>'+d+'</option>';
    }
    html+='</select></div></div>'
      +'<div class="row">'
      +'<div><label>Colour</label><input type="color" value="#'
      +hex2(sd.red||255)+hex2(sd.green||255)+hex2(sd.blue||255)+'" '
      +'onchange="var c=hexToRgb2(this.value);specialDates['+i+'].red=c.r;specialDates['+i+'].green=c.g;specialDates['+i+'].blue=c.b"></div>'
      +'<div><label>Warm White ('+(sd.white||0)+')</label>'
      +'<input type="range" min="0" max="255" value="'+(sd.white||0)+'" '
      +'oninput="specialDates['+i+'].white=+this.value;'
      +'this.previousElementSibling.textContent=\'Warm White (\'+this.value+\')\'"></div>'
      +'</div>'
      +'<label>Effect</label>'
      +'<select onchange="specialDates['+i+'].effect=+this.value">'
      +'<option value="7"'+(sd.effect==7?' selected':'')+'>Christmas</option>'
      +'<option value="8"'+(sd.effect==8?' selected':'')+'>Halloween</option>'
      +'<option value="9"'+(sd.effect==9?' selected':'')+'>Birthday</option>'
      +'<option value="0"'+(sd.effect==0?' selected':'')+'>Solid</option>'
      +'<option value="1"'+(sd.effect==1?' selected':'')+'>Breathing</option>'
      +'<option value="2"'+(sd.effect==2?' selected':'')+'>Soft Glow</option>'
      +'<option value="3"'+(sd.effect==3?' selected':'')+'>Starry Twinkle</option>'
      +'<option value="4"'+(sd.effect==4?' selected':'')+'>Rainbow</option>'
      +'<option value="5"'+(sd.effect==5?' selected':'')+'>Candle Flicker</option>'
      +'<option value="6"'+(sd.effect==6?' selected':'')+'>Sunrise</option>'
      +'</select>'
      +'<label>Brightness ('+Math.round((sd.brightness||128)/2.55)+'%)</label>'
      +'<input type="range" min="0" max="255" value="'+(sd.brightness||128)+'" '
      +'oninput="specialDates['+i+'].brightness=+this.value;'
      +'this.previousElementSibling.textContent=\'Brightness (\'+Math.round(this.value/2.55)+\'%)\'\">'
      +'</div>';
  });
  document.getElementById('specialDatesContainer').innerHTML=html;
  document.getElementById('addDateBtn').style.display=specialDates.length>=4?'none':'inline-block';
}
function addSpecialDate(){
  if(specialDates.length>=4)return;
  specialDates.push({enabled:true,month:1,day:1,effect:9,brightness:128,red:255,green:255,blue:255,white:0,label:'Birthday'});
  renderSpecialDates();
}
function removeSpecialDate(i){
  specialDates.splice(i,1);
  renderSpecialDates();
}
function saveSpecialDates(){
  fetch('/api/special-dates',{method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify({dates:specialDates})
  }).then(function(r){
    if(r.ok){toast('Special dates saved!');}
    else{toast('Save failed',true);}
  }).catch(function(){toast('Save failed',true)});
}
fetchStatus();
setInterval(fetchStatus,5000);
</script>
</body></html>
)rawliteral";

// ─── Schedule Editor Page ───────────────────────────────────────────────────
static const char SCHEDULE_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Schedule — Night Light</title>
<style>%CSS%</style></head><body>
<div class="container">
  <p><a href="/">&larr; Back to dashboard</a></p>
  <h1>&#128197; Schedule Editor</h1>
  <div class="card" style="padding:12px">
    <div class="tl-wrap" id="timeline"></div>
    <div class="tl-hours" id="tlHours"></div>
  </div>
  <div id="slots"></div>
  <button class="btn btn-secondary btn-block" id="addBtn" onclick="addSlot()">
    + Add Time Slot</button>
  <button class="btn btn-primary btn-block" onclick="saveSchedule()">
    Save Schedule</button>
  <button class="btn btn-danger btn-sm" style="margin-top:10px"
    onclick="if(confirm('Reset schedule to defaults?'))fetch('/api/reset',{method:'POST'}).then(()=>location.reload())">
    Reset to Defaults</button>
</div>
<div class="toast" id="toast"></div>
<script>
var MAX_SLOTS=8;
var slots=[];

function toast(msg,err){
  var t=document.getElementById('toast');
  t.textContent=msg;
  t.className=err?'toast error show':'toast show';
  setTimeout(function(){t.className='toast'},2500);
}
function pad(n){return n<10?'0'+n:''+n}

function renderSlots(){
  var html='';
  slots.forEach(function(s,i){
    html+='<div class="slot-card" id="slot'+i+'">'
      +'<div class="slot-header">'
      +'<strong>#'+(i+1)+'</strong>'
      +'<label class="toggle"><input type="checkbox" '
      +(s.enabled?'checked':'')+' onchange="slots['+i+'].enabled=this.checked">'
      +'<span class="slider"></span></label>'
      +'<button class="btn btn-secondary btn-sm" onclick="moveSlot('+i+',-1)"'
      +(i===0?' disabled':'')+'>&#9650;</button>'
      +'<button class="btn btn-secondary btn-sm" onclick="moveSlot('+i+',1)"'
      +(i===slots.length-1?' disabled':'')+'>&#9660;</button>'
      +'<button class="btn btn-danger btn-sm" onclick="removeSlot('+i+')">Delete</button>'
      +'</div>'
      +'<label>Label</label>'
      +'<input type="text" value="'+s.label+'" maxlength="19" '
      +'onchange="slots['+i+'].label=this.value">'
      +'<div class="row">'
      +'<div><label>Start</label><input type="time" value="'
      +pad(s.startHour)+':'+pad(s.startMinute)+'" '
      +'onchange="var p=this.value.split(\':\');slots['+i+'].startHour=+p[0];slots['+i+'].startMinute=+p[1]"></div>'
      +'<div><label>End</label><input type="time" value="'
      +pad(s.endHour)+':'+pad(s.endMinute)+'" '
      +'onchange="var p=this.value.split(\':\');slots['+i+'].endHour=+p[0];slots['+i+'].endMinute=+p[1]"></div>'
      +'</div>'
      +'<div class="row">'
      +'<div><label>Colour</label><input type="color" value="#'
      +hex(s.red)+hex(s.green)+hex(s.blue)+'" '
      +'onchange="var c=hexToRgb(this.value);slots['+i+'].red=c.r;slots['+i+'].green=c.g;slots['+i+'].blue=c.b"></div>'
      +'<div><label>Brightness ('+Math.round(s.brightness/2.55)+'%)</label>'
      +'<input type="range" min="0" max="255" value="'+s.brightness+'" '
      +'oninput="slots['+i+'].brightness=+this.value;'
      +'this.previousElementSibling.textContent=\'Brightness (\'+Math.round(this.value/2.55)+\'%)\'"></div>'
      +'</div>'
      +'<label>Warm White ('+(s.white||0)+')</label>'
      +'<input type="range" min="0" max="255" value="'+(s.white||0)+'" '
      +'oninput="slots['+i+'].white=+this.value;'
      +'this.previousElementSibling.textContent=\'Warm White (\'+this.value+\')\'\">'
      +'<label>Effect</label>'
      +'<select onchange="slots['+i+'].effect=+this.value">'
      +'<option value="0"'+(s.effect==0?' selected':'')+'>Solid</option>'
      +'<option value="1"'+(s.effect==1?' selected':'')+'>Breathing</option>'
      +'<option value="2"'+(s.effect==2?' selected':'')+'>Soft Glow</option>'
      +'<option value="3"'+(s.effect==3?' selected':'')+'>Starry Twinkle</option>'
      +'<option value="4"'+(s.effect==4?' selected':'')+'>Rainbow</option>'
      +'<option value="5"'+(s.effect==5?' selected':'')+'>Candle Flicker</option>'
      +'<option value="6"'+(s.effect==6?' selected':'')+'>Sunrise</option>'
      +'<option value="7"'+(s.effect==7?' selected':'')+'>Christmas</option>'
      +'<option value="8"'+(s.effect==8?' selected':'')+'>Halloween</option>'
      +'<option value="9"'+(s.effect==9?' selected':'')+'>Birthday</option>'
      +'</select>'
      +'</div>';
  });
  document.getElementById('slots').innerHTML=html;
  document.getElementById('addBtn').style.display=slots.length>=MAX_SLOTS?'none':'block';
  renderTimeline();
}

function hex(n){return ('0'+n.toString(16)).slice(-2)}
function hexToRgb(h){
  var r=parseInt(h.substr(1,2),16);
  var g=parseInt(h.substr(3,2),16);
  var b=parseInt(h.substr(5,2),16);
  return {r:r,g:g,b:b};
}

function addSlot(){
  if(slots.length>=MAX_SLOTS)return;
  slots.push({enabled:true,startHour:20,startMinute:0,endHour:6,endMinute:0,
    red:255,green:100,blue:0,white:0,brightness:50,effect:0,label:'New slot'});
  renderSlots();
}

function removeSlot(i){
  slots.splice(i,1);
  renderSlots();
}

function moveSlot(i,dir){
  var j=i+dir;
  if(j<0||j>=slots.length)return;
  var tmp=slots[i];
  slots[i]=slots[j];
  slots[j]=tmp;
  renderSlots();
}

function slotActive(s,m){
  // Is minute m inside slot s? Handles overnight ranges
  var start=s.startHour*60+s.startMinute;
  var end=s.endHour*60+s.endMinute;
  if(start<=end)return m>=start&&m<end;
  return m>=start||m<end;
}

function renderTimeline(){
  var tl=document.getElementById('timeline');
  var html='';
  var total=1440; // minutes in a day
  slots.forEach(function(s,i){
    if(!s.enabled&&slots.some(function(x){return x.enabled}))var cls=' disabled';else var cls='';
    var sm=s.startHour*60+s.startMinute;
    var em=s.endHour*60+s.endMinute;
    var col='rgb('+s.red+','+s.green+','+s.blue+')';
    var lbl=s.label||'#'+(i+1);
    if(sm===em) return; // zero-duration
    if(sm<em){
      // Normal range
      var left=(sm/total*100).toFixed(2);
      var width=((em-sm)/total*100).toFixed(2);
      html+='<div class="tl-bar'+cls+'" style="left:'+left+'%;width:'+width+'%;background:'+col+'">'
        +'<span class="tl-label">'+lbl+'</span></div>';
    } else {
      // Overnight: two segments
      var left1=(sm/total*100).toFixed(2);
      var width1=((total-sm)/total*100).toFixed(2);
      var width2=(em/total*100).toFixed(2);
      html+='<div class="tl-bar'+cls+'" style="left:'+left1+'%;width:'+width1+'%;background:'+col+';border-radius:3px 0 0 3px">'
        +'<span class="tl-label">'+lbl+'</span></div>';
      html+='<div class="tl-bar'+cls+'" style="left:0;width:'+width2+'%;background:'+col+';border-radius:0 3px 3px 0"></div>';
    }
  });
  tl.innerHTML=html;
  // Hour labels
  var hrs=document.getElementById('tlHours');
  if(hrs.childElementCount===0){
    var h='';
    for(var i=0;i<=24;i+=3){h+='<span>'+pad(i%24)+'</span>';}
    hrs.innerHTML=h;
  }
}

function checkOverlaps(){
  var warnings=[];
  for(var i=0;i<slots.length;i++){
    if(!slots[i].enabled)continue;
    for(var j=i+1;j<slots.length;j++){
      if(!slots[j].enabled)continue;
      // Check if any minute is active in both slots
      var overlap=false;
      var si=slots[i],sj=slots[j];
      var as=si.startHour*60+si.startMinute;
      var ae=si.endHour*60+si.endMinute;
      var bs=sj.startHour*60+sj.startMinute;
      var be=sj.endHour*60+sj.endMinute;
      // Check boundary points of each slot in the other
      var points=[as,ae>0?ae-1:1439,bs,be>0?be-1:1439];
      for(var p=0;p<points.length;p++){
        if(slotActive(si,points[p])&&slotActive(sj,points[p])){overlap=true;break;}
      }
      if(overlap){
        warnings.push((si.label||'#'+(i+1))+' and '+(sj.label||'#'+(j+1)));
      }
    }
  }
  return warnings;
}

function saveSchedule(){
  var overlaps=checkOverlaps();
  if(overlaps.length>0){
    toast('Warning: overlapping slots: '+overlaps.join(', '),true);
  }
  fetch('/schedule/save',{method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify({slots:slots})
  }).then(function(r){
    if(r.ok){toast('Schedule saved!');}
    else{toast('Save failed',true);}
  }).catch(function(){toast('Save failed',true)});
}

// Re-render timeline on any slot input change (event delegation)
document.getElementById('slots').addEventListener('change',function(){renderTimeline()});
document.getElementById('slots').addEventListener('input',function(){renderTimeline()});

// Load current schedule
fetch('/api/status').then(r=>r.json()).then(function(d){
  if(d.slotsDetail){slots=d.slotsDetail;}
  renderSlots();
}).catch(function(){renderSlots()});
</script>
</body></html>
)rawliteral";

// ─── Firmware Update Page ───────────────────────────────────────────────────
static const char UPDATE_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Update — Night Light</title>
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
  <div class="card">
    <h2>ArduinoOTA</h2>
    <p style="font-size:0.85em;color:#a0a0c0">
      You can also upload directly from the Arduino IDE over WiFi.
      Select <strong>nightlight</strong> as the network port under
      Tools &rarr; Port. The OTA password is your web UI password.</p>
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
  bar.style.display='block';
  fill.style.width='0%';
  btn.disabled=true;
  btn.textContent='Uploading...';
  status.textContent='';

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
      btn.disabled=false;
      btn.textContent='Upload & Flash';
    }
  };
  xhr.onerror=function(){
    status.textContent='Upload failed — connection lost';
    status.style.color='#e74c3c';
    btn.disabled=false;
    btn.textContent='Upload & Flash';
  };
  xhr.send(form);
}
fetch('/api/status').then(r=>r.json()).then(d=>{
  if(d.version)document.getElementById('curVer').textContent='v'+d.version;
}).catch(function(){});
</script>
</body></html>
)rawliteral";

// ─── Settings Page ─────────────────────────────────────────────────────────
static const char SETTINGS_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Settings — Night Light</title>
<style>%CSS%</style></head><body>
<div class="container">
  <p><a href="/">&larr; Back to dashboard</a></p>
  <h1>&#9881; Settings</h1>

  <!-- MQTT / Home Assistant -->
  <div class="card">
    <h2>MQTT / Home Assistant</h2>
    <div style="margin-bottom:8px">
      <span class="status-dot" id="mqttDot"></span>
      <span id="mqttStatus">Checking...</span>
    </div>
    <div style="margin-bottom:10px">
      <label class="toggle">
        <input type="checkbox" id="mqttToggle">
        <span class="slider"></span>
      </label>
      <span style="margin-left:8px">Enable MQTT</span>
    </div>
    <label>Broker host</label>
    <input type="text" id="mqttHost" placeholder="192.168.1.100">
    <label>Port</label>
    <input type="number" id="mqttPort" min="1" max="65535" value="1883">
    <label>Username (optional)</label>
    <input type="text" id="mqttUser" placeholder="Leave blank if none">
    <label>Password (optional)</label>
    <input type="password" id="mqttPass" placeholder="Leave blank if none">
    <button class="btn btn-primary btn-block" onclick="saveMqtt()">
      Save MQTT Settings</button>
  </div>

  <!-- Settings -->
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
    <label>Transition time (minutes)</label>
    <input type="number" id="transMin" min="0" max="30" value="5">
    <label>Change password</label>
    <input type="password" id="newPass" placeholder="New password">
    <button class="btn btn-primary btn-block" onclick="saveSettings()">
      Save Settings</button>
    <button class="btn btn-danger btn-block" onclick="if(confirm('Reset everything to defaults?'))fetch('/api/reset',{method:'POST'}).then(()=>location.reload())">
      Factory Reset</button>
  </div>

  <div class="footer">
    <a href="/update">Firmware Update</a> &middot;
    <a href="/logout">Log out</a> &middot;
    <span id="ipAddr"></span><br>
    <span id="fwVersion"></span> &middot;
    <span id="uptime"></span> &middot;
    <span id="freeHeap"></span> &middot;
    <span id="chipTemp"></span>
  </div>
</div>

<div class="toast" id="toast"></div>

<script>
var mqttLoaded=false;
function toast(msg,err){
  var t=document.getElementById('toast');
  t.textContent=msg;
  t.className=err?'toast error show':'toast show';
  setTimeout(function(){t.className='toast'},2500);
}
function fetchStatus(){
  fetch('/api/status').then(r=>r.json()).then(d=>{
    // Populate timezone
    if(d.timezone){
      var sel=document.getElementById('tzSelect');
      var found=false;
      for(var i=0;i<sel.options.length;i++){
        if(sel.options[i].value===d.timezone){sel.selectedIndex=i;found=true;break;}
      }
      if(!found)document.getElementById('tzCustom').value=d.timezone;
    }
    document.getElementById('transMin').value=d.transition||5;
    // MQTT status indicator
    var md=document.getElementById('mqttDot');
    var ms=document.getElementById('mqttStatus');
    if(d.mqttEnabled){
      md.className='status-dot '+(d.mqttConnected?'dot-green':'dot-red');
      ms.textContent=d.mqttConnected?'Connected to '+d.mqttHost:'Disconnected';
    }else{
      md.className='status-dot dot-amber';
      ms.textContent='Disabled';
    }
    // Populate MQTT form only on first load
    if(!mqttLoaded){
      mqttLoaded=true;
      document.getElementById('mqttToggle').checked=d.mqttEnabled;
      document.getElementById('mqttHost').value=d.mqttHost||'';
      document.getElementById('mqttPort').value=d.mqttPort||1883;
      if(d.mqttUsername)document.getElementById('mqttUser').value=d.mqttUsername;
    }
    // Footer
    document.getElementById('ipAddr').textContent=d.ip||'';
    if(d.version)document.getElementById('fwVersion').textContent='v'+d.version;
    if(d.uptime)document.getElementById('uptime').textContent='Up: '+d.uptime;
    if(d.freeHeap)document.getElementById('freeHeap').textContent=Math.round(d.freeHeap/1024)+'KB free';
    if(d.chipTemp!==undefined){
      var te=document.getElementById('chipTemp');
      te.textContent=d.chipTemp.toFixed(1)+'\u00B0C';
      te.style.color=d.thermalThrottle?'#e74c3c':(d.chipTemp>55?'#f39c12':'#a0a0c0');
    }
  }).catch(function(){});
}
function saveSettings(){
  var custom=document.getElementById('tzCustom').value;
  var tz=custom||document.getElementById('tzSelect').value;
  var trans=document.getElementById('transMin').value;
  var pass=document.getElementById('newPass').value;
  var body='timezone='+encodeURIComponent(tz)+'&transition='+trans;
  if(pass)body+='&password='+encodeURIComponent(pass);
  fetch('/api/settings',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:body}).then(function(){toast('Settings saved');fetchStatus()})
    .catch(function(){toast('Save failed',true)});
}
function saveMqtt(){
  var data={
    enabled:document.getElementById('mqttToggle').checked,
    host:document.getElementById('mqttHost').value,
    port:parseInt(document.getElementById('mqttPort').value)||1883,
    username:document.getElementById('mqttUser').value,
    password:document.getElementById('mqttPass').value
  };
  fetch('/api/mqtt',{method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify(data)
  }).then(function(r){
    if(r.ok){toast('MQTT settings saved');mqttLoaded=false;setTimeout(fetchStatus,2000);}
    else{toast('Save failed',true);}
  }).catch(function(){toast('Save failed',true)});
}
fetchStatus();
setInterval(fetchStatus,5000);
</script>
</body></html>
)rawliteral";

// ─── AP Mode Controls Page ─────────────────────────────────────────────────
static const char AP_CONTROLS_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Night Light Controls</title>
<style>%CSS%</style></head><body>
<div class="container">
  <h1>&#127769; Light Controls</h1>
  <p style="font-size:0.85em;color:#a0a0c0;margin-bottom:14px">
    Configure WiFi at <a href="http://192.168.4.1">192.168.4.1</a> to unlock
    scheduling and all features.</p>

  <div class="card">
    <h2>Colour</h2>
    <div class="row">
      <div><label>Pick colour</label>
        <input type="color" id="colourPick" value="#ff8c14"
               onchange="userTouched();sendColour(this.value)"></div>
      <div><label>Warm White (<span id="wVal">0</span>)</label>
        <input type="range" id="wSlider" min="0" max="255" value="0"
               oninput="userTouched();document.getElementById('wVal').textContent=this.value;sendWhite(this.value)"></div>
    </div>
  </div>

  <div class="card">
    <h2>Brightness (<span id="briPct">50</span>%)</h2>
    <input type="range" id="briSlider" min="0" max="255" value="128"
           oninput="userTouched();sendBrightness(this.value);document.getElementById('briPct').textContent=Math.round(this.value/2.55)">
  </div>

  <div class="card">
    <h2>Effect</h2>
    <select id="effectSel" onchange="userTouched();sendEffect(this.value)">
      <option value="0">Solid</option>
      <option value="1">Breathing</option>
      <option value="2">Soft Glow</option>
      <option value="3">Starry Twinkle</option>
      <option value="4">Rainbow</option>
      <option value="5">Candle Flicker</option>
      <option value="6">Sunrise</option>
      <option value="7">Christmas</option>
      <option value="8">Halloween</option>
      <option value="9">Birthday</option>
    </select>
  </div>

  <div class="footer">
    <a href="http://192.168.4.1">WiFi Setup</a>
  </div>
</div>

<div class="toast" id="toast"></div>

<script>
var lastUserInput=0;
function userTouched(){lastUserInput=Date.now();}
function recentlyTouched(){return Date.now()-lastUserInput<3000;}
function toast(msg,err){
  var t=document.getElementById('toast');
  t.textContent=msg;
  t.className=err?'toast error show':'toast show';
  setTimeout(function(){t.className='toast'},2500);
}
function sendBrightness(v){
  fetch('/api/brightness',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'brightness='+v});
}
function sendColour(hex){
  var r=parseInt(hex.substr(1,2),16);
  var g=parseInt(hex.substr(3,2),16);
  var b=parseInt(hex.substr(5,2),16);
  fetch('/api/colour',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'r='+r+'&g='+g+'&b='+b});
}
function sendWhite(v){
  fetch('/api/colour',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'w='+v});
}
function sendEffect(v){
  fetch('/api/effect',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'effect='+v}).then(function(){toast('Effect set')});
}
function fetchStatus(){
  fetch('/api/status').then(r=>r.json()).then(d=>{
    if(!recentlyTouched()){
      document.getElementById('briSlider').value=d.brightness||0;
      document.getElementById('briPct').textContent=Math.round((d.brightness||0)/2.55);
      if(d.colour)document.getElementById('colourPick').value=d.colour;
      document.getElementById('wSlider').value=d.white||0;
      document.getElementById('wVal').textContent=d.white||0;
      document.getElementById('effectSel').value=d.effect||0;
    }
  }).catch(function(){});
}
fetchStatus();
setInterval(fetchStatus,3000);
</script>
</body></html>
)rawliteral";

#endif // HTML_PAGES_H
