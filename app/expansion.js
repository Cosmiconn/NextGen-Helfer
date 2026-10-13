/* Isya Gold v2 – locally run economic companion.  No game memory, packets, input simulation or client control. */
'use strict';
const IC=ISYA_CATALOG;
const OLD_RENDER=render;
const shopKey='isya-gold-shop-v2';
let shopData=[];
let customRecipes=[];
try {const x=JSON.parse(localStorage.getItem('isya-gold-custom-recipes-v2')||'[]');if(Array.isArray(x))customRecipes=x;}catch(_){}
function applyCustomRecipes(){for(let r of customRecipes){if(!r||!r.name||!Array.isArray(r.ingredients)||!r.ingredients.length)continue;const found=IC.recipes.find(x=>x.name===r.name&&x.category===r.category&&x.tier===r.tier);if(found&&found.ingredients)continue;if(found)IC.recipes.splice(IC.recipes.indexOf(found),1);if(!IC.recipes.some(x=>x.id===r.id))IC.recipes.push(r)}}
applyCustomRecipes();
try { const raw=localStorage.getItem(shopKey);const parsed=raw?JSON.parse(raw):[];shopData=Array.isArray(parsed)?parsed:[]; }catch(e){shopData=[];}
let view={recipeQuery:'',recipeCategory:'alle',recipeTier:'alle',recipeCompleteness:'alle',resourceQuery:'',resourceCat:'alle',marketQuery:'',shopQuery:'',shopInput:'',shopImage:'',guideClass:'Priester',guideLevel:67,shopMode:'history'};
const byName=name=>state.items.find(x=>x.name.toLocaleLowerCase('de-DE')===String(name).toLocaleLowerCase('de-DE'));
const catStats=()=>({full:IC.recipes.filter(x=>x.ingredients).length,total:IC.recipes.length,missing:IC.recipes.filter(x=>!x.ingredients).length});
function catCounts(){const s=catStats();return `${s.full} berechenbare Rezepte · ${s.missing} unvollständige Einträge`}
function catRecipeCost(rec){if(!rec.ingredients)return null;let out=byName(rec.name),sell=(out&&isKnown(out.sell))?out.sell*n(rec.outputQty):null,materials=0,unknown=[],buy=0,unknownBuy=[];for(let ing of rec.ingredients){let item=byName(ing.name);if(item&&isKnown(item.sell))materials+=item.sell*ing.qty;else unknown.push(ing.name);if(item&&isKnown(item.buy))buy+=item.buy*ing.qty;else unknownBuy.push(ing.name)}return {out:sell,ingredientsSell:unknown.length?null:materials,ingredientsBuy:unknownBuy.length?null:buy,net:sell!==null&&!unknown.length?sell*(1-n(state.settings.fee)/100)-materials:null,missing:unknown,missingBuy:unknownBuy};}
function setContent(title,sub,html){pageHeader(title,sub);document.querySelector('#content').innerHTML=html;}
render=function(){
 if(page==='catalog')return setContent('Rezeptdatenbank',`${catCounts()} · dokumentierte Altquellen, keine Live-Daten.`,renderCatalog());
 if(page==='resources')return setContent('Ressourcen & NPCs','Fundgebiete, Materialtypen und Händler – mit Herkunftslinks und Prüfhinweisen.',renderResources());
 if(page==='shops')return setContent('Shop-Analysator','Preisbeobachtungen, Trends und Nutzen für Crafting – ohne Eingriffe in den Spielclient.',renderShops());
 if(page==='guides')return setContent('Klassen- & Levelguides','Sechs Klassen und Levelwege bis 145, mit Quellen und Unsicherheiten.',renderGuides());
 OLD_RENDER();
 if(page==='home'){
   const c=document.querySelector('#content');
   c.insertAdjacentHTML('afterbegin',`<div class="hint green"><b>Isya Gold 2.0:</b> ${catCounts()} im neuen <button class="btn small" data-page="catalog">Rezeptkatalog</button>. <button class="btn small" data-page="shops">Marktbeobachtungen & Trends</button> · <button class="btn small" data-page="resources">Sammelorte & Händler</button> · <button class="btn small" data-page="guides">Klassen-Levelguide</button></div>`);
 }
 if(page==='recipes')document.querySelector('#content .panel')?.insertAdjacentHTML('afterbegin',`<div class="hint green">Zusätzlich verfügbar: ${catCounts()} im <button class="btn small" data-page="catalog">Rezeptkatalog</button>. Importiere geprüfte Rezepte mit einem Klick in den bestehenden Gewinnrechner.</div>`);
 if(page==='market') {
  const t=document.querySelector('#content .panel');
  if(t){t.insertAdjacentHTML('afterbegin',`<label class="field" style="margin-bottom:12px">Gegenstände filtern<input type="search" id="market-search" value="${h(view.marketQuery)}" placeholder="Name / Kategorie"></label>`);
    const q=view.marketQuery.trim().toLocaleLowerCase('de-DE');if(q){t.querySelectorAll('table tbody tr').forEach(tr=>tr.hidden=!tr.textContent.toLocaleLowerCase('de-DE').includes(q));}
  }
 }
};
const badgeStat=(r)=>r.ingredients?`<span class="tag yes">${r.status==='Eigene Eingabe – ungeprüft'?'Eigene Eingabe – ungeprüft':'Mengen belegt (Wiki)'}</span>`:'<span class="tag no">Offen / nicht kalkulierbar</span>';
const sourceLink=(url,text='Quelle')=>url?`<a href="${h(url)}" target="_blank" rel="noopener noreferrer">${h(text)} ↗</a>`:'';
const recIngredients=r=>r.ingredients?r.ingredients.map(x=>`${x.qty}× ${h(x.name)}`).join(' · '):'Materialmengen noch nicht zuverlässig verfügbar.';
function renderCatalog(){
 const cats=['alle','Stein','Rolle','Trank','Materialaufwertung','Materialzerlegung'];
 const results=IC.recipes.filter(r=>
  (view.recipeCategory==='alle'||r.category===view.recipeCategory)&&
  (view.recipeTier==='alle'||r.tier==Number(view.recipeTier))&&
  (view.recipeCompleteness==='alle'||(view.recipeCompleteness==='belegt'?!!r.ingredients:!r.ingredients))&&
  (r.name+' '+recIngredients(r)+' '+r.category).toLocaleLowerCase('de-DE').includes(view.recipeQuery.toLocaleLowerCase('de-DE')));
 const s=catStats();
 return `<div class="cards" style="grid-template-columns:repeat(3,minmax(0,1fr))"><div class="card"><div class="k">Katalog-Einträge</div><div class="v">${s.total}</div><div class="m">Alle fünf Berufszweige, inkl. Platzhalter</div></div><div class="card"><div class="k">Mit Zutatenmengen</div><div class="v green">${s.full}</div><div class="m">Alte Community-Quellen</div></div><div class="card"><div class="k">Mengen fehlen</div><div class="v goldtext">${s.missing}</div><div class="m">Nie als profitabel bewerten</div></div></div>
 <section class="panel"><h2>Rezeptliste durchsuchen</h2><div class="form-grid" style="grid-template-columns:2fr 1fr 1fr 1fr"><label class="field">Suche<input id="cat-search" type="search" placeholder="z. B. Vitalität, Rubin, Lix" value="${h(view.recipeQuery)}"></label><label class="field">Beruf<select id="cat-type">${cats.map(x=>`<option value="${x}" ${x===view.recipeCategory?'selected':''}>${x==='alle'?'Alle':h(x)}</option>`).join('')}</select></label><label class="field">Tier<select id="cat-tier">${['alle',1,2,3,4,5,6,7,8].map(x=>`<option value="${x}" ${String(x)===String(view.recipeTier)?'selected':''}>${x==='alle'?'Alle':'T'+x}</option>`).join('')}</select></label><label class="field">Vollständigkeit<select id="cat-status">${[['alle','Alle'],['belegt','Mit Zutaten'],['offen','Nur offen']].map(([v,t])=>`<option value="${v}" ${v===view.recipeCompleteness?'selected':''}>${t}</option>`).join('')}</select></label></div>
 <div class="actions" style="margin:16px 0"><button class="btn primary" data-ext="import-visible-recipes" ${results.some(x=>x.ingredients)?'':'disabled'}>Alle angezeigten belegten Rezepte in Rechner übernehmen</button><button class="btn" data-ext="recipes-template">CSV-Rezeptvorlage herunterladen</button><button class="btn" data-ext="import-custom-csv">Eigene Rezept-CSV importieren</button><span class="muted">${results.length} Treffer</span></div>
 <div class="hint gold">„Alle Rezepte“ ist als offene, erweiterbare Datenbank angelegt, <b>nicht als garantiert vollständige aktuelle gamigo-Rezeptliste</b>. Fehlende Mengen werden nicht erfunden. Vorhandene Mengen stammen aus älteren Wiki-Tabellen und müssen im Spiel gegengeprüft werden. Materialaufwertung/-zerlegung enthalten derzeit keine gesicherten Mengenverhältnisse.</div>
 <div class="recipe-results">${results.length?results.map(r=>{let stat=catRecipeCost(r);return `<article class="mini recipe-card"><div class="panel-head"><div><strong>${h(r.name)}</strong><small>${h(r.category)} · Tier ${r.tier}${r.skillPoints!=null?' · Produktions-Skill '+Number(r.skillPoints).toLocaleString('de-DE'):''}</small></div>${badgeStat(r)}</div><p class="info">${recIngredients(r)}</p>${stat?`<p class="info">Mehrwert gegenüber Direktverkauf: <strong class="${stat.net>0?'green':stat.net<0?'red':''}">${moneyText(stat.net)}</strong>${stat.missing.length?' · Preis(e) fehlen':''}</p>`:''}<div class="actions">${r.ingredients?`<button class="btn small" data-ext="import-recipe" data-id="${h(r.id)}">In Crafting-Rechner</button>`:''}${sourceLink(r.source)}${r.note?`<small class="muted">${h(r.note)}</small>`:''}</div></article>`}).join(''):'<div class="empty">Keine passenden Rezepte. Filter ändern.</div>'}</div></section>`;
}
function importRecipe(rec,quiet=false){if(!rec||!rec.ingredients||!rec.ingredients.length)return false;
 function ensure(name,category='Material'){let current=byName(name);if(current)return current;let t={id:id(),name,category,owned:0,buy:null,sell:null,updatedAt:null};state.items.push(t);return t}
 let output=ensure(rec.name,rec.category), existing=state.recipes.find(r=>r.catalogId===rec.id||(r.name.toLocaleLowerCase('de-DE')===rec.name.toLocaleLowerCase('de-DE')&&r.outputId===output.id));
 if(existing){if(!existing.catalogId)existing.catalogId=rec.id;return false;}
 state.recipes.push({id:id(),name:rec.name,outputId:output.id,outputQty:rec.outputQty,ingredients:rec.ingredients.map(x=>({itemId:ensure(x.name).id,qty:x.qty})),extraCost:0,minutes:null,note:`Katalog · ${rec.category} T${rec.tier}. Alte Wiki-Daten! Rezept und Craftzeit im Client prüfen.`,catalogId:rec.id,source:rec.source});
 if(!quiet){save();page='recipes';render();toast('Rezept mit Zutaten übernommen. Jetzt Preise erfassen.')}return true;
}
function renderResources(){let q=view.resourceQuery.toLocaleLowerCase('de-DE');let cats=['alle',...new Set(IC.resources.map(r=>r.category))];let list=IC.resources.filter(r=>(view.resourceCat==='alle'||r.category===view.resourceCat)&&(r.name+' '+r.places.join(' ')+' '+r.usage).toLocaleLowerCase('de-DE').includes(q));return `
 <div class="panel"><h2>Ressourcen-Finder</h2><p class="muted">${IC.resources.length} Fundort-/Materialeinträge · historisches Wiki, keine garantierten Spawnkoordinaten. Für deinen Level-67-Priester sind besonders Uruga und Alte Elfenwälder interessant.</p><div class="form-grid"><label class="field">Material oder Ort suchen<input type="search" id="res-search" value="${h(view.resourceQuery)}" placeholder="z. B. Uruga, Rubin, Giftpilz"></label><label class="field">Gruppe<select id="res-type">${cats.map(v=>`<option value="${h(v)}" ${v===view.resourceCat?'selected':''}>${v==='alle'?'Alle Materialien':h(v)}</option>`).join('')}</select></label></div><div class="table-wrap"><table class="tbl" style="min-width:680px"><thead><tr><th>Material</th><th>Art</th><th>Wo holen?</th><th>Verwendung</th><th>Quelle</th></tr></thead><tbody>${list.map(r=>`<tr><td><b>${h(r.name)}</b></td><td><span class="tag">${h(r.category)}</span></td><td>${r.places.map(h).join(' · ')}</td><td>${h(r.usage)}</td><td>${sourceLink(r.source,'Wiki')}</td></tr>`).join('')}</tbody></table></div>${list.length?'':'<div class="empty">Keine Fundorte gefunden.</div>'}</div>
 <div class="panel"><h2>NPCs – Material und Rezeptbeschaffung</h2><div class="hint gold">Vellons Materialangebote sind aus einer älteren Wiki-Seite übernommen. **Keine hier gezeigten NPC-Preise gelten als aktuell**; bevor du bei Spielern einkaufst, vergleiche im offiziellen Client direkt beim NPC.</div><div class="grid2-col">${IC.npcs.map(npc=>`<div class="mini"><div class="panel-head"><strong>${h(npc.name)}</strong><span class="tag">NPC</span></div><small>${h(npc.location)}</small><p>${npc.items.map(h).join(' · ')}</p><small class="muted">${h(npc.note)}</small><p>${sourceLink(npc.source,'NPC-Quelle')}</p></div>`).join('')}</div></div>
 <div class="panel"><h2>Gezielt farmen statt wahllos sammeln</h2><p>Im <button class="btn small" data-page="catalog">Rezeptkatalog</button> findest du benötigte Materialien. Suche deren Namen hier oder prüfe Händlerangebote. Unter <button class="btn small" data-page="farm">Sammeln & Farmen</button> misst du den <b>tatsächlichen</b> Goldwert pro Stunde.</p></div>`}
function median(arr){if(!arr.length)return null;let v=[...arr].sort((a,b)=>a-b),m=v.length/2;return v.length%2?v[Math.floor(m)]:(v[m-1]+v[m])/2;}
function groupShops(){let obj=new Map();for(let row of shopData){const key=row.name.trim().toLocaleLowerCase('de-DE');if(!obj.has(key))obj.set(key,[]);obj.get(key).push(row)}return [...obj.entries()].map(([name,rows])=>{
 rows.sort((a,b)=>String(b.time).localeCompare(String(a.time)));
 let recent=rows.slice(0,Math.max(1,Math.ceil(rows.length/2))),previous=rows.slice(recent.length),prices=rows.map(x=>x.unit),cur=median(recent.map(x=>x.unit)),prev=median(previous.map(x=>x.unit));
 let sample=rows[0]; let recipe=IC.recipes.find(r=>r.name.toLocaleLowerCase('de-DE')===name);let ingredientMatches=IC.recipes.filter(r=>r.ingredients?.some(x=>x.name.toLocaleLowerCase('de-DE')===name));
 let localItem=byName(sample.name);return {name:sample.name,rows,cur,prev,min:Math.min(...prices),max:Math.max(...prices),low:prices[0],recipe,ingredientMatches,localItem,delta:prev>0?100*(cur-prev)/prev:null};
 }).sort((a,b)=>b.rows.length-a.rows.length);}
function shopAssessment(g){let info=[];if(g.ingredientMatches.length){info.push('Zutat in '+g.ingredientMatches.length+' Katalogrezept(en)');}
 if(g.recipe){info.push('Herstellbares Produkt · '+g.recipe.category+' T'+g.recipe.tier)}
 if(g.localItem?.owned)info.push(g.localItem.owned+' Stück im Lager');
 if(g.rows.length<3)info.push('Preistrend unsicher: nur '+g.rows.length+' Beobachtung(en)');
 if(!info.length)info.push('Keine zugeordnete Crafting-Nutzung; Itemname prüfen.');return info.join(' · ');}
function showHistory(){let gs=groupShops().filter(x=>x.name.toLocaleLowerCase('de-DE').includes(view.shopQuery.toLocaleLowerCase('de-DE')));return `<div class="table-wrap"><table class="tbl" style="min-width:840px"><thead><tr><th>Gegenstand / Nutzen</th><th>Beobachtungen</th><th>Median jüngste Hälfte</th><th>Günstigstes Angebot</th><th>Preisänderung*</th><th>Aktion</th></tr></thead><tbody>${gs.map(g=>`<tr><td><b>${h(g.name)}</b><div class="info">${h(shopAssessment(g))}</div></td><td>${g.rows.length}</td><td class="num goldtext">${moneyText(g.cur)}</td><td class="num green">${moneyText(g.min)}</td><td class="num ${g.delta>0?'red':g.delta<0?'green':''}">${g.delta===null?'Zu wenig Daten':`${g.delta>0?'+':''}${g.delta.toFixed(1)} %`}</td><td><button class="btn small" data-ext="shop-details" data-name="${h(g.name)}">Details</button></td></tr>`).join('')}</tbody></table></div>${gs.length?'':'<div class="empty">Noch keine passenden Marktbeobachtungen. Füge Shopzeilen hinzu.</div>'}<p class="info">* Median der zeitlich neueren Hälfte im Vergleich zur älteren Hälfte, <b>keine</b> offiziell gemessene Preisentwicklung. Bei weniger als zwei Beobachtungen nicht berechenbar. Angebote sind keine abgeschlossenen Verkäufe.</p>`}
function renderShops(){return `<div class="panel"><div class="panel-head"><h2>Shoplisten analysieren</h2><span class="pill">${shopData.length} Preisbeobachtungen</span></div><div class="hint gold"><b>Fair-Play:</b> Kein Steuern oder Verändern des laufenden Spiels. Im lokalen Live-Modus (v3) wird ausschließlich der sichtbare Bildschirm ausgelesen; alternativ kannst du Screenshots laden, vergrößern und erkannte Zeilen vor dem Import prüfen. Es werden keine Artikel gekauft, Shops geöffnet oder Mäuse im Spiel bewegt.</div>
 <div class="twocol"><div><h3>Screenshot aus Shopsuche</h3><label class="field">Bilddatei auswählen<input type="file" id="shop-image" accept="image/png,image/jpeg,image/webp"></label><div id="shot-box" class="shot-box"><div class="empty">Screenshot wählen. Danach mit der Maus darüberfahren, um Details vergrößert anzusehen.</div></div><div id="shop-mag" class="magnifier" hidden></div><div class="actions" style="margin-top:10px"><button class="btn" data-ext="shop-ocr">Text auf Screenshot erkennen (optional, online)</button></div><p class="info">Für OCR wird auf Knopfdruck die Bibliothek Tesseract.js von jsDelivr geladen. Text wird nach dem Laden lokal im Browser erkannt; bei fehlender Verbindung bleibt der manuelle Import verfügbar. Spielscreenshots können Informationen über deinen Charakter enthalten – lade nur gewünschte Ausschnitte hoch.</p></div><div><h3>Erkannte oder kopierte Shop-Liste</h3><p class="info">Pro Zeile: <code>Gegenstand;Menge;Gesamtpreis;Shopname</code>. Trenner auch Tab möglich. Beispiel: <code>Vitalität [Tier 3];5;2g 500s;Verkäufer A</code>. Einträge werden <b>vor der Übernahme</b> geprüft.</p><textarea id="shop-lines" rows="10" placeholder="Vitalität [Tier 3];5;2g 500s;Shop A\nRaues Leder;10;250s;Shop B">${h(view.shopInput)}</textarea><div class="actions" style="margin-top:10px"><button class="btn primary" data-ext="shop-preview">Zeilen prüfen</button><button class="btn" data-ext="shop-sample">Beispielformat einfügen</button><button class="btn" data-ext="shop-csv">CSV-Vorlage</button><button class="btn" data-ext="shop-suggest">Aus OCR-Text Spalten erkennen</button></div><div id="shop-preview"></div></div></div>
 </div><div class="panel"><div class="panel-head"><h2>Marktbeobachtungen und Preisentwicklung</h2><div class="actions"><button class="btn small" data-ext="shop-export">Preisverlauf CSV exportieren</button><button class="btn small danger" data-ext="shop-reset">Preisbeobachtungen löschen</button></div></div><label class="field">Beobachtungen filtern<input type="search" id="shop-search" value="${h(view.shopQuery)}" placeholder="Gegenstandsname"></label>${showHistory()}</div><div class="panel"><h2>Nutzen-Erkennung</h2><p>Die Analyse verknüpft Gegenstandsnamen mit den importierten Rezepten und erfasst, wie oft sie als Zutat benötigt werden. Sie zeigt außerdem Median, günstigstes Angebot, Trend und Lagerbestand. Bei niedrigen Stückzahlen oder wechselnden Serverpreisen sind die Ergebnisse nur Anhaltspunkte.</p></div>`}
const levelResults=level=>IC.levels.filter(l=>level>=l.min&&level<=l.max);
function renderGuides(){let c=IC.classes.find(x=>x.name===view.guideClass)||IC.classes[0],lv=Math.min(145,Math.max(1,Number(view.guideLevel)||67)),ranges=levelResults(lv);return `<section class="panel"><div class="panel-head"><h2>Klassen wählen</h2><span class="tag">6 Klassen laut gamigo</span></div><div class="class-tabs">${IC.classes.map(g=>`<button class="btn ${g.name===c.name?'primary':''}" data-ext="guide-class" data-class="${h(g.name)}">${h(g.name)}</button>`).join('')}</div><div class="mini" style="margin-top:14px"><h3>${h(c.name)} · ${h(c.role)}</h3><p><b>Spielweise:</b> ${h(c.focus)}</p><p><b>Freie Stats:</b> ${h(c.stats)}</p><p><b>Skill-Priorität:</b> ${h(c.skills)}</p><p><b>Ausrüstung:</b> ${h(c.gear)}</p><p><b>Levelstrategie:</b> ${h(c.strategy)}</p><p><b>In Gruppen:</b> ${h(c.group)}</p><p><b>Typische Fehler:</b> ${h(c.pitfalls)}</p><p>${sourceLink(c.source,'Community-Klassenguide')} · ${sourceLink('https://fiesta.gamigo.com/classes/','Offizielle Klassenübersicht')}</p></div><div class="hint gold">Die Klassen- und Statvorschläge sind praxisorientierte Orientierung, keine verifizierten exakten Talentbäume für die aktuelle Serverversion. Kreuzritter beginnt laut gamigo auf Level 60.</div></section>
 <section class="panel"><h2>Levelroute suchen</h2><div class="form-grid"><label class="field">Dein Level<input id="guide-lv" type="number" min="1" max="145" value="${lv}"></label><label class="field">Schnelle Stufenwahl<select id="guide-range"><option value="">– Level –</option>${[1,20,38,43,52,60,67,68,71,74,77,80,90,100,120,130,141].map(x=>`<option value="${x}" ${x===lv?'selected':''}>Ab Level ${x}</option>`).join('')}</select></label></div><div class="actions" style="margin:12px 0"><button class="btn primary" data-ext="guide-update">Levelroute anzeigen</button><button class="btn" data-ext="guide-all">Alle Levelbereiche anzeigen</button></div><div id="guide-list">${guideRangeCards(ranges)}</div></section>`}
function guideRangeCards(arr){return arr.length?arr.map(l=>`<div class="mini" style="margin-bottom:11px"><div class="panel-head"><strong>Level ${l.min}–${l.max}: ${h(l.title)}</strong><span class="tag">${h(l.quest)}</span></div><p>${h(l.route)}</p>${sourceLink(l.source,'Levelguide-Quelle')}</div>`).join(''):'<div class="empty">Für diesen Levelbereich ist noch keine Route hinterlegt.</div>'}
function guessOCRLines(text){
 let structured=[];
 const priceToken=String.raw`(?:\d+(?:[.,]\d+)?\s*(?:gem|gold|silber|kupfer|g|s|k)\s*)+`;
 const spaced=new RegExp('^(.+?)\\s{2,}[x×]?\\s*(\\d{1,8})\\s{2,}('+priceToken+')(?=\\s{2,}|$)(?:\\s{2,}(.+))?$','i');
 // Double-spaces are reliable text-column delimiters. Single spaces are ambiguous and are not guessed automatically.
 for(let line of String(text).split(/\r?\n/)){
  let clean=line.trim();if(!clean||/^(shop|artikel|name|preis|gesamt|menge|suche)/i.test(clean))continue;
  if(clean.includes(';')||clean.includes('\t')){structured.push(clean);continue;}
  let m=clean.match(spaced);
  if(m&&moneyParse(m[3])!==null){structured.push([m[1].trim(),m[2],m[3].replace(/\s+/g,' ').trim(),m[4]?.trim()||'OCR-Shop'].join(';'));}
 }
 return {structured};
}
function parseShopRows(str){let accepted=[],issues=[];for(let [index,line] of str.split(/\r?\n/).entries()){
 line=line.trim();if(!line||/^\s*(Artikel|Gegenstand|Name)\s*[;,\t]/i.test(line))continue;
 let fields=line.includes('\t')?line.split('\t'):line.split(';');if(fields.length<3){issues.push(`Zeile ${index+1}: Mindestens drei Felder durch Semikolon oder Tab erwartet.`);continue;}
 let [name,qtyRaw,priceRaw,sellerRaw]=fields.map(s=>s.trim()),qty=Number(qtyRaw.replace(',','.')),price=moneyParse(priceRaw);
 if(!name||name.length>140||!Number.isInteger(qty)||qty<1||qty>100000000||price===null||price<=0){issues.push(`Zeile ${index+1}: Name, Menge oder Gesamtpreis ungültig.`);continue;}
 const unit=Math.round(price/qty);if(unit<1){issues.push(`Zeile ${index+1}: Preis je Stück unter 1 Kupfer.`);continue;}
 accepted.push({name,qty,total:price,unit,seller:(sellerRaw||'Unbekannt').slice(0,100),time:new Date().toISOString()});
 }return {accepted,issues};}
function previewShop(){let value=document.getElementById('shop-lines')?.value||'',p=parseShopRows(value);view.shopInput=value;window.isyaPreview=p;let area=document.querySelector('#shop-preview');if(!area)return;
 area.innerHTML=`<div class="hint ${p.issues.length?'warn':'green'}">${p.accepted.length} gültige Zeile(n) · ${p.issues.length} Fehler. <b>Gesamtpreis wird durch Menge geteilt!</b></div>${p.issues.length?`<p class="red">${p.issues.map(h).join('<br>')}</p>`:''}<div class="table-wrap"><table class="tbl"><thead><tr><th>Item</th><th>Menge</th><th>Gesamtpreis</th><th>Preis/Stück</th></tr></thead><tbody>${p.accepted.map(x=>`<tr><td>${h(x.name)}</td><td>${x.qty}</td><td>${moneyText(x.total)}</td><td>${moneyText(x.unit)}</td></tr>`).join('')}</tbody></table></div><button class="btn primary" data-ext="shop-commit" ${p.accepted.length?'':'disabled'}>Geprüfte ${p.accepted.length} Zeilen zum Preisverlauf hinzufügen</button>`;
}
function saveShops(){try{localStorage.setItem(shopKey,JSON.stringify(shopData));}catch(e){alert('Shopdaten nicht gespeichert. Bitte CSV exportieren: '+e.message)}}
function csvDownload(name,rows){let csv='\ufeff'+rows.map(r=>r.map(x=>'"'+String(x??'').replace(/"/g,'""')+'"').join(';')).join('\r\n');let a=document.createElement('a'),url=URL.createObjectURL(new Blob([csv],{type:'text/csv;charset=utf-8'}));a.href=url;a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(url),2500);}
function showShopDetails(itemName){let g=groupShops().find(x=>x.name===itemName);if(!g)return;let cat=g.recipe?catRecipeCost(g.recipe):null;
 showModal(`<h2>${h(g.name)}</h2><p class="muted">${h(shopAssessment(g))}</p><div class="grid3"><div class="mini"><small>Preis-Minimum</small><strong>${moneyText(g.min)}</strong></div><div class="mini"><small>Median neuere Hälfte</small><strong>${moneyText(g.cur)}</strong></div><div class="mini"><small>Preis-Maximum</small><strong>${moneyText(g.max)}</strong></div></div><p class="info">${g.recipe?'Ausgabe eines Rezepts: '+h(g.recipe.name):''}${cat&&cat.net!==null?'<br>Vergleich mit Direktverkauf der Zutaten: '+moneyText(cat.net):''}</p><h3>Beobachtungen</h3><div class="table-wrap"><table class="tbl"><thead><tr><th>Zeit</th><th>Shop</th><th>Menge</th><th>Gesamt</th><th>Stückpreis</th></tr></thead><tbody>${g.rows.map(r=>`<tr><td>${h(r.time.slice(0,16).replace('T',' '))}</td><td>${h(r.seller)}</td><td>${r.qty}</td><td>${moneyText(r.total)}</td><td>${moneyText(r.unit)}</td></tr>`).join('')}</tbody></table></div><p>${modalFooter('Schließen').replace('<button type="submit" class="btn primary">Schließen</button>','')}</p>`);
}
let shopImageData=null;
function loadShopImage(file){if(!file||!file.type.startsWith('image/')){toast('Bitte ein Bild wählen.');return;}if(file.size>16*1024*1024){toast('Bild zu groß. Maximal 16 MB.');return;}
 const fr=new FileReader();fr.onload=()=>{shopImageData=String(fr.result);let box=document.getElementById('shot-box');if(!box)return;let img=new Image();img.onload=()=>{box.innerHTML='';box.append(img);};img.src=shopImageData;img.alt='Hochgeladener Shop-Screenshot';img.id='shot-img';};fr.readAsDataURL(file);
}
async function doOCR(){if(!shopImageData){toast('Bitte zuerst Screenshot auswählen.');return;}const button=document.querySelector('[data-ext="shop-ocr"]');if(button){button.disabled=true;button.textContent='OCR wird geladen / ausgeführt…'}
 try {
  if(!window.Tesseract){let script=document.createElement('script');script.src='https://cdn.jsdelivr.net/npm/tesseract.js@5.1.1/dist/tesseract.min.js';script.crossOrigin='anonymous';let loaded=new Promise((resolve,reject)=>{script.onload=resolve;script.onerror=()=>reject(Error('OCR konnte nicht geladen werden. Netzwerkverbindung prüfen.'))});document.head.appendChild(script);await loaded;}
  if(!window.Tesseract?.recognize)throw Error('OCR-Bibliothek nicht verfügbar.');
  let result=await window.Tesseract.recognize(shopImageData,'deu+eng');let text=String(result?.data?.text||'');
  if(!text.trim())throw Error('Kein Text erkannt. Screenshot stärker zuschneiden oder Text manuell übernehmen.');
  let input=document.getElementById('shop-lines');if(input){let proposed=guessOCRLines(text);input.value=proposed.structured.length?proposed.structured.join('\n'):text;view.shopInput=input.value;if(proposed.structured.length){previewShop();}}toast('OCR ausgewertet. Bitte erkannte Artikel, Mengen und Preise unbedingt kontrollieren.');
 }catch(err){alert('OCR nicht verfügbar: '+err.message+'\nDie manuelle/CSV-Eingabe bleibt nutzbar.');}
 finally{if(button){button.disabled=false;button.textContent='Text auf Screenshot erkennen (optional, online)';}}
}
function registerImageMagnifier(e){let image=document.getElementById('shot-img'),zoom=document.getElementById('shop-mag');if(!image||!zoom)return;let rect=image.getBoundingClientRect(),x=Math.max(0,Math.min(rect.width,e.clientX-rect.left)),y=Math.max(0,Math.min(rect.height,e.clientY-rect.top));zoom.hidden=false;zoom.style.backgroundImage=`url("${shopImageData}")`;zoom.style.backgroundSize=`${rect.width*2.3}px ${rect.height*2.3}px`;zoom.style.backgroundPosition=`${-x*2.3+70}px ${-y*2.3+70}px`;}
// New buttons actions have separate data-ext, and never reach the game client.
document.addEventListener('click',function(e){const btn=e.target.closest('button[data-ext]');if(!btn)return;const a=btn.dataset.ext;try{switch(a){
case 'import-recipe':importRecipe(IC.recipes.find(x=>x.id===btn.dataset.id));break;
case 'import-visible-recipes':{let filtered=IC.recipes.filter(r=>r.ingredients&&(view.recipeCategory==='alle'||r.category===view.recipeCategory)&&(view.recipeTier==='alle'||r.tier==view.recipeTier)&&(view.recipeCompleteness==='alle'||view.recipeCompleteness==='belegt')&&(r.name+' '+recIngredients(r)+' '+r.category).toLocaleLowerCase('de-DE').includes(view.recipeQuery.toLocaleLowerCase('de-DE')));let added=0;for(let rec of filtered)if(importRecipe(rec,true))added++;save();toast(added+' Rezepte übernommen, bestehende erhalten.');render();break;}
case 'recipes-template':csvDownload('IsyaGold-Rezeptvorlage.csv',[['Kategorie','Tier','Produktname','Zutat','Menge','Quelle','Status'],['Rolle','3','MEIN TESTREZEPT','Raues Leder','3','','Eigenes Beispiel'],['Rolle','3','MEIN TESTREZEPT','Bestienleder','4','','Eigenes Beispiel']]);break;
case 'import-custom-csv':document.getElementById('custom-recipe-file').click();break;
case 'shop-preview':previewShop();break;
case 'shop-suggest':{let input=document.getElementById('shop-lines'),guess=guessOCRLines(input.value);if(guess.structured.length){input.value=guess.structured.join('\n');view.shopInput=input.value;previewShop();toast(guess.structured.length+' OCR-Zeilen vorgeschlagen; bitte prüfen.');}else toast('Keine Zeilen erkannt. Bitte als Artikel;Menge;Gesamtpreis;Shop eingeben.');break;}
case 'shop-sample':{let text='Vitalität [Tier 3];5;2g 500s;Beispiel-Shop\nRaues Leder;10;250s;Beispiel-Shop';view.shopInput=text;document.getElementById('shop-lines').value=text;toast('Nur Beispielformat: nicht als echte Marktpreise importieren!');break;}
case 'shop-csv':csvDownload('IsyaGold-Shopvorlage.csv',[['Artikel','Menge','Gesamtpreis','Shopname'],['BEISPIEL (bitte löschen)','5','2g 500s','Verkäufer']]);break;
case 'shop-commit':{let vals=window.isyaPreview?.accepted||[];if(!vals.length)return;shopData.push(...vals);saveShops();view.shopInput='';window.isyaPreview=null;render();toast(vals.length+' Preisbeobachtungen übernommen.');break;}
case 'shop-export':csvDownload('IsyaGold-Preisverlauf.csv',[['Artikel','Menge','Gesamtpreis_Kupfer','Preis_je_Stueck_Kupfer','Shop','Zeit'],...shopData.map(r=>[r.name,r.qty,r.total,r.unit,r.seller,r.time])]);break;
case 'shop-reset':if(confirm('Alle gespeicherten Shop-Preisbeobachtungen löschen? Zuvor CSV exportieren.')){shopData=[];saveShops();render();}break;
case 'shop-details':showShopDetails(btn.dataset.name);break;
case 'shop-ocr':doOCR();break;
case 'guide-class':view.guideClass=btn.dataset.class;render();break;
case 'guide-update':view.guideLevel=Math.min(145,Math.max(1,Number(document.getElementById('guide-lv').value)||67));render();break;
case 'guide-all':document.getElementById('guide-list').innerHTML=guideRangeCards(IC.levels);break;
}}catch(err){alert('Aktion konnte nicht ausgeführt werden: '+err.message);}});
const SAMPLE_WIKI='https://fiesta-online.fandom.com/de/wiki/Rollen_Produktion';
document.addEventListener('input',function(e){let x=e.target;if(x.id==='cat-search'){view.recipeQuery=x.value;render();document.getElementById('cat-search')?.focus();document.getElementById('cat-search')?.setSelectionRange(x.selectionStart??0,x.selectionStart??0)}
else if(x.id==='res-search'){view.resourceQuery=x.value;render();document.getElementById('res-search')?.focus();}
else if(x.id==='shop-search'){view.shopQuery=x.value;render();document.getElementById('shop-search')?.focus();}
else if(x.id==='shop-lines'){view.shopInput=x.value;}
else if(x.id==='market-search'){view.marketQuery=x.value;document.querySelectorAll('#content table tbody tr').forEach(row=>row.hidden=!row.textContent.toLocaleLowerCase('de-DE').includes(x.value.toLocaleLowerCase('de-DE')));}
});
document.addEventListener('change',e=>{let x=e.target;
 if(x.id==='cat-type'){view.recipeCategory=x.value;render();}if(x.id==='cat-tier'){view.recipeTier=x.value;render();}if(x.id==='cat-status'){view.recipeCompleteness=x.value;render();}
 if(x.id==='res-type'){view.resourceCat=x.value;render();}
 if(x.id==='guide-range'&&x.value){view.guideLevel=Number(x.value);render();}
 if(x.id==='shop-image')loadShopImage(x.files?.[0]);
});
document.addEventListener('mousemove',e=>{if(e.target.id==='shot-img')registerImageMagnifier(e);else{let a=document.getElementById('shop-mag');if(a)a.hidden=true;}});
// Add robust backup of all newly introduced local shop data when user exports v1 JSON.
const OLD_EXPORT=exportData;
exportData=function(){const backup={...state,exportedAt:NOW(),extensions:{shopObservations:shopData,customRecipes,watches:JSON.parse(localStorage.getItem('isya-gold-needs-v3-1')||'{}')}};const blob=new Blob([JSON.stringify(backup,null,2)],{type:'application/json'}),url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download='IsyaGold-Backup-v2-'+new Date().toISOString().slice(0,10)+'.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),2000)};
// Note: v1 import handler does not import shopObservations. Include a separate transparent repair on changed data if relevant.
const importBox=document.getElementById('import-file');
importBox.addEventListener('change',async function(e){const file=e.target.files?.[0];if(!file)return;try{const data=JSON.parse(await file.text());if(data?.extensions?.shopObservations&&Array.isArray(data.extensions.shopObservations)){shopData=data.extensions.shopObservations.filter(x=>x&&typeof x.name==='string'&&Number.isFinite(x.unit)&&Number.isFinite(x.total));saveShops();}if(Array.isArray(data?.extensions?.customRecipes)){customRecipes=data.extensions.customRecipes.filter(x=>x&&typeof x.name==='string'&&Array.isArray(x.ingredients)).slice(0,3000);localStorage.setItem('isya-gold-custom-recipes-v2',JSON.stringify(customRecipes));applyCustomRecipes();}}catch(_){}},true);

function csvFields(line){let fields=[],buf='',quoted=false;for(let i=0;i<line.length;i++){let ch=line[i];if(ch==='"'){if(quoted&&line[i+1]==='"'){buf+='"';i++;}else quoted=!quoted;}else if(ch===';'&&!quoted){fields.push(buf.trim());buf='';}else buf+=ch;}fields.push(buf.trim());return fields;}
function parseCustomRecipeCsv(text){let groups=new Map(),lines=String(text).replace(/^\uFEFF/,'').split(/\r?\n/),skipped=0;
 for(let line of lines){if(!line.trim())continue;let a=csvFields(line);if(/^Kategorie$/i.test(a[0]||''))continue;
 let [category,tierRaw,name,ingredient,amount,source]=a;let tier=Number(tierRaw),qty=Number(amount);
 if(!category||!name||!ingredient||!Number.isInteger(tier)||tier<1||tier>30||!Number.isInteger(qty)||qty<1||qty>1000000){skipped++;continue;}
 if(name.length>130||ingredient.length>130){skipped++;continue;}
 let key=(category+'|'+tier+'|'+name).toLocaleLowerCase('de-DE'),obj=groups.get(key);
 if(!obj){obj={id:'user-'+id(),category,tier,name,source:source&&/^https:\/\//i.test(source)?source:'',ingredients:[],outputQty:1,skillPoints:null,status:'Eigene Eingabe – ungeprüft',note:'Über selbst importierte CSV ergänzt; gegen die Spielversion prüfen.'};groups.set(key,obj)}
 let found=obj.ingredients.find(x=>x.name===ingredient);if(found)found.qty+=qty;else obj.ingredients.push({name:ingredient,qty});
 }return {recipes:[...groups.values()],skipped};}
const customFile=document.createElement('input');customFile.type='file';customFile.id='custom-recipe-file';customFile.accept='.csv,text/csv';customFile.style.display='none';document.body.appendChild(customFile);
customFile.addEventListener('change',async e=>{let file=e.target.files?.[0];e.target.value='';if(!file)return;
 try{let parsed=parseCustomRecipeCsv(await file.text());if(!parsed.recipes.length)throw Error('Keine gültigen Rezepte. Format: Kategorie;Tier;Produktname;Zutat;Menge;Quelle;Status');if(!confirm(parsed.recipes.length+' Rezepte importieren? Bereits vollständig vorhandene Katalogrezepte werden nicht überschrieben. '+parsed.skipped+' ungültige Zeilen übersprungen.'))return;
 let added=0;for(let r of parsed.recipes){let found=IC.recipes.find(x=>x.name.toLocaleLowerCase('de-DE')===r.name.toLocaleLowerCase('de-DE')&&x.category===r.category&&x.tier===r.tier);if(found?.ingredients)continue;
 customRecipes.push(r);if(found)IC.recipes.splice(IC.recipes.indexOf(found),1);IC.recipes.push(r);added++;
 }localStorage.setItem('isya-gold-custom-recipes-v2',JSON.stringify(customRecipes));render();toast(added+' neue/geprüft ergänzbare Rezepte in Katalog übernommen.');}catch(err){alert('CSV-Import fehlgeschlagen: '+err.message)}});

// Zurücksetzen entfernt zusätzlich Erweiterungsdaten und persönliche CSV-Rezepte.
document.addEventListener('isya-gold-reset',()=>{shopData=[];customRecipes=[];localStorage.removeItem(shopKey);localStorage.removeItem('isya-gold-custom-recipes-v2');IC.recipes=IC.recipes.filter(r=>!String(r.id).startsWith('user-'));});
const nav=document.getElementById('nav');nav.insertAdjacentHTML('beforeend','<div class="nav-section">Nachschlagewerk & Analyse</div><button data-page="catalog"><span class="glyph">▦</span>Rezeptdatenbank</button><button data-page="resources"><span class="glyph">♣</span>Ressourcen & NPCs</button><button data-page="shops"><span class="glyph">▤</span>Shop-Analyse</button><button data-page="guides"><span class="glyph">♜</span>Klassen & Leveln</button>');
document.querySelector('footer.footer').innerHTML='<span>Isya Gold 2.0 · Fan-Wirtschaftsmanager · Stand Oktober 2026</span><span>Keine Spielautomation · ältere Wiki-Daten sind gekennzeichnet</span>';
render();
