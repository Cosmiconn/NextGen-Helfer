/* Isya Gold 3.1 – user-chosen crafting needs & gather highlights.
   Local-only; no game interaction. Matches OCR to an opt-in shopping/gather list. */
'use strict';
const WATCH_KEY='isya-gold-needs-v3-1';
const defaultWatch={plans:[],manual:[],limits:{},overlay:false,mode:'shop'};
let watch={...defaultWatch};
try{
  const obj=JSON.parse(localStorage.getItem(WATCH_KEY)||'{}');
  if(obj&&typeof obj==='object')watch={...watch,...obj};
  if(!Array.isArray(watch.plans))watch.plans=[];
  if(!Array.isArray(watch.manual))watch.manual=[];
  if(!watch.limits||typeof watch.limits!=='object'||Array.isArray(watch.limits))watch.limits={};
}catch(_){watch={...defaultWatch};}
function watchSave(){localStorage.setItem(WATCH_KEY,JSON.stringify(watch));}
function watchNormalize(str){return String(str||'').normalize('NFKD').replace(/[\u0300-\u036f]/g,'').toLowerCase().replace(/[^a-z0-9]+/g,' ').trim();}
function watchRecipes(){
  const all=[...state.recipes.filter(r=>r.ingredients?.length).map(r=>({id:'u:'+r.id,name:r.name,ingredients:r.ingredients.map(i=>({name:itemName(i.itemId),qty:i.qty})),outputQty:r.outputQty||1})),
             ...IC.recipes.filter(r=>r.ingredients?.length).map(r=>({id:'c:'+r.id,name:r.name,ingredients:r.ingredients,outputQty:r.outputQty||1}))];
  return all.filter(r=>r.ingredients.every(i=>i.name&&i.name!=='(gelöschter Gegenstand)'&&Number.isFinite(Number(i.qty))));
}
function watchMaterials(){
  const list=new Map();
  const source=(name,qty,why)=>{const key=watchNormalize(name);if(!key||!(qty>0))return;
    let entry=list.get(key)||{key,name,wanted:0,owned:0,source:[]};
    entry.wanted+=qty;entry.source.push(why);list.set(key,entry);
  };
  const dict=new Map(watchRecipes().map(r=>[r.id,r]));
  for(const p of watch.plans){const rec=dict.get(p.id),runs=Number(p.runs);if(!rec||!Number.isSafeInteger(runs)||runs<1||runs>1000000)continue;
    for(const i of rec.ingredients)source(i.name,Number(i.qty)*runs,rec.name);
  }
  for(const m of watch.manual){const qty=Number(m.qty);if(Number.isSafeInteger(qty)&&qty>0&&qty<=100000000)source(m.name,qty,'Manuelles Ziel');}
  const own=new Map();for(const item of state.items){const k=watchNormalize(item.name);own.set(k,(own.get(k)||0)+(Number(item.owned)||0));}
  for(const [key,item] of list){item.owned=Math.max(0,Math.floor(own.get(key)||0));item.need=Math.max(0,item.wanted-item.owned);item.maxUnit=(Number.isSafeInteger(watch.limits[key])&&watch.limits[key]>0)?watch.limits[key]:null;
    item.places=[];for(const res of IC.resources){if(watchNormalize(res.name)===key)item.places.push(...(res.places||[]));}
  }
  return [...list.values()].sort((a,b)=>b.need-a.need||a.name.localeCompare(b.name,'de'));
}
function watchTargets(){return watchMaterials().filter(i=>i.need>0).slice(0,600).map(x=>({name:x.name,need:x.need,maxUnit:x.maxUnit,source:x.source.join('; ').slice(0,150)}));}
function watchMatchName(name){return watchMaterials().find(x=>x.need>0&&x.key===watchNormalize(name));}
function watchHtml(){const active=watchMaterials(),recs=watchRecipes(),opts=recs.map(r=>`<option value="${h(r.id)}">${h(r.name)}${r.id.startsWith('c:')?' · Katalog':' · Eigenes Rezept'}</option>`).join('');
return `<section class="panel watch-panel"><div class="panel-head"><div><h2>Benötigte Items und Sammelrohstoffe <span class="pill">v3.1</span></h2><p class="info">Fehlmengen aus ausgewählten Rezepten und manuell festgelegten Zielen. Grüne/gelbe/rote Hinweise erscheinen bei erkannten Mouseover-Tooltips. Keine automatischen Spielaktionen.</p></div><span class="tag yes">${watchTargets().length} aktive Suchbegriffe</span></div>
 <div class="twocol"><div><h3>Crafting-Ziele</h3><div class="actions"><label class="field" style="flex:1;min-width:200px">Rezept<select id="watch-rec">${opts}</select></label><label class="field" style="width:90px">Durchläufe<input id="watch-runs" type="number" min="1" max="1000000" value="10"></label><button class="btn primary" data-watch="add-recipe">+ Hinzufügen</button></div>
 ${watch.plans.length?`<div class="watch-mini-list">${watch.plans.map((p,i)=>{let r=recs.find(x=>x.id===p.id);return `<div class="watch-mini-row"><span>${h(r?.name||'Entferntes Rezept')} · ${Number(p.runs)||1}×</span><button class="btn small danger" data-watch="remove-recipe" data-index="${i}">Entfernen</button></div>`}).join('')}</div>`:'<p class="info">Noch keine aktiven Crafting-Pläne. Ein belegtes Rezept auswählen; Mengen aus alten Wikis unbedingt gegen das Spiel prüfen.</p>'}
 </div><div><h3>Eigene Suchliste</h3><div class="actions"><label class="field" style="flex:1;min-width:165px">Itemname<input id="watch-name" maxlength="120" placeholder="z. B. Silbererz"></label><label class="field" style="width:90px">Menge<input id="watch-qty" type="number" min="1" max="100000000" value="10"></label><button class="btn primary" data-watch="add-material">+ Hinzufügen</button></div>
 ${watch.manual.length?`<div class="watch-mini-list">${watch.manual.map((x,i)=>`<div class="watch-mini-row"><span>${h(x.name)} · ${Number(x.qty)}×</span><button class="btn small danger" data-watch="remove-material" data-index="${i}">Entfernen</button></div>`).join('')}</div>`:'<p class="info">Hier kannst du auch Ausrüstung, Questitems oder Rohstoffe auf die Suchliste setzen.</p>'}</div></div>
 <div class="divider"></div><div class="panel-head"><h3>Fehlende Materialien & Suchziele</h3><div class="actions"><button class="btn" data-watch="sync" ${live.server?'':'disabled'}>Liste an Live-Scanner übertragen</button><span class="info" id="watch-sync-state">${live.server?'Noch nicht synchronisiert':'Ohne Live-Scanner lokal sichtbar'}</span></div></div>
 <div class="table-wrap">${active.length?`<table class="tbl watch-target-table"><thead><tr><th>Item</th><th>Soll</th><th>Bestand</th><th>Fehlt</th><th>Höchster Stückpreis (optional)</th><th>Bekannte Fundorte</th></tr></thead><tbody>${active.map(x=>`<tr class="${x.need?'watch-row-needed':'watch-row-done'}"><td><b>${h(x.name)}</b><small class="muted" style="display:block">${h(x.source.join(' · '))}</small></td><td>${x.wanted}</td><td>${x.owned}</td><td><strong class="${x.need?'goldtext':'green'}">${x.need||'✓ 0'}</strong></td><td><input class="watch-limit" data-name="${h(x.name)}" placeholder="z. B. 1g 500s" value="${h(x.maxUnit===null?'':moneyText(x.maxUnit))}"></td><td class="info">${x.places.length?h([...new Set(x.places)].join(' · ')):'Im Quellenkatalog nicht eindeutig erfasst'}</td></tr>`).join('')}</tbody></table>`:'<div class="empty">Wähle ein Crafting-Rezept oder füge Rohstoffe hinzu. Die Fehlmengen erscheinen hier.</div>'}</div>
 <div class="divider"></div><div class="actions"><label class="field">Erkennungsmodus<select id="watch-mode"><option value="shop" ${watch.mode==='shop'?'selected':''}>Shop: Tooltip + Preis</option><option value="gather" ${watch.mode==='gather'?'selected':''}>Sammeln: Tooltip ohne Preis</option></select></label><label class="watch-overlay-label"><input id="watch-overlay" type="checkbox" ${watch.overlay?'checked':''} ${live.server?'':'disabled'}> Farbiges, klickdurchlässiges Hinweisfenster über dem Spiel (Windows)</label></div>
 <div class="hint gold">Der Hinweis erscheint <b>nur, wenn du selbst über das Item fährst und der Name lesbar ist</b>. Die Software markiert keine unsichtbaren Ressourcen und findet keine Objekte automatisch in der 3D-Welt. Für die Anzeige über Fiesta wird Fenster-/randloser Modus benötigt; Vollbild kann Overlays unterdrücken. OCR ist fehleranfällig – Käufe immer selbst prüfen.</div>
 <div id="watch-live-indicator" class="watch-indicator muted">Kein Bedarfstreffer im aktuellen Mouseover.</div>
 </section>`;
}
const WATCH_ORIGINAL_HTML = liveHtml;
liveHtml = function(){return WATCH_ORIGINAL_HTML()+watchHtml();};
const WATCH_ORIGINAL_STATUS = showLiveStatus;
showLiveStatus = function(data){
  WATCH_ORIGINAL_STATUS(data);
  if(page!=='shops')return;
  const m=data.preview?.match||{status:'none'}, box=document.getElementById('watch-live-indicator');
  if(box){const label={needed:'BENÖTIGT',affordable:'IM PREISLIMIT',expensive:'ZU TEUER',review:'OCR-NAME PRÜFEN',none:'Kein gesuchtes Item erkannt'};
    box.className='watch-indicator watch-'+m.status;
    box.innerHTML=m.status==='none'?h(label.none):`<strong>${h(label[m.status])}: ${h(m.name||'')}</strong> · ${Number(m.need||0)} benötigt${m.unit!=null?` · OCR-Stückpreis: ${h(moneyText(m.unit))}`:''}<div class="info">${h(m.message||'')}${m.qtyAssumed?' · Menge nur geschätzt!':''}</div>`;
  }
  const sync=document.getElementById('watch-sync-state');if(sync)sync.textContent=`${Number(data.targetsCount)||0} Suchziele im Scanner`;
  if(data.config){const mode=document.getElementById('watch-mode');if(mode&&document.activeElement!==mode)mode.value=data.config.scanMode||'shop';}
  document.querySelectorAll('tr[data-live-id]').forEach(tr=>{
    const name=tr.querySelector('.live-item-name')?.value||'';
    const match=watchMatchName(name);
    tr.classList.toggle('watch-row-needed',!!match);
    const old=tr.querySelector('.watch-row-tag');if(old)old.remove();
    if(match){const q=Number(tr.querySelector('.live-item-qty')?.value)||1,total=moneyParse(tr.querySelector('.live-item-total')?.value),unit=total===null?null:total/q;
      const costly=match.maxUnit!==null&&unit!==null&&unit>match.maxUnit;
      const tag=document.createElement('small');tag.className='watch-row-tag '+(costly?'watch-expensive':'watch-affordable');
      tag.textContent=costly?'Zu teuer laut OCR':'Auf Einkaufsliste';tr.querySelector('td:nth-child(2)')?.appendChild(tag);
    }
  });
};
async function watchSync(){if(!live.server)return;const entries=watchTargets();const result=await liveRequest('targets',{targets:entries});
  return result;
}
async function watchSaveBackendConfig(){if(!live.server)return;return await liveRequest('config',{scanMode:watch.mode,overlay:watch.overlay});}
async function watchChanged(){watchSave();render();if(live.server){try{await watchSync();await watchSaveBackendConfig();await livePoll();}catch(e){liveError(e);}}}
window.addEventListener('isya-watch-import',()=>{
  try{let next=JSON.parse(localStorage.getItem(WATCH_KEY)||'{}');watch={...defaultWatch,...next};}catch(_){watch={...defaultWatch};}
});
document.addEventListener('isya-gold-reset',()=>{
  watch={...defaultWatch,plans:[],manual:[],limits:{}};watchSave();
  if(live.server)watchSync().catch(e=>liveError(e));
});
// Shared handler: delegated controls appear only on the Shop Analysis screen.
document.addEventListener('click',async e=>{
  const btn=e.target.closest('button[data-watch]');if(!btn)return;
  try{
    btn.disabled=true;
    const action=btn.dataset.watch;
    if(action==='add-recipe'){
      const id=document.getElementById('watch-rec')?.value,runs=Number(document.getElementById('watch-runs')?.value);
      if(!id||!Number.isSafeInteger(runs)||runs<1||runs>1000000)throw Error('Bitte gültiges Rezept und Durchläufe eingeben.');
      const prev=watch.plans.find(x=>x.id===id);if(prev)prev.runs=runs;else watch.plans.push({id,runs});
      await watchChanged();
    }else if(action==='remove-recipe'){
      watch.plans.splice(Number(btn.dataset.index),1);await watchChanged();
    }else if(action==='add-material'){
      const name=document.getElementById('watch-name')?.value.trim(),qty=Number(document.getElementById('watch-qty')?.value);
      if(!name||name.length>120||!Number.isSafeInteger(qty)||qty<1||qty>100000000)throw Error('Itemname und Menge prüfen.');
      const prev=watch.manual.find(x=>watchNormalize(x.name)===watchNormalize(name));if(prev)prev.qty=qty;else watch.manual.push({name,qty});
      await watchChanged();
    }else if(action==='remove-material'){
      watch.manual.splice(Number(btn.dataset.index),1);await watchChanged();
    }else if(action==='sync'){await watchSync();toast('Bedarfsliste an Scanner übertragen.');}
  }catch(err){alert('Suchliste: '+err.message);}finally{btn.disabled=false;}
});
document.addEventListener('change',async e=>{
  const el=e.target;
  try{
    if(el.id==='watch-mode'){watch.mode=el.value;await watchChanged();}
    if(el.id==='watch-overlay'){watch.overlay=el.checked;await watchChanged();}
    if(el.classList?.contains('watch-limit')){
      const key=watchNormalize(el.dataset.name),text=el.value.trim(),limit=text?moneyParse(text):null;
      if(text&&(!Number.isSafeInteger(limit)||limit<1))throw Error('Bitte gültigen Höchstpreis je Stück angeben, z. B. 1g 250s.');
      if(limit===null)delete watch.limits[key];else watch.limits[key]=limit;
      await watchChanged();
    }
  }catch(err){alert('Suchliste: '+err.message);}
});
