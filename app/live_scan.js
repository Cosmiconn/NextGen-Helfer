/* Isya Gold 4.1 — local, read-only screen OCR companion. Native C++20 backend.
   Does not capture mouse events from Fiesta, automate actions, or access game data. */
'use strict';
const ISYA_LIVE_HOST = location.protocol==='http:' && location.hostname==='127.0.0.1' && location.port==='8765';
const live = {
  server: ISYA_LIVE_HOST, status: null, image: null, screenshot: null,
  mode:'tooltip', scanMode:'shop', drag:null, selection:null, lastError:'',
  handled:new Set(JSON.parse(localStorage.getItem('isya-gold-live-reviewed-v3')||'[]')),
  pollBusy:false
};
function liveHtml(){return `<section class="panel" id="live-scanner-panel">
  <div class="panel-head"><div><h2>Shop- & Welt-Scanner <span class="pill">v4.1 C++</span></h2><p class="info">Du führst die Maus über Gegenstände. Isya Gold liest ausschließlich den sichtbaren Bildschirm; vor dem Speichern bestätigst du die erkannten Angebote.</p></div><span class="tag ${live.server?'yes':'no'}" id="live-state">${live.server?'Verbinde…':'Live-Modus nicht gestartet'}</span></div>
  ${!live.server?`<div class="hint gold"><b>Live-Scanner öffnen:</b> Starte <code>STARTEN_CPP_SCANNER.bat</code> aus dem Programmordner. Die normale <code>STARTEN.bat</code> öffnet nur die bisherige Offline-Oberfläche. Daten aus v2 zuvor per JSON-Backup sichern und unter v3 wieder importieren.</div>`:
  `<div class="hint green">Nur Bildschirmaufnahme und lokale Texterkennung (OCR). <b>Kein</b> Klicken, Kaufen, Bewegen, Auslesen von Spieldaten oder Eingriff in Fiesta. Die Nutzung liegt in deiner Verantwortung; eine ausdrückliche Freigabe von gamigo liegt nicht vor.</div>
  <div class="twocol live-columns"><div>
   <h3>1 · Bildschirmbereiche einrichten</h3>
   <div class="form-grid"><label class="field">Shop-Notiz<input id="live-shop-name" maxlength="80" value="Live-Shop" placeholder="Beispiel: Elderine Shopsuche"></label><label class="field">Menge, falls nicht separat lesbar<input id="live-default-qty" type="number" min="1" max="100000000" value="1"></label></div>
   <p class="info">Halte in Fiesta den Mauszeiger auf einem Shopartikel, so dass der <b>Tooltip sichtbar</b> ist. Klicke auf „Screenshot in 5 Sekunden“, wechsle sofort zurück ins Spiel und lasse die Maus dort stehen. Nach der Aufnahme ziehst du zuerst ein Rechteck um den <b>Item-Tooltip</b>, dann eines um die <b>Preiszeile</b> des gleichen Artikels (optional zusätzlich die Mengenangabe).</p>
   <div class="actions"><button type="button" class="btn" data-live="snapshot">Screenshot in 5 Sekunden</button><span class="info" id="live-capture-status"></span></div>
   <div class="live-selection-actions"><label><input type="radio" name="live-region" value="tooltip" checked> Tooltip / Itemname</label><label><input type="radio" name="live-region" value="price"> Preiszeile</label><label><input type="radio" name="live-region" value="quantity"> Mengenfeld (optional)</label><label><input type="radio" name="live-region" value="world"> Spielansicht / sichtbare Objektnamen</label></div>
   <canvas id="live-shot" class="live-shot" width="720" height="180" aria-label="Bildschirmfoto – Rechteck für OCR-Bereich aufziehen"></canvas>
   <div id="live-region-status" class="info">Noch kein Kalibrierungsbild. Tipp: Fenster- oder randloser Fenstermodus und stabile Spielauflösung verbessern die Erkennung.</div>
  </div><div>
   <h3>2 · Live erfassen</h3><label class="field">Scanmodus <select id="live-scan-mode"><option value="shop">Shoppreise beim Mouseover</option><option value="gather">Sammel-Tooltip beim Mouseover</option><option value="world">Spielansicht: sichtbare Drops und Rohstoffe markieren</option></select></label>
   <p class="info">Im Modus Spielansicht zuerst die Screenshot-Aufnahme nutzen und mit "Spielansicht" den gewünschten festen Bildschirmbereich markieren. Nur sichtbare, lesbare Gegenstandsbeschriftungen werden umrahmt. Kein Erkennen versteckter Gegenstände oder Positionen.</p><p class="info">Im Shop-Modus müssen Tooltip und Preis markiert sein, im Sammelmodus genügt der Tooltip. Dann Scannen starten. Jetzt einfach selbst von Item zu Item fahren und kurz auf jedem verweilen (ca. 1–3 Sekunden, abhängig von OCR-Leistung). Die Preiszone muss zum Mauszeiger ausgerichtet sein.</p>
   <div class="actions"><button class="btn primary" data-live="start">▶ Scan starten</button><button class="btn" data-live="stop">■ Stoppen</button><button class="btn" data-live="refresh">Aktualisieren</button></div>
   <div id="live-stats" class="info">Status wird geladen…</div><div id="live-world-hits" class="info">Noch keine sichtbaren Ziele erkannt.</div><div id="live-preview" class="live-ocr-preview">Noch keine OCR-Ergebnisse.</div>
   <p class="info">Wenn Preis oder Name falsch erkannt wird: Erkennungsbereiche neu markieren. OCR liest nur sichtbare Bildschirm-Pixel. Bei mehreren Monitoren ggf. das Spiel auf den Hauptmonitor legen.</p>
  </div></div>
  <div class="divider"></div><div class="panel-head"><h3>3 · Erkannte Angebote prüfen</h3><div class="actions"><button class="btn primary" data-live="commit">Ausgewählte in Preisverlauf übernehmen</button><button class="btn" data-live="clear">Scannerliste leeren</button></div></div>
  <p class="info">**Wichtig:** OCR-Zahlen und Gegenstandsnamen sind unverlässlich, bis du sie geprüft hast. Insbesondere „Gesamtpreis“ vs. „Stückpreis“ beachten. Die angezeigte Standardmenge 1 ist eine <b>Annahme</b>, falls kein Mengenfeld markiert wurde. Ohne Häkchen wird nichts übernommen.</p>
  <div id="live-pending" class="table-wrap"><div class="empty">Noch keine sicher genug erkannten Angebote. Bitte kalibrieren und starten.</div></div>`}
 </section>`}
const originalRenderShops = renderShops;
renderShops = function(){return liveHtml()+originalRenderShops();};

async function liveRequest(path, payload){
  const opts=payload===undefined?{}:{method:'POST', headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)};
  const response=await fetch('/api/'+path, {...opts, cache:'no-store'});
  const result=await response.json();
  if(!response.ok)throw Error(result.error||`HTTP ${response.status}`);
  return result;
}
function liveError(ex){live.lastError=String(ex.message||ex);let node=document.getElementById('live-stats');if(node)node.textContent='Fehler: '+live.lastError;}
function showLiveStatus(data){
  live.status=data;
  if(page!=='shops')return;
  const el=(id)=>document.getElementById(id);
  let tag=el('live-state');if(tag){tag.className='tag '+(data.running?'yes':'no');tag.textContent=data.running?'● Scannt live':'○ Gestoppt';}
  const stats=el('live-stats');if(stats)stats.innerHTML=`<div><b>${data.frames||0} Scans</b> · ${data.pending?.length||0} Kandidaten · OCR: ${h(data.ocrAvailable||'nicht installiert')}</div>${data.error?`<div class="red">${h(data.error)}</div>`:''}`;
  const hits=el('live-world-hits');if(hits){const found=(data.worldHits||[]);hits.innerHTML=found.length?`<div class="hint green"><b>${found.length} sichtbare Zielnamen erkannt:</b> ${found.slice(0,12).map(x=>h(x.name)).join(', ')}. Trefferrahmen werden nur bei eingeschaltetem Overlay angezeigt.</div>`:'<div class="info">Keine Zielnamen in der kalibrierten Spielansicht erkannt. OCR erkennt nur sichtbare Beschriftungen und Suchlisteneinträge.</div>';}
  const selectMode=el('live-scan-mode');if(selectMode){if(!live.modeInitialized){live.scanMode=data.config.scanMode||'shop';live.modeInitialized=true;}if(document.activeElement!==selectMode)selectMode.value=live.scanMode;}
  if(el('live-shop-name') && document.activeElement!==el('live-shop-name'))el('live-shop-name').value=data.config.shopName||'Live-Shop';
  if(el('live-default-qty') && document.activeElement!==el('live-default-qty'))el('live-default-qty').value=data.config.defaultQuantity||1;
  const p=data.preview||{},prev=el('live-preview');
  if(prev)prev.innerHTML=`<b>${h(p.name||'Kein Name erkannt')}</b>${p.price?` <span class="green">${h(moneyText(p.price))}</span>`:''}<div class="info">${h(p.message||'')} ${p.quantityAssumed?'· Menge angenommen: '+Number(p.quantity):''}</div><details><summary>OCR-Rohtext anzeigen</summary><div class="live-raw"><b>Tooltip</b><pre>${h(p.tooltipText||'')}</pre><b>Preis</b><pre>${h(p.priceText||'')}</pre><b>Menge</b><pre>${h(p.quantityText||'')}</pre></div></details>`;
  const region=el('live-region-status');
  if(region){const cfg=data.config||{};region.textContent='Bereiche: '+(['tooltip','price','quantity','worldRegion'].map(k=>`${k==='tooltip'?'Tooltip':k==='price'?'Preis':k==='worldRegion'?'Spielansicht':'Menge'} ${cfg[k]?'✓':'–'}`).join(' · '))+' · Auflösung/Positionswechsel erfordern Neukalibrierung.';}
  const display=el('live-pending');if(!display)return;
  const all=(data.pending||[]).filter(x=>!live.handled.has(x.id)).slice().reverse();
  const existing=display.querySelectorAll('tr[data-live-id]');
  if(existing.length===all.length && [...existing].every((node,index)=>node.dataset.liveId===all[index].id))return; // preserve user edits
  const edits = new Map([...existing].map(row=>[row.dataset.liveId,{
    checked:row.querySelector('.live-check')?.checked||false,
    name:row.querySelector('.live-item-name')?.value,
    price:row.querySelector('.live-item-total')?.value,
    quantity:row.querySelector('.live-item-qty')?.value,
    seller:row.querySelector('.live-item-shop')?.value
  }]));
  display.innerHTML=all.length?`<table class="tbl live-review-table"><thead><tr><th>✓</th><th>Artikel (editierbar)</th><th>Gesamtpreis (editierbar)</th><th>Menge (prüfen!)</th><th>Shop</th><th>OCR</th></tr></thead><tbody>${all.map(evt=>{const edited=edits.get(evt.id)||{};return `<tr data-live-id="${h(evt.id)}"><td><input type="checkbox" class="live-check" aria-label="Angebot bestätigen" ${edited.checked?'checked':''}></td><td><input class="live-item-name" value="${h(edited.name??evt.name??'')}" maxlength="140"></td><td><input class="live-item-total" value="${h(edited.price??moneyText(evt.total))}"></td><td><input class="live-item-qty" type="number" min="1" max="100000000" value="${h(edited.quantity??(Number(evt.qty)||1))}" title="${evt.quantityAssumed?'Menge nur angenommen!':'Menge per OCR ermittelt'}"></td><td><input class="live-item-shop" value="${h(edited.seller??evt.seller??'Live-Shop')}" maxlength="100"></td><td title="Tooltip: ${h((evt.tooltipText||'').slice(0,160))}; Preis: ${h((evt.priceText||'').slice(0,160))}"><span class="tag ${evt.quantityAssumed?'no':'yes'}">${evt.quantityAssumed?'Menge prüfen':'Menge gelesen'}</span></td></tr>`}).join('')}</tbody></table>`:'<div class="empty">Keine neuen Kandidaten. Mit der Maus über ein Item fahren und 3–5 Sekunden warten.</div>';
}
async function livePoll(){if(!live.server||live.pollBusy||page!=='shops')return;
  live.pollBusy=true;
  try{showLiveStatus(await liveRequest('status'));}catch(ex){liveError(ex)}finally{live.pollBusy=false;}
}
setInterval(livePoll,1500);
const ORIGINAL_RENDER = render;
render = function(){ORIGINAL_RENDER();if(page==='shops')livePoll();};

async function liveSnapshot(){
  await liveRequest('snapshot',{delay:5});
  let el=document.getElementById('live-capture-status');if(el)el.textContent='In 5 Sekunden wird der gesamte Bildschirm fotografiert. Bitte jetzt zu Fiesta wechseln!';
  let attempts=0;
  const timer=setInterval(async()=>{
    attempts++;
    if(page!=='shops'||attempts>=25){clearInterval(timer);return;}
    try{let res=await liveRequest('capture');if(res.image){clearInterval(timer);if(res.image.error)throw Error(res.image.error);live.screenshot=res.image;
      live.image=new Image();live.image.onload=drawLiveShot;live.image.src=res.image.base64;
      if(el)el.textContent='Screenshot erfasst. Jetzt Tooltip, Preiszeile und ggf. Menge als Rechteck markieren.';
    }}catch(ex){clearInterval(timer);liveError(ex)}
  },750);
}
function drawLiveShot(){
  const canvas=document.getElementById('live-shot'), img=live.image, shot=live.screenshot;
  if(!canvas||!img||!shot)return;
  canvas.width=shot.previewWidth;canvas.height=shot.previewHeight;
  const ctx=canvas.getContext('2d');ctx.drawImage(img,0,0,canvas.width,canvas.height);
  const scaleX=canvas.width/shot.width,scaleY=canvas.height/shot.height;
  const cx=(shot.cursor[0]-shot.origin[0])*scaleX,cy=(shot.cursor[1]-shot.origin[1])*scaleY;
  ctx.lineWidth=2;ctx.strokeStyle='#20e9bd';ctx.beginPath();ctx.moveTo(cx-8,cy);ctx.lineTo(cx+8,cy);ctx.moveTo(cx,cy-8);ctx.lineTo(cx,cy+8);ctx.stroke();
  const configs=live.status?.config||{};
  for(const [key,color] of [['tooltip','#f4c46b'],['price','#60d0ff'],['quantity','#dd9eff']]){
    let r=configs[key];if(!r)continue;
    ctx.lineWidth=3;ctx.strokeStyle=color;ctx.strokeRect(cx+r.dx*scaleX,cy+r.dy*scaleY,r.w*scaleX,r.h*scaleY);
  }
  let world=configs.worldRegion;if(world){ctx.lineWidth=3;ctx.strokeStyle='#69ff9d';ctx.strokeRect((world.x-shot.origin[0])*scaleX,(world.y-shot.origin[1])*scaleY,world.w*scaleX,world.h*scaleY);}
  if(live.selection){let s=live.selection;ctx.lineWidth=3;ctx.strokeStyle='#ffffff';ctx.strokeRect(s.x,s.y,s.w,s.h);}
}
function canvasPos(event){let can=event.currentTarget,rect=can.getBoundingClientRect();return{x:(event.clientX-rect.left)*can.width/rect.width,y:(event.clientY-rect.top)*can.height/rect.height}}
async function commitRegion(){if(!live.selection||!live.screenshot)return;
 const shot=live.screenshot, sel=live.selection;
 const factorX=shot.width/shot.previewWidth, factorY=shot.height/shot.previewHeight;
 const region=live.mode==='world'
 ? {x:Math.round(sel.x*factorX+shot.origin[0]),y:Math.round(sel.y*factorY+shot.origin[1]),w:Math.round(sel.w*factorX),h:Math.round(sel.h*factorY)}
 : {dx:Math.round(sel.x*factorX+shot.origin[0]-shot.cursor[0]),dy:Math.round(sel.y*factorY+shot.origin[1]-shot.cursor[1]),w:Math.round(sel.w*factorX),h:Math.round(sel.h*factorY)};
 if(region.w<12||region.h<12)return;
 try{const data=await liveRequest('config',{[live.mode==='world'?'worldRegion':live.mode]:region});live.selection=null;showLiveStatus(data);drawLiveShot();toast('Bereich '+live.mode+' gespeichert.');}
 catch(ex){liveError(ex)}
}
// Pointer events act ONLY on browser calibration canvas, not on Fiesta.
document.addEventListener('pointerdown',e=>{if(e.target.id!=='live-shot'||!live.screenshot)return;const p=canvasPos(e);live.drag=p;live.selection=null;e.target.setPointerCapture(e.pointerId)});
document.addEventListener('pointermove',e=>{if(e.target.id!=='live-shot'||!live.drag)return;const p=canvasPos(e);const x=Math.max(0,Math.min(live.drag.x,p.x)),y=Math.max(0,Math.min(live.drag.y,p.y));live.selection={x,y,w:Math.abs(p.x-live.drag.x),h:Math.abs(p.y-live.drag.y)};drawLiveShot()});
document.addEventListener('pointerup',e=>{if(e.target.id!=='live-shot'||!live.drag)return;live.drag=null;commitRegion();});
document.addEventListener('change',e=>{if(e.target?.name==='live-region')live.mode=e.target.value;if(e.target?.id==='live-scan-mode')live.scanMode=e.target.value;});

async function importReviewed(){
  const selected=[...document.querySelectorAll('.live-review-table tbody tr')].filter(tr=>tr.querySelector('.live-check')?.checked);
  if(!selected.length){toast('Bitte zuerst die kontrollierten Angebote mit Häkchen auswählen.');return;}
  const rows=[];
  for(const tr of selected){const name=tr.querySelector('.live-item-name').value.trim(),q=Number(tr.querySelector('.live-item-qty').value),total=moneyParse(tr.querySelector('.live-item-total').value),seller=tr.querySelector('.live-item-shop').value.trim();
    if(!name||name.length>140||!Number.isSafeInteger(q)||q<1||q>100000000||total===null||total<1||total/q<1){alert('Bitte Itemname, Gesamtpreis und Menge in den ausgewählten Zeilen korrigieren.');return;}
    if(!Number.isFinite(total)||!Number.isSafeInteger(total)){alert('Preis ist zu hoch oder ungültig.');return;}
    rows.push({id:tr.dataset.liveId,name,qty:q,total,unit:Math.round(total/q),seller:seller||'Live-Shop',time:new Date().toISOString()});
  }
  if(!confirm(`${rows.length} geprüfte Live-Angebote in den Preisverlauf übernehmen? Preise sind Shop-Angebote, keine abgeschlossenen Verkäufe.`))return;
  for(const row of rows){shopData.push({name:row.name,qty:row.qty,total:row.total,unit:row.unit,seller:row.seller,time:row.time});live.handled.add(row.id);}
  localStorage.setItem('isya-gold-live-reviewed-v3',JSON.stringify([...live.handled].slice(-5000)));
  saveShops();render();toast(rows.length+' geprüfte Angebote in den Preisverlauf übernommen.');
}
// Browser buttons only operate localhost scanner, not client UI.
document.addEventListener('click',async e=>{let btn=e.target.closest('button[data-live]');if(!btn||!live.server)return;
  try{btn.disabled=true;
    const action=btn.dataset.live;
    if(action==='snapshot')await liveSnapshot();
    else if(action==='start'){
      if(typeof watchSync==='function')await watchSync();
      await liveRequest('config',{shopName:document.getElementById('live-shop-name').value,defaultQuantity:Number(document.getElementById('live-default-qty').value),scanMode:document.getElementById('live-scan-mode')?.value||'shop',overlay:document.getElementById('live-scan-mode')?.value==='world'||(typeof watch!=='undefined'&&watch.overlay)});
      showLiveStatus(await liveRequest('start',{}));
    }
    else if(action==='stop')showLiveStatus(await liveRequest('stop',{}));
    else if(action==='refresh')await livePoll();
    else if(action==='clear') {if(confirm('Nur unbestätigte Live-Kandidaten im Scanner verwerfen? Gespeicherte Preisverläufe bleiben erhalten.')){await liveRequest('clear',{});live.handled.clear();livePoll();}}
    else if(action==='commit')await importReviewed();
  }catch(ex){liveError(ex);alert('Live-Scanner: '+ex.message)}finally{btn.disabled=false;}
});
if(page==='shops')livePoll();
